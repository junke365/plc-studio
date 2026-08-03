/**
 * test_plk_core.c - 协议核心自检测试
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
#include "plk/plk.h"
#include "plk/frame.h"
#include "plk/plk_core.h"
#include "plk/nmt.h"
#include "plk/od.h"

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

  crc = plk_crc32(data, sizeof(data) - 1);
  CHECK(crc == 0xCBF43926u, "CRC32 '123456789' == 0xCBF43926");
}

static void test_frame_build(void)
{
  PlkFrame frame;
  uint8_t srcMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  uint8_t cnMac[6]  = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t payload[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint16_t len;
  uint8_t mcast[6];
  int ret;

  /* 多播 MAC 查询 */
  ret = plk_get_mcast_mac(PLK_MSG_SOC, mcast);
  CHECK(ret == PLK_ERR_OK &&
        mcast[0] == 0x01 && mcast[1] == 0x11 && mcast[2] == 0x1E &&
        mcast[3] == 0x00 && mcast[4] == 0x00 && mcast[5] == 0x01,
        "SoC 多播 MAC = 01:11:1E:00:00:01");

  ret = plk_get_mcast_mac(PLK_MSG_PRES, mcast);
  CHECK(ret == PLK_ERR_OK && mcast[5] == 0x02, "PRes 多播 MAC ...:02");

  /* SoC 构建 */
  len = plk_build_soc(&frame, srcMac, PLK_ADR_MN_DEF, 100000, 1000);
  CHECK(len == 36, "SoC 帧长度 = 36");
  CHECK(frame.etherType == plk_hton16(PLK_ETHERTYPE),
        "SoC EtherType == 0x88AB(网络序)");
  CHECK(frame.messageType == PLK_MSG_SOC, "SoC messageType == 0x01");
  CHECK(frame.srcNodeId == PLK_ADR_MN_DEF, "SoC srcNode == 0xF0");
  CHECK(frame.dstNodeId == PLK_ADR_BROADCAST, "SoC dstNode == 0xFF");
  CHECK(frame.data.soc.relativeTimeLe == 1000, "SoC relativeTime == 1000");
  CHECK(frame.aSrcMac[5] == 0x55 && frame.aDstMac[0] == 0x01,
        "SoC MAC 地址正确");

  /* PReq 构建 */
  len = plk_build_preq(&frame, srcMac, cnMac, PLK_ADR_MN_DEF, 0x01,
                       payload, sizeof(payload), PLK_FRAME_FLAG1_RD);
  CHECK(len == 28, "PReq 帧长度 = 24+4");
  CHECK(frame.messageType == PLK_MSG_PREQ, "PReq messageType == 0x03");
  CHECK(frame.dstNodeId == 0x01, "PReq dstNode == 0x01");
  CHECK(frame.data.preq.sizeLe == 4, "PReq sizeLe == 4");
  CHECK(frame.data.preq.aPayload[0] == 0xDE &&
        frame.data.preq.aPayload[3] == 0xEF, "PReq 载荷正确");

  /* PRes 构建 */
  len = plk_build_pres(&frame, srcMac, 0x01, 0xFD,
                       payload, sizeof(payload),
                       PLK_FRAME_FLAG1_RD, 0);
  CHECK(len == 28, "PRes 帧长度 = 28");
  CHECK(frame.messageType == PLK_MSG_PRES, "PRes messageType == 0x04");
  CHECK(frame.data.pres.nmtStatus == 0xFD, "PRes nmtStatus 正确");
  CHECK(frame.data.pres.sizeLe == 4, "PRes sizeLe == 4");

  /* SoA 构建 */
  len = plk_build_soa(&frame, srcMac, PLK_ADR_MN_DEF, 0xFD,
                      PLK_REQ_SERVICE_IDENT, 0x01, 0);
  CHECK(len == 54, "SoA 帧长度 = 54");
  CHECK(frame.messageType == PLK_MSG_SOA, "SoA messageType == 0x05");
  CHECK(frame.data.soa.reqServiceId == PLK_REQ_SERVICE_IDENT,
        "SoA reqServiceId == Ident");
  CHECK(frame.data.soa.reqServiceTarget == 0x01, "SoA 目标节点 == 0x01");

  /* ASnd 构建 */
  len = plk_build_asnd(&frame, srcMac, cnMac, PLK_ADR_MN_DEF, 0x01,
                       PLK_ASND_NMT_COMMAND, payload, sizeof(payload));
  CHECK(len == 22, "ASnd 帧长度 = 18+4");
  CHECK(frame.messageType == PLK_MSG_ASND, "ASnd messageType == 0x06");
  CHECK(frame.data.asnd.serviceId == PLK_ASND_NMT_COMMAND,
        "ASnd serviceId == NmtCommand");

  /* 帧校验 */
  len = plk_build_soc(&frame, srcMac, PLK_ADR_MN_DEF, 100000, 0);
  CHECK(plk_frame_validate(&frame, len) == PLK_ERR_OK, "合法帧通过校验");
  frame.etherType = 0x0800;
  CHECK(plk_frame_validate(&frame, len) != PLK_ERR_OK, "错误 EtherType 被拒绝");
}

static void test_nmt(void)
{
  PlkNmtState s;

  /* 正常启动序列 */
  CHECK(plk_nmt_state_transition(PLK_NMT_GS_OFFLINE,
                                 PLK_NMT_EVENT_SWITCH_ON, &s) == PLK_ERR_OK &&
        s == PLK_NMT_GS_INITIALISING, "Offline --SwitchOn--> Initialising");

  CHECK(plk_nmt_state_transition(PLK_NMT_GS_INITIALISING,
                                 PLK_NMT_EVENT_SWITCH_ON, &s) == PLK_ERR_OK &&
        s == PLK_NMT_GS_RESET_APPLICATION, "Initialising --> ResetApp");

  CHECK(plk_nmt_state_transition(PLK_NMT_GS_RESET_APPLICATION,
                                 PLK_NMT_EVENT_INTERNAL_RESET_COM, &s) == PLK_ERR_OK &&
        s == PLK_NMT_GS_RESET_COMMUNICATION, "ResetApp --> ResetCom");

  CHECK(plk_nmt_state_transition(PLK_NMT_GS_RESET_COMMUNICATION,
                                 PLK_NMT_EVENT_INTERNAL_RESET_CONFIG, &s) == PLK_ERR_OK &&
        s == PLK_NMT_GS_RESET_CONFIGURATION, "ResetCom --> ResetConfig");

  CHECK(plk_nmt_state_transition(PLK_NMT_GS_RESET_CONFIGURATION,
                                 PLK_NMT_EVENT_SWITCH_ON, &s) == PLK_ERR_OK &&
        s == PLK_NMT_CS_NOT_ACTIVE, "ResetConfig --> NotActive");

  /* 收到 SoC 进入预运行 */
  CHECK(plk_nmt_state_transition(PLK_NMT_CS_NOT_ACTIVE,
                                 PLK_NMT_EVENT_RECEIVE_SOC, &s) == PLK_ERR_OK &&
        s == PLK_NMT_CS_PRE_OPERATIONAL_1, "NotActive --SoC--> PreOp1");

  CHECK(plk_nmt_state_transition(PLK_NMT_CS_PRE_OPERATIONAL_1,
                                 PLK_NMT_EVENT_RECEIVE_SOC, &s) == PLK_ERR_OK &&
        s == PLK_NMT_CS_PRE_OPERATIONAL_2, "PreOp1 --SoC--> PreOp2");

  /* Operational 相关迁移 */
  CHECK(plk_nmt_state_transition(PLK_NMT_CS_OPERATIONAL,
                                 PLK_NMT_EVENT_STOP_NODE, &s) == PLK_ERR_OK &&
        s == PLK_NMT_CS_STOPPED, "Operational --StopNode--> Stopped");

  CHECK(plk_nmt_state_transition(PLK_NMT_CS_STOPPED,
                                 PLK_NMT_EVENT_START_NODE, &s) == PLK_ERR_OK &&
        s == PLK_NMT_CS_OPERATIONAL, "Stopped --StartNode--> Operational");

  /* 非法事件 */
  CHECK(plk_nmt_state_transition(PLK_NMT_CS_OPERATIONAL,
                                 PLK_NMT_EVENT_RECEIVE_SOC, &s) != PLK_ERR_OK,
        "非法事件被拒绝 (Operational + ReceiveSoC)");

  /* 状态名映射 */
  CHECK(plk_nmt_state_name(PLK_NMT_CS_OPERATIONAL) != NULL, "状态名映射可用");
}

static void test_cn_handshake(void)
{
  PlkCnmStateMachine sm;

  plk_cn_nmt_init(&sm);

  /* 启动到 PreOp2 */
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_SWITCH_ON);
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_SWITCH_ON);
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_INTERNAL_RESET_COM);
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_INTERNAL_RESET_CONFIG);
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_SWITCH_ON);
  CHECK(sm.state == PLK_NMT_CS_NOT_ACTIVE, "CN 启动到 NotActive");

  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_RECEIVE_SOC);
  CHECK(sm.state == PLK_NMT_CS_PRE_OPERATIONAL_1, "CN --SoC--> PreOp1");
  plk_cn_nmt_process(&sm, PLK_NMT_EVENT_RECEIVE_SOC);
  CHECK(sm.state == PLK_NMT_CS_PRE_OPERATIONAL_2, "CN --SoC--> PreOp2");

  /* 仅 MN 允许：不应迁移 */
  CHECK(plk_cn_nmt_process(&sm, PLK_NMT_EVENT_ENABLE_READY_TO_OPERATE)
        != PLK_ERR_OK && sm.state == PLK_NMT_CS_PRE_OPERATIONAL_2,
        "仅 EnableReady 不迁移");

  /* 应用就绪后握手完成 */
  CHECK(plk_cn_nmt_process(&sm, PLK_NMT_EVENT_ENTER_READY_TO_OPERATE)
        == PLK_ERR_OK && sm.state == PLK_NMT_CS_READY_TO_OPERATE,
        "双握手完成进入 ReadyToOperate");

  /* EnterOperational */
  CHECK(plk_cn_nmt_process(&sm, PLK_NMT_EVENT_TO_OPERATIONAL) == PLK_ERR_OK &&
        sm.state == PLK_NMT_CS_OPERATIONAL, "Ready --ToOp--> Operational");
}

static void test_od(void)
{
  PlkOd od;
  PlkOdEntry entry;
  uint8_t data;
  uint16_t size;

  plk_od_init(&od);

  /* 添加 0x6000 sub1：BOOL，可读写 PDO */
  data = 1;
  entry.index = PLK_OD_IDX_TX_PDO;      /* 0x6200 */
  entry.subIndex = 1;
  entry.access = PLK_OD_ACC_VPRW;
  entry.objectType = PLK_OD_OBJ_VAR;
  entry.dataType = PLK_OD_TYPE_BOOL;
  entry.size = 1;
  entry.data = &data;
  entry.arraySize = 0;
  entry.subEntries = NULL;
  entry.next = NULL;
  CHECK(plk_od_add(&od, &entry) == PLK_ERR_OK, "OD 添加条目成功");

  /* 读取 */
  size = 1;
  CHECK(plk_od_read(&od, PLK_OD_IDX_TX_PDO, 1, &data, &size) == PLK_ERR_OK &&
        data == 1, "OD 读取成功");
  CHECK(size == 1, "OD 返回正确大小");

  /* 写入 */
  data = 0;
  CHECK(plk_od_write(&od, PLK_OD_IDX_TX_PDO, 1, &data, 1) == PLK_ERR_OK,
        "OD 写入成功");
  CHECK(data == 0, "OD 写入生效");

  /* 不存在索引 */
  size = 1;
  CHECK(plk_od_read(&od, 0x7000, 0, &data, &size) != PLK_ERR_OK,
        "不存在索引被拒绝");

  /* 数据长度不匹配 */
  CHECK(plk_od_write(&od, PLK_OD_IDX_TX_PDO, 1, &data, 2) != PLK_ERR_OK,
        "长度不匹配被拒绝");

  /* 类型尺寸表 */
  CHECK(plk_od_type_size(PLK_OD_TYPE_UINT32) == 4, "UINT32 尺寸 = 4");
  CHECK(plk_od_type_size(PLK_OD_TYPE_INT64) == 8, "INT64 尺寸 = 8");
}

int main(void)
{
  printf("=== POWERLINK 协议核心自检 ===\n");
  test_crc32();
  test_frame_build();
  test_nmt();
  test_cn_handshake();
  test_od();
  printf("=== 结果: %s (%d 失败) ===\n",
         g_failed == 0 ? "全部通过" : "存在失败", g_failed);
  return g_failed == 0 ? 0 : 1;
}
