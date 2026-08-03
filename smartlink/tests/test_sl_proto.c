/**
 * test_sl_proto.c - 协议层集成测试（回环 edrv 直连 MN/CN）
 *
 * 场景 1：NMT + PDO 互通
 *   内存回环驱动连接 MN(0xF0) 与 CN(0x01)。MN 通过 edrv 发送 SoC /
 *   NMT 命令 / PReq，CN 侧由 SlCnm 栈处理状态迁移并回发 PRes，
 *   全程走真实 SlEdrv 抽象层（发送、过滤、接收回调、轮询）。
 * 场景 2：SDO 服务器侧
 *   MN 构建 SDO 读写请求，SlCnm 内的 SDO 服务器应答并操作对象字典。
 * 场景 3：SDO 客户端侧
 *   MN 用 SDO 客户端阻塞式读写（响应经回环驱动注入）。
 *
 * 编译：见 CMakeLists.txt (add_test)
 */

#include <stdio.h>
#include <string.h>

#include "smartlink/smartlink.h"
#include "smartlink/frame.h"
#include "smartlink/sl_core.h"
#include "smartlink/od.h"
#include "smartlink/sdo.h"
#include "smartlink/cnm.h"
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

#define CN_NODE 0x01
#define MN_NODE 0xF0

static const uint8_t g_cnMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x01};
static const uint8_t g_mnMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xF0};

/* ========== 回环驱动与 CN 栈 ========== */

static SlEdrvLoopbackPort g_mnPort;   /* MN 网卡 */
static SlEdrvLoopbackPort g_cnPort;   /* CN 网卡 */
static SlCnm g_cnm;                   /* CN 协议栈 */

static uint8_t g_cnOutput[4];
static uint8_t g_cnInput[4] = {0x11, 0x22, 0x33, 0x44};

/* MN 接收缓冲（收到 CN 的帧） */
static uint8_t g_mnRxBuf[SL_ETH_FRAME_MAX];
static uint16_t g_mnRxLen;

/* CN 侧：edrv 接收回调 → 协议栈 */
static int cnRxCb(const uint8_t* frame, uint16_t len, void* ctx)
{
  return sl_cnm_process_rx((SlCnm*)ctx, frame, len);
}

/* MN 侧：edrv 接收回调 → 存缓冲 */
static int mnRxCb(const uint8_t* frame, uint16_t len, void* ctx)
{
  (void)ctx;
  if (len > sizeof(g_mnRxBuf)) {
    return SL_ERR_INVALID_PARAM;
  }
  memcpy(g_mnRxBuf, frame, len);
  g_mnRxLen = len;
  return SL_ERR_OK;
}

/* 初始化回环链路 + CN 栈 */
static void test_link_setup(void)
{
  sl_edrv_loopback_create(&g_mnPort, &g_cnPort, g_mnMac, g_cnMac);
  g_mnPort.edrv.init(&g_mnPort.edrv);
  g_cnPort.edrv.init(&g_cnPort.edrv);

  sl_cnm_init(&g_cnm, CN_NODE, g_cnMac, &g_cnPort.edrv);

  g_cnPort.edrv.setRxCallback(&g_cnPort.edrv, cnRxCb, &g_cnm);
  g_mnPort.edrv.setRxCallback(&g_mnPort.edrv, mnRxCb, NULL);

  /* 接收过滤器：MN 收 PRes/ASnd/单播；CN 收 SoC/ASnd/单播 */
  g_mnPort.edrv.setRxFilter(&g_mnPort.edrv,
      SL_RX_FILTER_PRES | SL_RX_FILTER_ASND |
      SL_RX_FILTER_UNICAST | SL_RX_FILTER_BROADCAST);
  g_cnPort.edrv.setRxFilter(&g_cnPort.edrv,
      SL_RX_FILTER_SOC | SL_RX_FILTER_ASND |
      SL_RX_FILTER_UNICAST | SL_RX_FILTER_BROADCAST);

  sl_cnm_set_pdo_in(&g_cnm, g_cnInput, 4);
  sl_cnm_set_pdo_out(&g_cnm, g_cnOutput, 4, NULL, NULL);
}

/* ========== MN 侧帧构建 ========== */

static uint16_t test_build_soc(SlFrame* f)
{
  return sl_build_soc(f, g_mnMac, MN_NODE, 100000, 0);
}

static uint16_t test_build_nmt_cmd(SlFrame* f, uint8_t cmdId)
{
  SlNmtCommandService svc;
  uint8_t mcast[6];

  memset(&svc, 0, sizeof(svc));
  svc.nmtCommandId = cmdId;
  svc.aNmtCommandData[0] = CN_NODE;
  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  return sl_build_asnd(f, g_mnMac, mcast, MN_NODE, SL_ADR_BROADCAST,
                        SL_ASND_NMT_COMMAND, (const uint8_t*)&svc, sizeof(svc));
}

static uint16_t test_build_preq(SlFrame* f, const uint8_t* payload, uint16_t size)
{
  return sl_build_preq(f, g_mnMac, g_cnMac, MN_NODE, CN_NODE,
                        payload, size, SL_FRAME_FLAG1_RD);
}

static uint8_t g_sdoReqSeq;

static uint16_t test_build_sdo_request(SlFrame* f, uint8_t cmd, uint16_t index,
                                       uint8_t subIndex, const uint8_t* data,
                                       uint16_t size)
{
  SlAsySdoSeq seq;
  uint8_t payload[32];
  uint8_t mcast[6];

  memset(&seq, 0, sizeof(seq));
  seq.sendSeqNumCon = ++g_sdoReqSeq;
  seq.sdoSeqPayload.transactionId = g_sdoReqSeq;
  seq.sdoSeqPayload.flags = SL_SDO_CMDL_FLAG_EXPEDITED;
  if (cmd == SL_SDO_CID_WRITE_BY_INDEX) {
    seq.sdoSeqPayload.flags |= (uint8_t)(4 - size);
    memcpy(&seq.sdoSeqPayload.aCommandData[4], data, size);
  }
  seq.sdoSeqPayload.commandId = cmd;
  seq.sdoSeqPayload.aCommandData[0] = (uint8_t)(index & 0xFF);
  seq.sdoSeqPayload.aCommandData[1] = (uint8_t)(index >> 8);
  seq.sdoSeqPayload.aCommandData[2] = subIndex;

  memcpy(payload, &seq, sizeof(seq));
  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  return sl_build_asnd(f, g_mnMac, mcast, MN_NODE, CN_NODE, SL_ASND_SDO,
                        payload, (uint16_t)(4 + sizeof(seq)));
}

/* ========== 场景 1：NMT + PDO 互通 ========== */

static void test_nmt_pdo_interop(void)
{
  SlFrame f;
  SlFrame pres;
  uint8_t out[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  uint16_t len;
  int ret;

  test_link_setup();

  /* CN 上电初始化到 NotActive */
  ret = sl_cnm_start(&g_cnm);
  CHECK(ret == SL_ERR_OK && g_cnm.sm.state == SL_NMT_CS_NOT_ACTIVE,
        "CN 上电初始化到 NotActive");

  /* MN 发 SoC → CN 进入 PreOp1 */
  len = test_build_soc(&f);
  ret = g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&f, len);
  CHECK(ret == SL_ERR_OK, "MN 经 edrv 发送 SoC 成功");
  ret = g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  CHECK(ret == SL_ERR_OK, "MN→CN 队列可取帧");
  CHECK(g_cnm.sm.state == SL_NMT_CS_PRE_OPERATIONAL_1, "收到 SoC → PreOp1");

  /* 再次 SoC → PreOp2 */
  len = test_build_soc(&f);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&f, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  CHECK(g_cnm.sm.state == SL_NMT_CS_PRE_OPERATIONAL_2, "再次收到 SoC → PreOp2");

  /* MN 发 EnterReadyToOperate 命令（允许就绪），CN 等待应用握手 */
  len = test_build_nmt_cmd(&f, SL_NMT_CMD_ENTER_READY_TO_OPERATE);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&f, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  CHECK(g_cnm.sm.state == SL_NMT_CS_PRE_OPERATIONAL_2 && g_cnm.sm.mnReadyToOperate,
        "MN 允许就绪，等待应用握手");

  /* 应用就绪 → 双握手完成 → ReadyToOperate */
  ret = sl_cnm_app_ready(&g_cnm);
  CHECK(ret == SL_ERR_OK && g_cnm.sm.state == SL_NMT_CS_READY_TO_OPERATE,
        "双握手完成 → ReadyToOperate");

  /* MN 发 EnterOperational 命令 → Operational */
  len = test_build_nmt_cmd(&f, SL_NMT_CMD_ENTER_OPERATIONAL);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&f, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  CHECK(g_cnm.sm.state == SL_NMT_CS_OPERATIONAL, "EnterOperational → Operational");

  /* MN 发 PReq（输出 PDO）→ CN 回 PRes（输入 PDO） */
  len = test_build_preq(&f, out, 4);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&f, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  CHECK(memcmp(g_cnOutput, out, 4) == 0, "CN 已接收输出 PDO");

  ret = g_mnPort.edrv.poll(&g_mnPort.edrv, 0);
  CHECK(ret == SL_ERR_OK, "CN 已回复 PRes");
  if (ret == SL_ERR_OK) {
    memcpy(&pres, g_mnRxBuf, sizeof(pres));
    CHECK(pres.messageType == SL_MSG_PRES, "PRes messageType 正确");
    CHECK(pres.srcNodeId == CN_NODE, "PRes 源节点 = CN");
    CHECK(pres.data.pres.nmtStatus == (uint8_t)(SL_NMT_CS_OPERATIONAL & 0xFF),
          "PRes NMT 状态 = Operational");
    CHECK(pres.data.pres.sizeLe == 4 &&
          memcmp(pres.data.pres.aPayload, g_cnInput, 4) == 0,
          "PRes 载荷 = 输入 PDO");
  }
}

/* ========== 场景 2：SDO 服务器侧 ========== */

static void test_sdo_server(void)
{
  static uint16_t myVar;
  static SlOdEntry entry;
  SlFrame req;
  SlFrame resp;
  SlAsySdoSeq seq;
  uint16_t len;
  uint16_t wval;
  uint16_t rd;

  /* CN 对象字典：0x2000 UINT16 变量 */
  memset(&entry, 0, sizeof(entry));
  entry.index = 0x2000;
  entry.subIndex = 0;
  entry.access = SL_OD_ACC_RW;
  entry.objectType = SL_OD_OBJ_VAR;
  entry.dataType = SL_OD_TYPE_UINT16;
  entry.size = 2;
  entry.data = &myVar;
  sl_od_add(&g_cnm.od, &entry);

  /* 写请求：更新 OD */
  wval = 0x1234;
  len = test_build_sdo_request(&req, SL_SDO_CID_WRITE_BY_INDEX, 0x2000, 0,
                               (const uint8_t*)&wval, 2);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&req, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  g_mnPort.edrv.poll(&g_mnPort.edrv, 0);
  CHECK(myVar == 0x1234, "SDO 写请求更新 OD");
  CHECK(g_mnRxLen > 0, "SDO 写请求有响应");
  if (g_mnRxLen > 0) {
    memcpy(&resp, g_mnRxBuf, sizeof(resp));
    seq = *(const SlAsySdoSeq*)&resp.data.asnd.payload;
    CHECK(resp.messageType == SL_MSG_ASND &&
          resp.data.asnd.serviceId == SL_ASND_SDO, "SDO 响应为 ASnd/SDO");
    CHECK(resp.srcNodeId == CN_NODE && resp.dstNodeId == MN_NODE,
          "SDO 响应节点正确");
    CHECK((seq.sdoSeqPayload.flags & SL_SDO_CMDL_FLAG_RESPONSE) != 0,
          "SDO 响应标志 RESPONSE");
    CHECK((seq.sdoSeqPayload.flags & SL_SDO_CMDL_FLAG_ABORT) == 0,
          "SDO 写响应无 Abort");
  }

  /* 读请求：读回 OD 值 */
  g_mnRxLen = 0;
  len = test_build_sdo_request(&req, SL_SDO_CID_READ_BY_INDEX, 0x2000, 0,
                               NULL, 0);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&req, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  g_mnPort.edrv.poll(&g_mnPort.edrv, 0);
  CHECK(g_mnRxLen > 0, "SDO 读请求有响应");
  if (g_mnRxLen > 0) {
    memcpy(&resp, g_mnRxBuf, sizeof(resp));
    seq = *(const SlAsySdoSeq*)&resp.data.asnd.payload;
    CHECK(seq.sdoSeqPayload.commandId == SL_SDO_CID_READ_BY_INDEX,
          "SDO 读响应命令 ID 正确");
    rd = (uint16_t)(seq.sdoSeqPayload.aCommandData[4] |
                    (seq.sdoSeqPayload.aCommandData[5] << 8));
    CHECK(rd == 0x1234, "SDO 读响应数据 = OD 值");
  }

  /* 读不存在的索引 → Abort */
  g_mnRxLen = 0;
  len = test_build_sdo_request(&req, SL_SDO_CID_READ_BY_INDEX, 0x9999, 0,
                               NULL, 0);
  g_mnPort.edrv.send(&g_mnPort.edrv, (const uint8_t*)&req, len);
  g_cnPort.edrv.poll(&g_cnPort.edrv, 0);
  g_mnPort.edrv.poll(&g_mnPort.edrv, 0);
  CHECK(g_mnRxLen > 0, "读不存在索引有响应");
  if (g_mnRxLen > 0) {
    memcpy(&resp, g_mnRxBuf, sizeof(resp));
    seq = *(const SlAsySdoSeq*)&resp.data.asnd.payload;
    CHECK((seq.sdoSeqPayload.flags & SL_SDO_CMDL_FLAG_ABORT) != 0,
          "读不存在索引 → Abort");
  }
}

/* ========== 场景 3：SDO 客户端侧 ========== */

static uint8_t g_mnReqTid;
static uint8_t g_mnReqCmd;
static uint8_t g_mnReqSub;
static uint16_t g_simReadValue;

/* MN 客户端发送请求：构造 CN 模拟响应并经回环注入本机 */
static int mnSdoSendFn(const uint8_t* frame, uint16_t len)
{
  const SlFrame* f = (const SlFrame*)frame;
  const SlAsySdoSeq* seq = (const SlAsySdoSeq*)&f->data.asnd.payload;
  SlFrame resp;
  SlAsySdoSeq rseq;
  uint8_t payload[32];
  uint8_t mcast[6];
  uint16_t rlen;

  (void)len;

  g_mnReqTid = seq->sdoSeqPayload.transactionId;
  g_mnReqCmd = seq->sdoSeqPayload.commandId;
  g_mnReqSub = seq->sdoSeqPayload.aCommandData[2];

  /* 构造 CN 模拟响应 */
  memset(&rseq, 0, sizeof(rseq));
  rseq.recvSeqNumCon = g_mnReqTid;
  rseq.sendSeqNumCon = 1;
  rseq.sdoSeqPayload.transactionId = g_mnReqTid;
  rseq.sdoSeqPayload.flags = SL_SDO_CMDL_FLAG_RESPONSE |
                             SL_SDO_CMDL_FLAG_EXPEDITED;
  rseq.sdoSeqPayload.commandId = g_mnReqCmd;
  rseq.sdoSeqPayload.aCommandData[0] = seq->sdoSeqPayload.aCommandData[0];
  rseq.sdoSeqPayload.aCommandData[1] = seq->sdoSeqPayload.aCommandData[1];
  rseq.sdoSeqPayload.aCommandData[2] = g_mnReqSub;

  if (g_mnReqCmd == SL_SDO_CID_READ_BY_INDEX) {
    rseq.sdoSeqPayload.flags |= 2;   /* padsize = 2，2 字节数据 */
    rseq.sdoSeqPayload.aCommandData[4] = (uint8_t)(g_simReadValue & 0xFF);
    rseq.sdoSeqPayload.aCommandData[5] = (uint8_t)(g_simReadValue >> 8);
  }

  memcpy(payload, &rseq, sizeof(rseq));
  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  rlen = sl_build_asnd(&resp, g_cnMac, mcast, CN_NODE, MN_NODE, SL_ASND_SDO,
                        payload, (uint16_t)(4 + sizeof(rseq)));
  return sl_edrv_loopback_inject(&g_mnPort, (const uint8_t*)&resp, rlen);
}

static int mnSdoPumpFn(uint8_t* frame, uint16_t maxLen, uint16_t* len,
                       uint32_t timeoutMs)
{
  (void)timeoutMs;
  return sl_edrv_loopback_pop(&g_mnPort, frame, maxLen, len);
}

static void test_sdo_client(void)
{
  static SlOd mnOd;
  uint16_t rval;
  uint16_t rsize;
  uint16_t wval;
  int ret;

  sl_od_init(&mnOd);
  sl_sdo_init(4, &mnOd);
  sl_sdo_set_local_node(MN_NODE);
  sl_sdo_set_io(mnSdoSendFn, mnSdoPumpFn);

  /* 读 */
  g_simReadValue = 0xABCD;
  rval = 0;
  rsize = sizeof(rval);
  ret = sl_sdo_read_req(CN_NODE, 0x2000, 0, &rval, &rsize, 1000);
  CHECK(ret == SL_ERR_OK, "SDO 客户端读成功");
  CHECK(rsize == 2 && rval == 0xABCD, "SDO 客户端读数据正确");

  /* 写 */
  wval = 0x5678;
  ret = sl_sdo_write_req(CN_NODE, 0x2000, 0, &wval, 2, 1000);
  CHECK(ret == SL_ERR_OK, "SDO 客户端写成功");
}

int main(void)
{
  printf("=== 智能总线 协议集成测试 ===\n");
  test_nmt_pdo_interop();
  test_sdo_server();
  test_sdo_client();

  if (g_failed == 0) {
    printf("=== 结果: 全部通过 ===\n");
    return 0;
  }
  printf("=== 结果: %d 项失败 ===\n", g_failed);
  return 1;
}
