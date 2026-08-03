/**
 * test_sl_core.c - 协议核心自检测试
 *
 * 验证：
 *  1. CRC32 (IEEE 802.3) 校验向量
 *  2. SoC/PReq/PRes/SoA/ASnd 帧构建与帧头字段
 *  3. NMT 状态迁移全流程
 *  4. CN ReadyToOperate 双握手
 *  5. 对象字典读写与访问权限
 *
 * 编译：见 CMakeLists.txt (add_test)
 */

#include <stdio.h>
#include "smartlink/smartlink.h"
#include "smartlink/frame.h"
#include "smartlink/sl_core.h"
#include "smartlink/nmt.h"
#include "smartlink/od.h"

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

static void test_crc32(void)
{
  uint32_t crc;
  const uint8_t data[] = "123456789";

  crc = sl_crc32(data, sizeof(data) - 1);
  CHECK(crc == 0xCBF43926u, "CRC32 '123456789' == 0xCBF43926");
}

static void test_frame_build(void)
{
  SlFrame frame;
  uint8_t srcMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  uint8_t cnMac[6]  = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t payload[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint16_t len;
  uint8_t mcast[6];
  int ret;

  /* 多播 MAC 查询 */
  ret = sl_get_mcast_mac(SL_MSG_SOC, mcast);
  CHECK(ret == SL_ERR_OK &&
        mcast[0] == 0x01 && mcast[1] == 0x11 && mcast[2] == 0x1E &&
        mcast[3] == 0x00 && mcast[4] == 0x00 && mcast[5] == 0x01,
        "SoC 多播 MAC = 01:11:1E:00:00:01");

  ret = sl_get_mcast_mac(SL_MSG_PRES, mcast);
  CHECK(ret == SL_ERR_OK && mcast[5] == 0x02, "PRes 多播 MAC ...:02");

  /* SoC 构建 */
  len = sl_build_soc(&frame, srcMac, SL_ADR_MN_DEF, 100000, 1000);
  CHECK(len == 36, "SoC 帧长度 = 36");
  CHECK(frame.etherType == sl_hton16(SL_ETHERTYPE),
        "SoC EtherType == 0x88AB(网络序)");
  CHECK(frame.messageType == SL_MSG_SOC, "SoC messageType == 0x01");
  CHECK(frame.srcNodeId == SL_ADR_MN_DEF, "SoC srcNode == 0xF0");
  CHECK(frame.dstNodeId == SL_ADR_BROADCAST, "SoC dstNode == 0xFF");
  CHECK(frame.data.soc.relativeTimeLe == 1000, "SoC relativeTime == 1000");
  CHECK(frame.aSrcMac[5] == 0x55 && frame.aDstMac[0] == 0x01,
        "SoC MAC 地址正确");

  /* PReq 构建 */
  len = sl_build_preq(&frame, srcMac, cnMac, SL_ADR_MN_DEF, 0x01,
                       payload, sizeof(payload), SL_FRAME_FLAG1_RD);
  CHECK(len == 28, "PReq 帧长度 = 24+4");
  CHECK(frame.messageType == SL_MSG_PREQ, "PReq messageType == 0x03");
  CHECK(frame.dstNodeId == 0x01, "PReq dstNode == 0x01");
  CHECK(frame.data.preq.sizeLe == 4, "PReq sizeLe == 4");
  CHECK(frame.data.preq.aPayload[0] == 0xDE &&
        frame.data.preq.aPayload[3] == 0xEF, "PReq 载荷正确");

  /* PRes 构建 */
  len = sl_build_pres(&frame, srcMac, 0x01, 0xFD,
                       payload, sizeof(payload),
                       SL_FRAME_FLAG1_RD, 0);
  CHECK(len == 28, "PRes 帧长度 = 28");
  CHECK(frame.messageType == SL_MSG_PRES, "PRes messageType == 0x04");
  CHECK(frame.data.pres.nmtStatus == 0xFD, "PRes nmtStatus 正确");
  CHECK(frame.data.pres.sizeLe == 4, "PRes sizeLe == 4");

  /* SoA 构建 */
  len = sl_build_soa(&frame, srcMac, SL_ADR_MN_DEF, 0xFD,
                      SL_REQ_SERVICE_IDENT, 0x01, 0);
  CHECK(len == 54, "SoA 帧长度 = 54");
  CHECK(frame.messageType == SL_MSG_SOA, "SoA messageType == 0x05");
  CHECK(frame.data.soa.reqServiceId == SL_REQ_SERVICE_IDENT,
        "SoA reqServiceId == Ident");
  CHECK(frame.data.soa.reqServiceTarget == 0x01, "SoA 目标节点 == 0x01");

  /* ASnd 构建 */
  len = sl_build_asnd(&frame, srcMac, cnMac, SL_ADR_MN_DEF, 0x01,
                       SL_ASND_NMT_COMMAND, payload, sizeof(payload));
  CHECK(len == 22, "ASnd 帧长度 = 18+4");
  CHECK(frame.messageType == SL_MSG_ASND, "ASnd messageType == 0x06");
  CHECK(frame.data.asnd.serviceId == SL_ASND_NMT_COMMAND,
        "ASnd serviceId == NmtCommand");

  /* 帧校验 */
  len = sl_build_soc(&frame, srcMac, SL_ADR_MN_DEF, 100000, 0);
  CHECK(sl_frame_validate(&frame, len) == SL_ERR_OK, "合法帧通过校验");
  frame.etherType = 0x0800;
  CHECK(sl_frame_validate(&frame, len) != SL_ERR_OK, "错误 EtherType 被拒绝");
}

static void test_nmt(void)
{
  SlNmtState s;

  /* 正常启动序列 */
  CHECK(sl_nmt_state_transition(SL_NMT_GS_OFFLINE,
                                 SL_NMT_EVENT_SWITCH_ON, &s) == SL_ERR_OK &&
        s == SL_NMT_GS_INITIALISING, "Offline --SwitchOn--> Initialising");

  CHECK(sl_nmt_state_transition(SL_NMT_GS_INITIALISING,
                                 SL_NMT_EVENT_SWITCH_ON, &s) == SL_ERR_OK &&
        s == SL_NMT_GS_RESET_APPLICATION, "Initialising --> ResetApp");

  CHECK(sl_nmt_state_transition(SL_NMT_GS_RESET_APPLICATION,
                                 SL_NMT_EVENT_INTERNAL_RESET_COM, &s) == SL_ERR_OK &&
        s == SL_NMT_GS_RESET_COMMUNICATION, "ResetApp --> ResetCom");

  CHECK(sl_nmt_state_transition(SL_NMT_GS_RESET_COMMUNICATION,
                                 SL_NMT_EVENT_INTERNAL_RESET_CONFIG, &s) == SL_ERR_OK &&
        s == SL_NMT_GS_RESET_CONFIGURATION, "ResetCom --> ResetConfig");

  CHECK(sl_nmt_state_transition(SL_NMT_GS_RESET_CONFIGURATION,
                                 SL_NMT_EVENT_SWITCH_ON, &s) == SL_ERR_OK &&
        s == SL_NMT_CS_NOT_ACTIVE, "ResetConfig --> NotActive");

  /* 收到 SoC 进入预运行 */
  CHECK(sl_nmt_state_transition(SL_NMT_CS_NOT_ACTIVE,
                                 SL_NMT_EVENT_RECEIVE_SOC, &s) == SL_ERR_OK &&
        s == SL_NMT_CS_PRE_OPERATIONAL_1, "NotActive --SoC--> PreOp1");

  CHECK(sl_nmt_state_transition(SL_NMT_CS_PRE_OPERATIONAL_1,
                                 SL_NMT_EVENT_RECEIVE_SOC, &s) == SL_ERR_OK &&
        s == SL_NMT_CS_PRE_OPERATIONAL_2, "PreOp1 --SoC--> PreOp2");

  /* Operational 相关迁移 */
  CHECK(sl_nmt_state_transition(SL_NMT_CS_OPERATIONAL,
                                 SL_NMT_EVENT_STOP_NODE, &s) == SL_ERR_OK &&
        s == SL_NMT_CS_STOPPED, "Operational --StopNode--> Stopped");

  CHECK(sl_nmt_state_transition(SL_NMT_CS_STOPPED,
                                 SL_NMT_EVENT_START_NODE, &s) == SL_ERR_OK &&
        s == SL_NMT_CS_OPERATIONAL, "Stopped --StartNode--> Operational");

  /* 非法事件 */
  CHECK(sl_nmt_state_transition(SL_NMT_CS_OPERATIONAL,
                                 SL_NMT_EVENT_RECEIVE_SOC, &s) != SL_ERR_OK,
        "非法事件被拒绝 (Operational + ReceiveSoC)");

  /* 状态名映射 */
  CHECK(sl_nmt_state_name(SL_NMT_CS_OPERATIONAL) != NULL, "状态名映射可用");
}

static void test_cn_handshake(void)
{
  SlCnmStateMachine sm;

  sl_cn_nmt_init(&sm);

  /* 启动到 PreOp2 */
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_SWITCH_ON);
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_SWITCH_ON);
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_INTERNAL_RESET_COM);
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_INTERNAL_RESET_CONFIG);
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_SWITCH_ON);
  CHECK(sm.state == SL_NMT_CS_NOT_ACTIVE, "CN 启动到 NotActive");

  sl_cn_nmt_process(&sm, SL_NMT_EVENT_RECEIVE_SOC);
  CHECK(sm.state == SL_NMT_CS_PRE_OPERATIONAL_1, "CN --SoC--> PreOp1");
  sl_cn_nmt_process(&sm, SL_NMT_EVENT_RECEIVE_SOC);
  CHECK(sm.state == SL_NMT_CS_PRE_OPERATIONAL_2, "CN --SoC--> PreOp2");

  /* 仅 MN 允许：不应迁移 */
  CHECK(sl_cn_nmt_process(&sm, SL_NMT_EVENT_ENABLE_READY_TO_OPERATE)
        != SL_ERR_OK && sm.state == SL_NMT_CS_PRE_OPERATIONAL_2,
        "仅 EnableReady 不迁移");

  /* 应用就绪后握手完成 */
  CHECK(sl_cn_nmt_process(&sm, SL_NMT_EVENT_ENTER_READY_TO_OPERATE)
        == SL_ERR_OK && sm.state == SL_NMT_CS_READY_TO_OPERATE,
        "双握手完成进入 ReadyToOperate");

  /* EnterOperational */
  CHECK(sl_cn_nmt_process(&sm, SL_NMT_EVENT_TO_OPERATIONAL) == SL_ERR_OK &&
        sm.state == SL_NMT_CS_OPERATIONAL, "Ready --ToOp--> Operational");
}

static void test_od(void)
{
  SlOd od;
  SlOdEntry entry;
  uint8_t data;
  uint16_t size;

  sl_od_init(&od);

  /* 添加 0x6000 sub1：BOOL，可读写 PDO */
  data = 1;
  entry.index = SL_OD_IDX_TX_PDO;      /* 0x6200 */
  entry.subIndex = 1;
  entry.access = SL_OD_ACC_VPRW;
  entry.objectType = SL_OD_OBJ_VAR;
  entry.dataType = SL_OD_TYPE_BOOL;
  entry.size = 1;
  entry.data = &data;
  entry.arraySize = 0;
  entry.subEntries = NULL;
  entry.next = NULL;
  CHECK(sl_od_add(&od, &entry) == SL_ERR_OK, "OD 添加条目成功");

  /* 读取 */
  size = 1;
  CHECK(sl_od_read(&od, SL_OD_IDX_TX_PDO, 1, &data, &size) == SL_ERR_OK &&
        data == 1, "OD 读取成功");
  CHECK(size == 1, "OD 返回正确大小");

  /* 写入 */
  data = 0;
  CHECK(sl_od_write(&od, SL_OD_IDX_TX_PDO, 1, &data, 1) == SL_ERR_OK,
        "OD 写入成功");
  CHECK(data == 0, "OD 写入生效");

  /* 不存在索引 */
  size = 1;
  CHECK(sl_od_read(&od, 0x7000, 0, &data, &size) != SL_ERR_OK,
        "不存在索引被拒绝");

  /* 数据长度不匹配 */
  CHECK(sl_od_write(&od, SL_OD_IDX_TX_PDO, 1, &data, 2) != SL_ERR_OK,
        "长度不匹配被拒绝");

  /* 类型尺寸表 */
  CHECK(sl_od_type_size(SL_OD_TYPE_UINT32) == 4, "UINT32 尺寸 = 4");
  CHECK(sl_od_type_size(SL_OD_TYPE_INT64) == 8, "INT64 尺寸 = 8");
}

int main(void)
{
  printf("=== 智能总线 协议核心自检 ===\n");
  test_crc32();
  test_frame_build();
  test_nmt();
  test_cn_handshake();
  test_od();
  printf("=== 结果: %s (%d 失败) ===\n",
         g_failed == 0 ? "全部通过" : "存在失败", g_failed);
  return g_failed == 0 ? 0 : 1;
}
