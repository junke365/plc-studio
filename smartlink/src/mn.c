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

#include "smartlink/mn.h"
#include "smartlink/nmt.h"
#include "smartlink/sl_core.h"

/* ========== 内部状态（接收侧记录） ========== */

/* 最近收到的 PRes 源节点（供周期内等待用），0xFF 表示无 */
#define SL_MN_NODE_NONE 0xFF

/* ========== 帧构建辅助 ========== */

static int mn_send_frame(SlMn* mn, const uint8_t* frame, uint16_t len)
{
  if (mn->edrv == NULL || mn->edrv->send == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  return mn->edrv->send(mn->edrv, frame, len);
}

static int mn_send_soc(SlMn* mn)
{
  SlFrame f;
  uint16_t len;

  len = sl_build_soc(&f, mn->mac, mn->nodeId, mn->cycleParam.cycleLen, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

static int mn_send_soa(SlMn* mn, uint8_t reqServiceId, uint8_t reqServiceTarget)
{
  SlFrame f;
  uint16_t len;

  len = sl_build_soa(&f, mn->mac, mn->nodeId,
                      (uint8_t)(SL_NMT_CS_OPERATIONAL & 0xFF),
                      reqServiceId, reqServiceTarget, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* 广播 NMT 命令（目标节点填在 NMT 命令数据首字节） */
static int mn_send_nmt_cmd(SlMn* mn, uint8_t cmdId, uint8_t targetNode)
{
  SlNmtCommandService svc;
  SlFrame f;
  uint8_t mcast[6];
  uint16_t len;

  memset(&svc, 0, sizeof(svc));
  svc.nmtCommandId = cmdId;
  svc.aNmtCommandData[0] = targetNode;

  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  len = sl_build_asnd(&f, mn->mac, mcast, mn->nodeId, SL_ADR_BROADCAST,
                       SL_ASND_NMT_COMMAND, (const uint8_t*)&svc,
                       sizeof(svc));
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* 广播 IdentRequest：请求所有 CN 回发 IdentRes */
static int mn_send_ident_request(SlMn* mn)
{
  SlFrame f;
  uint8_t mcast[6];
  uint16_t len;

  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  len = sl_build_asnd(&f, mn->mac, mcast, mn->nodeId, SL_ADR_BROADCAST,
                       SL_ASND_IDENT_RESPONSE, NULL, 0);
  return mn_send_frame(mn, (const uint8_t*)&f, len);
}

/* ========== 接收泵 ========== */

/* 泵取驱动队列帧（触发接收回调 → sl_mn_process_rx） */
static int mn_pump(SlMn* mn, uint32_t timeoutMs)
{
  if (mn->edrv == NULL || mn->edrv->poll == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  return mn->edrv->poll(mn->edrv, timeoutMs);
}

/* ========== 接收分发 ========== */

/* 处理 IdentRes：注册/更新节点表并通知应用 */
static void mn_handle_ident(SlMn* mn, const SlFrame* f)
{
  const SlIdentResponse* ident;
  SlDllNodeInfo info;
  uint16_t mtu;

  if (f->srcNodeId == SL_ADR_INVALID) {
    return;
  }
  ident = (const SlIdentResponse*)&f->data.asnd.payload;
  mtu = ident->mtuLe;
  if (mtu == 0) {
    mtu = SL_ASYNC_MTU;
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

  if (sl_dll_register_node(&info) == SL_ERR_OK && mn->onNodeFound != NULL) {
    mn->onNodeFound(mn, &info, mn->appCtx);
  }
}

/* 处理 PRes：更新节点状态并交付输入 PDO */
static void mn_handle_pres(SlMn* mn, const SlFrame* f)
{
  SlDllNodeInfo info;
  uint16_t size;
  const uint8_t* data;

  mn->rxPresNode = f->srcNodeId;

  if (sl_dll_get_node(f->srcNodeId, &info) == SL_ERR_OK) {
    info.nmtState = f->data.pres.nmtStatus;
    info.connected = true;
    info.configured = true;
    sl_dll_register_node(&info);
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

int sl_mn_process_rx(SlMn* mn, const uint8_t* raw, uint16_t len)
{
  const SlFrame* f;

  if (mn == NULL || raw == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  f = (const SlFrame*)raw;
  if (sl_frame_validate(f, len) != SL_ERR_OK) {
    return SL_ERR_PROTOCOL;
  }

  switch (f->messageType) {
    case SL_MSG_PRES:
      mn_handle_pres(mn, f);
      break;

    case SL_MSG_ASND:
      if (f->data.asnd.serviceId == SL_ASND_IDENT_RESPONSE) {
        mn_handle_ident(mn, f);
      }
      break;

    default:
      break;
  }

  return SL_ERR_OK;
}

/* ========== 周期调度 ========== */

/* 等待指定节点 PRes：泵帧直至收到或达到轮询上限 */
static int mn_wait_pres(SlMn* mn, uint8_t nodeId)
{
  uint16_t budget = (mn->presTimeoutUs > 0) ? mn->presTimeoutUs : 1000;
  uint16_t i;

  mn->rxPresNode = SL_MN_NODE_NONE;   /* 只认本次周期的 PRes */
  for (i = 0; i < budget; i++) {
    if (mn->rxPresNode == nodeId) {
      return SL_ERR_OK;
    }
    mn_pump(mn, 0);
  }
  return SL_ERR_TIMEOUT;
}

int sl_mn_cycle(SlMn* mn)
{
  SlDllNodeInfo node;
  SlFrame f;
  uint16_t len;
  uint32_t i;

  if (mn == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (mn->state != SL_MN_STATE_OPERATIONAL &&
      mn->state != SL_MN_STATE_STOPPED) {
    return SL_ERR_NMT_STATE;
  }

  mn->cycleCount++;

  /* 1. SoC：周期起始 */
  len = sl_build_soc(&f, mn->mac, mn->nodeId, mn->cycleParam.cycleLen, 0);
  if (mn_send_frame(mn, (const uint8_t*)&f, len) != SL_ERR_OK) {
    return SL_ERR_LINK_DOWN;
  }

  /* 2. 逐节点 PReq/PRes：仅轮询已连接且处于 Operational 的节点 */
  for (i = 0; i < SL_DLL_MAX_NODES; i++) {
    if (sl_dll_get_node_at(i, &node) != SL_ERR_OK) {
      continue;
    }
    if (!node.connected ||
        node.nmtState != (uint8_t)(SL_NMT_CS_OPERATIONAL & 0xFF)) {
      continue;
    }

    len = sl_build_preq(&f, mn->mac, node.aMacAddress, mn->nodeId,
                         node.nodeId, mn->pollOutData, mn->pollOutSize,
                         SL_FRAME_FLAG1_RD);
    if (mn_send_frame(mn, (const uint8_t*)&f, len) != SL_ERR_OK) {
      return SL_ERR_LINK_DOWN;
    }
    /* 收 PRes（回环/网卡均同步泵取） */
    mn_wait_pres(mn, node.nodeId);
  }

  /* 3. SoA：宣告异步阶段（无授权，供 CN 监测链路） */
  mn_send_soa(mn, SL_REQ_SERVICE_NONE, SL_ADR_MN_DEF);

  return SL_ERR_OK;
}

/* ========== 启动流程 ========== */

/* 节点发现：广播 IdentRequest 并收集 IdentRes */
static int mn_discover(SlMn* mn)
{
  uint16_t budget = (mn->bootTimeoutMs > 0) ? mn->bootTimeoutMs : 100;
  uint16_t i;

  mn->bootPhase = SL_MN_BOOT_DISCOVER;
  if (mn_send_ident_request(mn) != SL_ERR_OK) {
    return SL_ERR_LINK_DOWN;
  }

  /* 泵帧收集 IdentRes：本实现不做多轮广播，单轮即可满足直连场景 */
  for (i = 0; i < budget; i++) {
    mn_pump(mn, 0);
  }
  return SL_ERR_OK;
}

/* 配置单个节点到 Operational */
static int mn_configure_node(SlMn* mn, SlDllNodeInfo* node)
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
  mn_send_nmt_cmd(mn, SL_NMT_CMD_ENTER_READY_TO_OPERATE, node->nodeId);
  mn_pump(mn, 0);

  /* 等待 CN 应用就绪（ReadyToOperate 双标志的另一侧） */
  wait = 0;
  while (wait < budget) {
    if (mn->appReady != NULL) {
      if (mn->appReady(mn, node->nodeId, &ready, mn->appCtx) == SL_ERR_OK && ready) {
        break;
      }
    } else {
      break;   /* 未注册回调：假定已就绪 */
    }
    mn_pump(mn, 0);
    wait++;
  }
  if (wait >= budget) {
    return SL_ERR_TIMEOUT;
  }

  /* EnterOperational：CN 进入运行态 */
  mn_send_nmt_cmd(mn, SL_NMT_CMD_ENTER_OPERATIONAL, node->nodeId);
  mn_pump(mn, 0);

  node->nmtState = (uint8_t)(SL_NMT_CS_OPERATIONAL & 0xFF);
  node->configured = true;
  node->connected = true;
  sl_dll_register_node(node);
  return SL_ERR_OK;
}

int sl_mn_start(SlMn* mn)
{
  SlDllNodeInfo node;
  uint32_t i;
  int ret;

  if (mn == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* 复位节点表 */
  sl_dll_init();

  /* 链路就绪：发 SoA 宣告 MN 上线。
   * 注意不广播 SW_RESET：会把已就绪的 CN 打回复位链
   * （NotActive→Initialising，且无后续复位事件驱动回到 NotActive）。 */
  mn->bootPhase = SL_MN_BOOT_RESET_COM;
  mn_send_soa(mn, SL_REQ_SERVICE_NONE, SL_ADR_MN_DEF);
  mn_pump(mn, 0);

  /* 节点发现 */
  ret = mn_discover(mn);
  if (ret != SL_ERR_OK) {
    mn->bootPhase = SL_MN_BOOT_FAILED;
    return ret;
  }

  /* 节点配置 */
  mn->bootPhase = SL_MN_BOOT_CONFIG;
  for (i = 0; i < SL_DLL_MAX_NODES; i++) {
    if (sl_dll_get_node_at(i, &node) != SL_ERR_OK) {
      continue;
    }
    ret = mn_configure_node(mn, &node);
    if (ret != SL_ERR_OK) {
      mn->bootPhase = SL_MN_BOOT_FAILED;
      return ret;
    }
  }

  mn->bootPhase = SL_MN_BOOT_DONE;
  mn->state = SL_MN_STATE_OPERATIONAL;
  mn->cycleCount = 0;
  return SL_ERR_OK;
}

/* ========== 接口 ========== */

int sl_mn_init(SlMn* mn, uint8_t nodeId, const uint8_t mac[6], SlEdrv* edrv)
{
  if (mn == NULL || mac == NULL || edrv == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (edrv->send == NULL || edrv->poll == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  memset(mn, 0, sizeof(*mn));
  mn->nodeId = nodeId;
  memcpy(mn->mac, mac, 6);
  mn->edrv = edrv;
  mn->state = SL_MN_STATE_OFFLINE;
  mn->bootPhase = SL_MN_BOOT_IDLE;
  mn->bootTimeoutMs = 100;
  mn->presTimeoutUs = 1000;
  mn->rxPresNode = SL_MN_NODE_NONE;

  mn->cycleParam.cycleLen = 100000;        /* 100us */
  mn->cycleParam.multipliedCycle = 1;
  mn->cycleParam.prescaledCycle = 1;
  mn->asyncParam.asyncMtu = SL_ASYNC_MTU;
  mn->asyncParam.asyncSlotId = SL_ADR_MN_DEF;
  mn->asyncParam.asyncSlotPriority = SL_ASYNC_PRIO_GENERIC;
  mn->asyncParam.asyncSlotTimeout = 100;

  sl_dll_init();
  return SL_ERR_OK;
}

void sl_mn_exit(SlMn* mn)
{
  if (mn == NULL) {
    return;
  }
  mn->state = SL_MN_STATE_OFFLINE;
  mn->bootPhase = SL_MN_BOOT_IDLE;
}

int sl_mn_stop(SlMn* mn)
{
  if (mn == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  mn->state = SL_MN_STATE_STOPPED;
  return SL_ERR_OK;
}

int sl_mn_set_pdo_out(SlMn* mn, uint8_t* data, uint16_t size)
{
  if (mn == NULL || (size > 0 && data == NULL)) {
    return SL_ERR_INVALID_PARAM;
  }
  mn->pollOutData = data;
  mn->pollOutSize = size;
  return SL_ERR_OK;
}

int sl_mn_set_pdo_in(SlMn* mn, uint8_t* data, uint16_t size)
{
  if (mn == NULL || (size > 0 && data == NULL)) {
    return SL_ERR_INVALID_PARAM;
  }
  mn->pollInData = data;
  mn->pollInSize = size;
  return SL_ERR_OK;
}

int sl_mn_set_callbacks(SlMn* mn, SlMnNodeFoundCb nodeFound,
                         SlMnPdoInCb pdoIn, SlMnAppReadyCb appReady,
                         void* ctx)
{
  if (mn == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  mn->onNodeFound = nodeFound;
  mn->onPdoIn = pdoIn;
  mn->appReady = appReady;
  mn->appCtx = ctx;
  return SL_ERR_OK;
}

int sl_mn_register_node(SlMn* mn, const SlDllNodeInfo* info)
{
  (void)mn;
  if (info == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  return sl_dll_register_node(info);
}
