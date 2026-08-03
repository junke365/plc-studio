/**
 * test_mn.c - MN（主站）集成测试
 *
 * 场景 1：MN 启动流程（单节点）
 *   回环驱动连接 MN(0xF0) 与 CN(0x01)。MN 经真实 PlkEdrv 抽象层
 *   广播 IdentRequest 发现节点，并将 CN 从 NotActive 配置到 Operational。
 * 场景 2：周期 PDO 交换
 *   MN 周期内对 CN 发 PReq（输出 PDO），CN 回 PRes（输入 PDO），双向校验。
 * 场景 3：多节点拓扑（hub 模式）
 *   hub 模拟共享介质挂 2 台 CN，MN 发现并配置两台节点后周期轮询。
 *
 * MN 的接收泵通过"协作调度 wrapper"驱动：MN 每次 poll 时先驱动所有 CN
 * 端口（处理 MN 发来的帧），再 poll 本端口（处理 CN 回复的帧）。
 *
 * 编译：见 CMakeLists.txt (add_test)
 */

#include <stdio.h>
#include <string.h>

#include "plk/plk.h"
#include "plk/mn.h"
#include "plk/cnm.h"
#include "edrv_loopback.h"

static int g_failed = 0;

#define CHECK(cond, msg) \
  do { \
    if (cond) { \
      printf("[PASS] %s\n", msg); \
    } else { \
      printf("[FAIL] %s\n", msg); \
      g_failed++; \
    } \
  } while (0)

/* ========== 节点常量 ========== */

#define MN_NODE  0xF0
#define CN1_NODE 0x01
#define CN2_NODE 0x02

static const uint8_t g_mnMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xF0};
static const uint8_t g_cn1Mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x01};
static const uint8_t g_cn2Mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x02};

/* ========== MN 协作调度 wrapper ========== */
/* 将 MN 的真实回环端口包一层：send 转发到真实端口，
 * poll 时先驱动所有 CN 端口再 poll MN 端口。 */

static PlkEdrvLoopbackPort* g_mnPort;
static PlkEdrvLoopbackPort* g_cnPorts[PLK_HUB_MAX_PORTS];
static int g_cnCount;
static PlkEdrv g_mnWrapEdrv;

static int wrapInit(PlkEdrv* self)
{
  (void)self;
  return PLK_ERR_OK;
}

static int wrapExit(PlkEdrv* self)
{
  (void)self;
  return PLK_ERR_OK;
}

static int wrapGetMac(PlkEdrv* self, uint8_t mac[6])
{
  (void)self;
  return g_mnPort->edrv.getMacAddr(&g_mnPort->edrv, mac);
}

static int wrapSend(PlkEdrv* self, const uint8_t* frame, uint16_t len)
{
  (void)self;
  return g_mnPort->edrv.send(&g_mnPort->edrv, frame, len);
}

static int wrapSetRxFilter(PlkEdrv* self, uint16_t filterMask)
{
  (void)self;
  return g_mnPort->edrv.setRxFilter(&g_mnPort->edrv, filterMask);
}

static int wrapSetRxCallback(PlkEdrv* self, PlkEdrvRxCallback cb, void* ctx)
{
  (void)self;
  return g_mnPort->edrv.setRxCallback(&g_mnPort->edrv, cb, ctx);
}

static int wrapLinkState(PlkEdrv* self, PlkLinkState* state)
{
  (void)self;
  return g_mnPort->edrv.linkState(&g_mnPort->edrv, state);
}

/* 关键：先驱动所有 CN，再收 MN 自己的帧 */
static int wrapPoll(PlkEdrv* self, uint32_t timeoutMs)
{
  int i;

  (void)self;
  for (i = 0; i < g_cnCount; i++) {
    g_cnPorts[i]->edrv.poll(&g_cnPorts[i]->edrv, 0);
  }
  return g_mnPort->edrv.poll(&g_mnPort->edrv, timeoutMs);
}

static void mnWrapInit(void)
{
  memset(&g_mnWrapEdrv, 0, sizeof(g_mnWrapEdrv));
  g_mnWrapEdrv.init = wrapInit;
  g_mnWrapEdrv.exit = wrapExit;
  g_mnWrapEdrv.getMacAddr = wrapGetMac;
  g_mnWrapEdrv.send = wrapSend;
  g_mnWrapEdrv.setRxFilter = wrapSetRxFilter;
  g_mnWrapEdrv.setRxCallback = wrapSetRxCallback;
  g_mnWrapEdrv.linkState = wrapLinkState;
  g_mnWrapEdrv.poll = wrapPoll;
}

/* ========== 接收回调 ========== */

static int cnRxCb(const uint8_t* frame, uint16_t len, void* ctx)
{
  return plk_cnm_process_rx((PlkCnm*)ctx, frame, len);
}

static int mnRxCb(const uint8_t* frame, uint16_t len, void* ctx)
{
  return plk_mn_process_rx((PlkMn*)ctx, frame, len);
}

/* ========== MN 回调（测试记录） ========== */

static uint8_t  g_foundNodes[PLK_HUB_MAX_PORTS];
static int      g_foundCount;
static uint8_t  g_pdoInByNode[PLK_HUB_MAX_PORTS][64];
static uint16_t g_pdoInSizeByNode[PLK_HUB_MAX_PORTS];
static uint8_t  g_lastPdoInNode;
static int      g_pdoInCount;
static int      g_appReadyCalls;

static int onNodeFound(PlkMn* mn, const PlkDllNodeInfo* info, void* ctx)
{
  (void)mn;
  (void)ctx;
  if (g_foundCount < PLK_HUB_MAX_PORTS) {
    g_foundNodes[g_foundCount++] = info->nodeId;
  }
  return PLK_ERR_OK;
}

static int onPdoIn(PlkMn* mn, uint8_t nodeId, const uint8_t* data,
                   uint16_t size, void* ctx)
{
  (void)mn;
  (void)ctx;
  g_lastPdoInNode = nodeId;
  if (nodeId < PLK_HUB_MAX_PORTS && size <= sizeof(g_pdoInByNode[0])) {
    memcpy(g_pdoInByNode[nodeId], data, size);
    g_pdoInSizeByNode[nodeId] = size;
  }
  g_pdoInCount++;
  return PLK_ERR_OK;
}

static int appReady(PlkMn* mn, uint8_t nodeId, bool* ready, void* ctx)
{
  (void)mn;
  (void)nodeId;
  (void)ctx;
  g_appReadyCalls++;
  *ready = true;
  return PLK_ERR_OK;
}

/* ========== 通用链路装配 ========== */

/* 装配单节点回环链路与 MN 协作调度（PDO 缓冲由调用方绑定） */
static void link_common(PlkMn* mn, PlkEdrvLoopbackPort* mnRealPort,
                        PlkCnm* cnm, PlkEdrvLoopbackPort* cnPort,
                        uint8_t cnNode)
{
  g_cnPorts[0] = cnPort;
  g_cnCount = 1;
  mnWrapInit();
  plk_mn_init(mn, MN_NODE, g_mnMac, &g_mnWrapEdrv);
  plk_cnm_init(cnm, cnNode, cnPort->mac, &cnPort->edrv);

  mnRealPort->edrv.setRxCallback(&mnRealPort->edrv, mnRxCb, mn);
  cnPort->edrv.setRxCallback(&cnPort->edrv, cnRxCb, cnm);
  mnRealPort->edrv.setRxFilter(&mnRealPort->edrv,
      PLK_RX_FILTER_PRES | PLK_RX_FILTER_ASND |
      PLK_RX_FILTER_UNICAST | PLK_RX_FILTER_BROADCAST);
  cnPort->edrv.setRxFilter(&cnPort->edrv,
      PLK_RX_FILTER_SOC | PLK_RX_FILTER_ASND |
      PLK_RX_FILTER_UNICAST | PLK_RX_FILTER_BROADCAST);

  g_foundCount = 0;
  g_pdoInCount = 0;
  g_appReadyCalls = 0;
  memset(g_pdoInByNode, 0, sizeof(g_pdoInByNode));
  memset(g_pdoInSizeByNode, 0, sizeof(g_pdoInSizeByNode));
  plk_mn_set_callbacks(mn, onNodeFound, onPdoIn, appReady, NULL);
}

/* ========== 场景 1+2：单节点 启动 + 周期 PDO ========== */

static void test_single_node(void)
{
  PlkEdrvLoopbackPort mnPort, cnPort;
  PlkMn mn;
  PlkCnm cnm;
  uint8_t cnOut[4] = {0};
  uint8_t cnIn[4] = {0x21, 0x43, 0x65, 0x87};
  uint8_t mnOut[4] = {0x12, 0x34, 0x56, 0x78};
  uint8_t mnIn[4] = {0};
  int ret;
  int i;

  plk_edrv_loopback_create(&mnPort, &cnPort, g_mnMac, g_cn1Mac);
  mnPort.edrv.init(&mnPort.edrv);
  cnPort.edrv.init(&cnPort.edrv);

  g_mnPort = &mnPort;
  link_common(&mn, &mnPort, &cnm, &cnPort, CN1_NODE);

  plk_cnm_set_pdo_in(&cnm, cnIn, 4);
  plk_cnm_set_pdo_out(&cnm, cnOut, 4, NULL, NULL);
  plk_mn_set_pdo_in(&mn, mnIn, 4);
  plk_mn_set_pdo_out(&mn, mnOut, 4);

  ret = plk_cnm_start(&cnm);
  CHECK(ret == PLK_ERR_OK && cnm.sm.state == PLK_NMT_CS_NOT_ACTIVE,
        "CN 上电初始化到 NotActive");

  ret = plk_mn_start(&mn);
  CHECK(ret == PLK_ERR_OK, "MN 启动成功");
  CHECK(mn.state == PLK_MN_STATE_OPERATIONAL, "MN 进入 Operational");
  CHECK(mn.bootPhase == PLK_MN_BOOT_DONE, "MN 启动阶段 = DONE");
  CHECK(cnm.sm.state == PLK_NMT_CS_OPERATIONAL, "CN 进入 Operational");
  CHECK(g_foundCount == 1 && g_foundNodes[0] == CN1_NODE,
        "MN 发现 1 个节点（CN1）");
  CHECK(g_appReadyCalls >= 1, "MN 执行了应用就绪握手");

  /* 周期 PDO 交换：MN→CN 输出 / CN→MN 输入 */
  g_pdoInCount = 0;
  ret = plk_mn_cycle(&mn);
  CHECK(ret == PLK_ERR_OK, "MN 执行一个周期");
  CHECK(memcmp(cnOut, mnOut, 4) == 0, "CN 收到输出 PDO（MN→CN）");
  CHECK(memcmp(mnIn, cnIn, 4) == 0, "MN 收到输入 PDO（CN→MN）");
  CHECK(g_pdoInCount >= 1 && g_lastPdoInNode == CN1_NODE,
        "输入 PDO 回调已触发（源 CN1）");

  /* 连续运行多个周期 */
  for (i = 0; i < 10; i++) {
    ret = plk_mn_cycle(&mn);
    if (ret != PLK_ERR_OK) {
      break;
    }
  }
  CHECK(ret == PLK_ERR_OK && mn.cycleCount == 11, "连续运行 10 个周期正常");

  plk_mn_exit(&mn);
  plk_cnm_exit(&cnm);
}

/* ========== 场景 3：多节点（hub 模式） ========== */

static void test_multi_node(void)
{
  PlkEdrvHub hub;
  PlkMn mn;
  PlkCnm cn1, cn2;
  uint8_t macs[3][6] = {
    {0x00, 0x11, 0x22, 0x33, 0x44, 0xF0},
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x01},
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x02},
  };
  uint8_t cn1In[2] = {0x01, 0x02};
  uint8_t cn2In[2] = {0x03, 0x04};
  uint8_t out[2] = {0xAB, 0xCD};
  int ret;
  int i;

  ret = plk_edrv_loopback_hub_create(&hub, 3, macs);
  CHECK(ret == PLK_ERR_OK, "创建 3 端口 hub");

  for (i = 0; i < 3; i++) {
    hub.ports[i].edrv.init(&hub.ports[i].edrv);
  }

  g_mnPort = &hub.ports[0];
  g_cnPorts[0] = &hub.ports[1];
  g_cnPorts[1] = &hub.ports[2];
  g_cnCount = 2;
  mnWrapInit();

  plk_mn_init(&mn, MN_NODE, macs[0], &g_mnWrapEdrv);
  plk_cnm_init(&cn1, CN1_NODE, macs[1], &hub.ports[1].edrv);
  plk_cnm_init(&cn2, CN2_NODE, macs[2], &hub.ports[2].edrv);

  hub.ports[0].edrv.setRxCallback(&hub.ports[0].edrv, mnRxCb, &mn);
  hub.ports[1].edrv.setRxCallback(&hub.ports[1].edrv, cnRxCb, &cn1);
  hub.ports[2].edrv.setRxCallback(&hub.ports[2].edrv, cnRxCb, &cn2);
  hub.ports[0].edrv.setRxFilter(&hub.ports[0].edrv,
      PLK_RX_FILTER_PRES | PLK_RX_FILTER_ASND |
      PLK_RX_FILTER_UNICAST | PLK_RX_FILTER_BROADCAST);
  hub.ports[1].edrv.setRxFilter(&hub.ports[1].edrv,
      PLK_RX_FILTER_SOC | PLK_RX_FILTER_ASND |
      PLK_RX_FILTER_UNICAST | PLK_RX_FILTER_BROADCAST);
  hub.ports[2].edrv.setRxFilter(&hub.ports[2].edrv,
      PLK_RX_FILTER_SOC | PLK_RX_FILTER_ASND |
      PLK_RX_FILTER_UNICAST | PLK_RX_FILTER_BROADCAST);

  plk_cnm_set_pdo_in(&cn1, cn1In, 2);
  plk_cnm_set_pdo_out(&cn1, NULL, 0, NULL, NULL);
  plk_cnm_set_pdo_in(&cn2, cn2In, 2);
  plk_cnm_set_pdo_out(&cn2, NULL, 0, NULL, NULL);
  plk_mn_set_pdo_in(&mn, NULL, 0);
  plk_mn_set_pdo_out(&mn, out, 2);

  g_foundCount = 0;
  g_pdoInCount = 0;
  g_appReadyCalls = 0;
  memset(g_pdoInByNode, 0, sizeof(g_pdoInByNode));
  memset(g_pdoInSizeByNode, 0, sizeof(g_pdoInSizeByNode));
  plk_mn_set_callbacks(&mn, onNodeFound, onPdoIn, appReady, NULL);

  plk_cnm_start(&cn1);
  plk_cnm_start(&cn2);

  ret = plk_mn_start(&mn);
  CHECK(ret == PLK_ERR_OK, "MN 启动（多节点）成功");
  CHECK(mn.state == PLK_MN_STATE_OPERATIONAL, "MN 进入 Operational");
  CHECK(cn1.sm.state == PLK_NMT_CS_OPERATIONAL, "CN1 进入 Operational");
  CHECK(cn2.sm.state == PLK_NMT_CS_OPERATIONAL, "CN2 进入 Operational");
  CHECK(g_foundCount == 2 &&
        ((g_foundNodes[0] == CN1_NODE && g_foundNodes[1] == CN2_NODE) ||
         (g_foundNodes[0] == CN2_NODE && g_foundNodes[1] == CN1_NODE)),
        "MN 发现 2 个节点");

  /* 周期轮询两个节点：输出 PDO 相同，输入 PDO 各自汇总 */
  ret = plk_mn_cycle(&mn);
  CHECK(ret == PLK_ERR_OK, "多节点周期执行成功");
  CHECK(g_pdoInSizeByNode[CN1_NODE] == 2 &&
        memcmp(g_pdoInByNode[CN1_NODE], cn1In, 2) == 0,
        "CN1 输入 PDO 已汇总");
  CHECK(g_pdoInSizeByNode[CN2_NODE] == 2 &&
        memcmp(g_pdoInByNode[CN2_NODE], cn2In, 2) == 0,
        "CN2 输入 PDO 已汇总");

  for (i = 0; i < 5; i++) {
    ret = plk_mn_cycle(&mn);
    if (ret != PLK_ERR_OK) {
      break;
    }
  }
  CHECK(ret == PLK_ERR_OK && mn.cycleCount == 6, "多节点连续周期正常");

  plk_mn_exit(&mn);
  plk_cnm_exit(&cn1);
  plk_cnm_exit(&cn2);
}

int main(void)
{
  printf("=== MN 主站集成测试 ===\n");
  test_single_node();
  test_multi_node();

  if (g_failed == 0) {
    printf("=== 结果: 全部通过 ===\n");
    return 0;
  }
  printf("=== 结果: %d 项失败 ===\n", g_failed);
  return 1;
}
