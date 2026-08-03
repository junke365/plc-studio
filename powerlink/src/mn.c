/**
 * mn.c - MN（管理节点/主站）协议栈实现
 *
 * 职责：
 *   - 启动流程：广播通信复位 → 节点发现（IdentRequest/IdentRes）→
 *               节点配置到 Operational（含 ReadyToOperate 双标志握手）
 *   - 周期调度：SoC → 逐节点 PReq → 收 PRes → SoA 异步阶段
 *   - 接收分发：IdentRes / PRes / ASnd
 *
 * 时序说明：本实现为轻量同步调度，超时用"驱动轮询次数"近似，
 * 无真实时钟依赖；周期节奏由调用方控制（定时器/主循环）。
 */

#include "plk/mn.h"
#include "plk/nmt.h"
#include "plk/plk_core.h"

/* ========== 内部状态（接收侧记录） ========== */

/* 最近收到的 PRes 源节点（供周期内等待用），0xFF 表示无 */
#define PLK_MN_NODE_NONE 0xFF

/* ========== 帧构建辅助 ========== */

static int mn_send_frame(PlkMn* mn, const uint8_t* frame, uint16_t len)
{
  if (mn->edrv == NULL || mn->edrv->send == NULL) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  return mn->edrv->send(mn->edrv, frame, len);
}

static int mn_send_soc(PlkMn* mn)
{
  PlkFrame f;
  uint16_t len;

  len = plk_build_soc(&f, mn->mac, mn->nodeId, mn->cycleParam.cycleLen, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

static int mn_send_soa(PlkMn* mn, uint8_t reqServiceId, uint8_t reqServiceTarget)
{
  PlkFrame f;
  uint16_t len;

  len = plk_build_soa(&f, mn->mac, mn->nodeId,
                      (uint8_t)(PLK_NMT_CS_OPERATIONAL & 0xFF),
                      reqServiceId, reqServiceTarget, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* 广播 NMT 命令（目标节点填在 NMT 命令数据首字节） */
static int mn_send_nmt_cmd(PlkMn* mn, uint8_t cmdId, uint8_t targetNode)
{
  PlkNmtCommandService svc;
  PlkFrame f;
  uint8_t mcast[6];
  uint16_t len;

  memset(&svc, 0, sizeof(svc));
  svc.nmtCommandId = cmdId;
  svc.aNmtCommandData[0] = targetNode;

  plk_get_mcast_mac(PLK_MSG_ASND, mcast);
  len = plk_build_asnd(&f, mn->mac, mcast, mn->nodeId, PLK_ADR_BROADCAST,
                       PLK_ASND_NMT_COMMAND, (const uint8_t*)&svc,
                       sizeof(svc));
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* 广播 IdentRequest：请求所有 CN 回发 IdentRes */
static int mn_send_ident_request(PlkMn* mn)
{
  PlkFrame f;
  uint8_t mcast[6];
  uint16_t len;

  plk_get_mcast_mac(PLK_MSG_ASND, mcast);
  len = plk_build_asnd(&f, mn->mac, mcast, mn->nodeId, PLK_ADR_BROADCAST,
                       PLK_ASND_IDENT_RESPONSE, NULL, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* ========== 接收泵 ========== */

/* 泵取驱动队列帧（触发接收回调 → plk_mn_process_rx） */
static int mn_pump(PlkMn* mn, uint32_t timeoutMs)
{
  if (mn->edrv == NULL || mn->edrv->poll == NULL) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  return mn->edrv->poll(mn->edrv, timeoutMs);
}

/* ========== 接收分发 ========== */

/* 处理 IdentRes：注册/更新节点表并通知应用 */
static void mn_handle_ident(PlkMn* mn, const PlkFrame* f)
{
  const PlkIdentResponse* ident;
  PlkDllNodeInfo info;
  uint16_t mtu;

  if (f->srcNodeId == PLK_ADR_INVALID) {
    return;
  }
  ident = (const PlkIdentResponse*)&f->data.asnd.payload;
  mtu = ident->mtuLe;
  if (mtu == 0) {
    mtu = PLK_ASYNC_MTU;
  }

  memset(&info, 0, sizeof(info));
  info.nodeId = f->srcNodeId;
  info.nodeType = 1;                          /* CN */
  info.nmtState = ident->nmtStatus;
  info.prescaler = 1;
  info.configured = false;
  info.connected = false;
  info.presChaining = false;
  memcpy(info.aMacAddress, f->aSrcMac, 6);
  info.minCycleTime = 0;
  info.maxCycleTime = 0;
  info.minAsyncMtu = mtu;
  info.maxAsyncMtu = mtu;

  if (plk_dll_register_node(&info) == PLK_ERR_OK && mn->onNodeFound != NULL) {
    mn->onNodeFound(mn, &info, mn->appCtx);
  }
}

/* 处理 PRes：更新节点状态并交付输入 PDO */
static void mn_handle_pres(PlkMn* mn, const PlkFrame* f)
{
  PlkDllNodeInfo info;
  uint16_t size;
  const uint8_t* data;

  mn->rxPresNode = f->srcNodeId;

  if (plk_dll_get_node(f->srcNodeId, &info) == PLK_ERR_OK) {
    info.nmtState = f->data.pres.nmtStatus;
    info.connected = true;
    info.configured = true;
    plk_dll_register_node(&info);
  }

  size = f->data.pres.sizeLe;
  if (size > 256) {
    size = 256;
  }
  data = f->data.pres.aPayload;

  if (mn->pollInData != NULL && size > 0) {
    uint16_t n = (size < mn->pollInSize) ? size : mn->pollInSize;
    memcpy(mn->pollInData, data, n);
  }
  if (mn->onPdoIn != NULL) {
    mn->onPdoIn(mn, f->srcNodeId, data, size, mn->appCtx);
  }
}

int plk_mn_process_rx(PlkMn* mn, const uint8_t* raw, uint16_t len)
{
  const PlkFrame* f;

  if (mn == NULL || raw == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  f = (const PlkFrame*)raw;
  if (plk_frame_validate(f, len) != PLK_ERR_OK) {
    return PLK_ERR_PROTOCOL;
  }

  switch (f->messageType) {
    case PLK_MSG_PRES:
      mn_handle_pres(mn, f);
      break;

    case PLK_MSG_ASND:
      if (f->data.asnd.serviceId == PLK_ASND_IDENT_RESPONSE) {
        mn_handle_ident(mn, f);
      }
      break;

    default:
      break;
  }

  return PLK_ERR_OK;
}

/* ========== 周期调度 ========== */

/* 等待指定节点 PRes：泵帧直至收到或达到轮询上限 */
static int mn_wait_pres(PlkMn* mn, uint8_t nodeId)
{
  uint16_t budget = (mn->presTimeoutUs > 0) ? mn->presTimeoutUs : 1000;
  uint16_t i;

  mn->rxPresNode = PLK_MN_NODE_NONE;   /* 只认本次周期的 PRes */
  for (i = 0; i < budget; i++) {
    if (mn->rxPresNode == nodeId) {
      return PLK_ERR_OK;
    }
    mn_pump(mn, 0);
  }
  return PLK_ERR_TIMEOUT;
}

int plk_mn_cycle(PlkMn* mn)
{
  PlkDllNodeInfo node;
  PlkFrame f;
  uint16_t len;
  uint32_t i;

  if (mn == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  if (mn->state != PLK_MN_STATE_OPERATIONAL &&
      mn->state != PLK_MN_STATE_STOPPED) {
    return PLK_ERR_NMT_STATE;
  }

  mn->cycleCount++;

  /* 1. SoC：周期起始 */
  len = plk_build_soc(&f, mn->mac, mn->nodeId, mn->cycleParam.cycleLen, 0);
  if (mn_send_frame(mn, (const uint8_t*)&f, len) != PLK_ERR_OK) {
    return PLK_ERR_LINK_DOWN;
  }

  /* 2. 逐节点 PReq/PRes：仅轮询已连接且处于 Operational 的节点 */
  for (i = 0; i < PLK_DLL_MAX_NODES; i++) {
    if (plk_dll_get_node_at(i, &node) != PLK_ERR_OK) {
      continue;
    }
    if (!node.connected ||
        node.nmtState != (uint8_t)(PLK_NMT_CS_OPERATIONAL & 0xFF)) {
      continue;
    }

    len = plk_build_preq(&f, mn->mac, node.aMacAddress, mn->nodeId,
                         node.nodeId, mn->pollOutData, mn->pollOutSize,
                         PLK_FRAME_FLAG1_RD);
    if (mn_send_frame(mn, (const uint8_t*)&f, len) != PLK_ERR_OK) {
      return PLK_ERR_LINK_DOWN;
    }
    /* 收 PRes（回环/网卡均同步泵取） */
    mn_wait_pres(mn, node.nodeId);
  }

  /* 3. SoA：宣告异步阶段（无授权，供 CN 监测链路） */
  mn_send_soa(mn, PLK_REQ_SERVICE_NONE, PLK_ADR_MN_DEF);

  return PLK_ERR_OK;
}

/* ========== 启动流程 ========== */

/* 节点发现：广播 IdentRequest 并收集 IdentRes */
static int mn_discover(PlkMn* mn)
{
  uint16_t budget = (mn->bootTimeoutMs > 0) ? mn->bootTimeoutMs : 100;
  uint16_t i;

  mn->bootPhase = PLK_MN_BOOT_DISCOVER;
  if (mn_send_ident_request(mn) != PLK_ERR_OK) {
    return PLK_ERR_LINK_DOWN;
  }

  /* 泵帧收集 IdentRes：本实现不做多轮广播，单轮即可满足直连场景 */
  for (i = 0; i < budget; i++) {
    mn_pump(mn, 0);
  }
  return PLK_ERR_OK;
}

/* 配置单个节点到 Operational */
static int mn_configure_node(PlkMn* mn, PlkDllNodeInfo* node)
{
  uint16_t budget = (mn->bootTimeoutMs > 0) ? mn->bootTimeoutMs : 100;
  uint16_t wait;
  bool ready;

  /* SoC ×2：CN 从 NotActive → PreOp1 → PreOp2 */
  mn_send_soc(mn);
  mn_pump(mn, 0);
  mn_send_soc(mn);
  mn_pump(mn, 0);

  /* EnableReadyToOperate：置 CN 侧握手标志 */
  mn_send_nmt_cmd(mn, PLK_NMT_CMD_ENTER_READY_TO_OPERATE, node->nodeId);
  mn_pump(mn, 0);

  /* 等待 CN 应用就绪（ReadyToOperate 双标志的另一侧） */
  wait = 0;
  while (wait < budget) {
    if (mn->appReady != NULL) {
      if (mn->appReady(mn, node->nodeId, &ready, mn->appCtx) == PLK_ERR_OK && ready) {
        break;
      }
    } else {
      break;   /* 未注册回调：假定已就绪 */
    }
    mn_pump(mn, 0);
    wait++;
  }
  if (wait >= budget) {
    return PLK_ERR_TIMEOUT;
  }

  /* EnterOperational：CN 进入运行态 */
  mn_send_nmt_cmd(mn, PLK_NMT_CMD_ENTER_OPERATIONAL, node->nodeId);
  mn_pump(mn, 0);

  node->nmtState = (uint8_t)(PLK_NMT_CS_OPERATIONAL & 0xFF);
  node->configured = true;
  node->connected = true;
  plk_dll_register_node(node);
  return PLK_ERR_OK;
}

int plk_mn_start(PlkMn* mn)
{
  PlkDllNodeInfo node;
  uint32_t i;
  int ret;

  if (mn == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  /* 复位节点表 */
  plk_dll_init();

  /* 链路就绪：发 SoA 宣告 MN 上线。
   * 注意不广播 SW_RESET：会把已就绪的 CN 打回复位链
   * （NotActive→Initialising，且无后续复位事件驱动回到 NotActive）。 */
  mn->bootPhase = PLK_MN_BOOT_RESET_COM;
  mn_send_soa(mn, PLK_REQ_SERVICE_NONE, PLK_ADR_MN_DEF);
  mn_pump(mn, 0);

  /* 节点发现 */
  ret = mn_discover(mn);
  if (ret != PLK_ERR_OK) {
    mn->bootPhase = PLK_MN_BOOT_FAILED;
    return ret;
  }

  /* 节点配置 */
  mn->bootPhase = PLK_MN_BOOT_CONFIG;
  for (i = 0; i < PLK_DLL_MAX_NODES; i++) {
    if (plk_dll_get_node_at(i, &node) != PLK_ERR_OK) {
      continue;
    }
    ret = mn_configure_node(mn, &node);
    if (ret != PLK_ERR_OK) {
      mn->bootPhase = PLK_MN_BOOT_FAILED;
      return ret;
    }
  }

  mn->bootPhase = PLK_MN_BOOT_DONE;
  mn->state = PLK_MN_STATE_OPERATIONAL;
  mn->cycleCount = 0;
  return PLK_ERR_OK;
}

/* ========== 接口 ========== */

int plk_mn_init(PlkMn* mn, uint8_t nodeId, const uint8_t mac[6], PlkEdrv* edrv)
{
  if (mn == NULL || mac == NULL || edrv == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  if (edrv->send == NULL || edrv->poll == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  memset(mn, 0, sizeof(*mn));
  mn->nodeId = nodeId;
  memcpy(mn->mac, mac, 6);
  mn->edrv = edrv;
  mn->state = PLK_MN_STATE_OFFLINE;
  mn->bootPhase = PLK_MN_BOOT_IDLE;
  mn->bootTimeoutMs = 100;
  mn->presTimeoutUs = 1000;
  mn->rxPresNode = PLK_MN_NODE_NONE;

  mn->cycleParam.cycleLen = 100000;        /* 100us */
  mn->cycleParam.multipliedCycle = 1;
  mn->cycleParam.prescaledCycle = 1;
  mn->asyncParam.asyncMtu = PLK_ASYNC_MTU;
  mn->asyncParam.asyncSlotId = PLK_ADR_MN_DEF;
  mn->asyncParam.asyncSlotPriority = PLK_ASYNC_PRIO_GENERIC;
  mn->asyncParam.asyncSlotTimeout = 100;

  plk_dll_init();
  return PLK_ERR_OK;
}

void plk_mn_exit(PlkMn* mn)
{
  if (mn == NULL) {
    return;
  }
  mn->state = PLK_MN_STATE_OFFLINE;
  mn->bootPhase = PLK_MN_BOOT_IDLE;
}

int plk_mn_stop(PlkMn* mn)
{
  if (mn == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  mn->state = PLK_MN_STATE_STOPPED;
  return PLK_ERR_OK;
}

int plk_mn_set_pdo_out(PlkMn* mn, uint8_t* data, uint16_t size)
{
  if (mn == NULL || (size > 0 && data == NULL)) {
    return PLK_ERR_INVALID_PARAM;
  }
  mn->pollOutData = data;
  mn->pollOutSize = size;
  return PLK_ERR_OK;
}

int plk_mn_set_pdo_in(PlkMn* mn, uint8_t* data, uint16_t size)
{
  if (mn == NULL || (size > 0 && data == NULL)) {
    return PLK_ERR_INVALID_PARAM;
  }
  mn->pollInData = data;
  mn->pollInSize = size;
  return PLK_ERR_OK;
}

int plk_mn_set_callbacks(PlkMn* mn, PlkMnNodeFoundCb nodeFound,
                         PlkMnPdoInCb pdoIn, PlkMnAppReadyCb appReady,
                         void* ctx)
{
  if (mn == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  mn->onNodeFound = nodeFound;
  mn->onPdoIn = pdoIn;
  mn->appReady = appReady;
  mn->appCtx = ctx;
  return PLK_ERR_OK;
}

int plk_mn_register_node(PlkMn* mn, const PlkDllNodeInfo* info)
{
  (void)mn;
  if (info == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  return plk_dll_register_node(info);
}
