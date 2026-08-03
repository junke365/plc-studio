/**
 * nmt.h - NMT (Network Management) 状态机定义
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.3 章。
 * NMT 状态机是 智能总线 的核心：MN 通过 NMT 命令驱动 CN 在各状态间迁移，
 * CN 在状态迁移中完成配置下载、周期建立、应用就绪等步骤。
 *
 * 状态迁移双标志握手：
 *   ReadyToOperate 需要应用调用 SL_NMT_EVENT_ENTER_READY_TO_OPERATE
 *   且 MN 发送 EnableReadyToOperate (SL_NMT_EVENT_ENABLE_READY_TO_OPERATE) 同时成立。
 */

#ifndef SL_NMT_H
#define SL_NMT_H

#include "smartlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== NMT 状态码 ========== */

typedef enum
{
  SL_NMT_GS_OFFLINE             = 0x0100,  /* 离线（无通信） */
  SL_NMT_GS_INITIALISING        = 0x0120,  /* 初始化中 */
  SL_NMT_GS_RESET_APPLICATION   = 0x0104,  /* 应用复位 */
  SL_NMT_GS_RESET_COMMUNICATION = 0x0110,  /* 通信复位 */
  SL_NMT_GS_RESET_CONFIGURATION = 0x0112,  /* 配置复位 */
  SL_NMT_CS_NOT_ACTIVE          = 0x011C,  /* 未激活（等待 SoC/SoA） */
  SL_NMT_CS_BASIC_ETHERNET      = 0x011E,  /* 基本以太网 */
  SL_NMT_CS_PRE_OPERATIONAL_1   = 0x011D,  /* 预运行 1（无周期数据） */
  SL_NMT_CS_PRE_OPERATIONAL_2   = 0x015D,  /* 预运行 2（有周期数据） */
  SL_NMT_CS_READY_TO_OPERATE    = 0x016D,  /* 就绪（同步建立，无 PDO 交换） */
  SL_NMT_CS_OPERATIONAL         = 0x01FD,  /* 运行（全通信） */
  SL_NMT_CS_STOPPED             = 0x014D,  /* 停止（周期通信挂起） */
} SlNmtState;

/* ========== NMT 事件（位标志，可组合） ========== */

typedef enum
{
  SL_NMT_EVENT_SW_RESET                = 0x00000000,  /* 软件复位 */
  SL_NMT_EVENT_START_UP                = 0x00000001,  /* 上电启动 */
  SL_NMT_EVENT_SWITCH_OFF              = 0x00000002,  /* 断电 */
  SL_NMT_EVENT_SWITCH_ON               = 0x00000003,  /* 上电 */
  SL_NMT_EVENT_TO_PRE_OPERATIONAL_1    = 0x00000004,
  SL_NMT_EVENT_TO_PRE_OPERATIONAL_2    = 0x00000005,
  SL_NMT_EVENT_TO_READY_TO_OPERATE     = 0x00000006,
  SL_NMT_EVENT_TO_OPERATIONAL          = 0x00000007,
  SL_NMT_EVENT_TO_STOPPED              = 0x00000008,
  SL_NMT_EVENT_REQUEST_RESET_APP       = 0x00000010,
  SL_NMT_EVENT_REQUEST_RESET_COM       = 0x00000020,
  SL_NMT_EVENT_REQUEST_RESET_CONFIG    = 0x00000040,
  SL_NMT_EVENT_ENTER_READY_TO_OPERATE  = 0x00000080,  /* 应用主动就绪 */
  SL_NMT_EVENT_STOP_NODE               = 0x00000100,
  SL_NMT_EVENT_INTERNAL_RESET_APP      = 0x00000200,
  SL_NMT_EVENT_INTERNAL_RESET_COM      = 0x00000400,
  SL_NMT_EVENT_INTERNAL_RESET_CONFIG   = 0x00000800,
  SL_NMT_EVENT_ENTER_OPERATIONAL       = 0x00001000,
  SL_NMT_EVENT_NODE_ERROR              = 0x00002000,
  SL_NMT_EVENT_CRITICAL_ERROR          = 0x00004000,
  SL_NMT_EVENT_ERROR_RESET             = 0x00008000,
  SL_NMT_EVENT_RECEIVE_SOC             = 0x00010000,  /* 收到 SoC（周期同步） */
  SL_NMT_EVENT_RECEIVE_SOA             = 0x00020000,  /* 收到 SoA */
  SL_NMT_EVENT_ENABLE_READY_TO_OPERATE = 0x00040000,  /* MN 允许就绪 */
  SL_NMT_EVENT_MN_ENTER_OPERATIONAL    = 0x00100000,
  SL_NMT_EVENT_START_NODE              = 0x00400000,
  SL_NMT_EVENT_RESYNC                  = 0x00800000,
  SL_NMT_EVENT_DISABLE_READY_TO_OPERATE = 0x04000000,
  SL_NMT_EVENT_DISABLE_OPERATIONAL     = 0x08000000,
} SlNmtEvent;

/* ========== NMT 命令 ID ========== */

typedef enum
{
  SL_NMT_CMD_SW_RESET                 = 0x01,  /* 软件复位 */
  SL_NMT_CMD_START_UP                 = 0x02,  /* 启动 */
  SL_NMT_CMD_STOP                     = 0x03,  /* 停止 */
  SL_NMT_CMD_ENTER_PRE_OPERATIONAL_1  = 0x04,  /* 进入预运行 1 */
  SL_NMT_CMD_ENTER_PRE_OPERATIONAL_2  = 0x05,  /* 进入预运行 2 */
  SL_NMT_CMD_START_NODE               = 0x06,  /* 启动节点 */
  SL_NMT_CMD_STOP_NODE                = 0x07,  /* 停止节点 */
  SL_NMT_CMD_ENTER_READY_TO_OPERATE   = 0x08,  /* 进入就绪 */
  SL_NMT_CMD_ENTER_OPERATIONAL        = 0x09,  /* 进入运行 */
  SL_NMT_CMD_RESYNC                   = 0x0A,  /* 重新同步 */
} SlNmtCommandId;

/* ========== CN 状态机封装（双握手） ========== */

/**
 * CN 状态机上下文。
 * ReadyToOperate 迁移需要双标志同时成立：
 *   - mnReadyToOperate：MN 已发送 EnableReadyToOperate
 *   - appReadyToOperate：应用已调用 EnterReadyToOperate
 */
typedef struct
{
  SlNmtState state;             /* 当前状态 */
  bool mnReadyToOperate;         /* MN EnableReadyToOperate 已收到 */
  bool appReadyToOperate;        /* 应用 EnterReadyToOperate 已调用 */
} SlCnmStateMachine;

/* ========== 接口 ========== */

/**
 * 状态迁移函数。
 * @param state  当前状态
 * @param event  触发事件
 * @param pNewState 输出新状态
 * @return 0 迁移成功；负值表示事件在当前状态不允许
 */
int sl_nmt_state_transition(SlNmtState state, SlNmtEvent event,
                             SlNmtState* pNewState);

/**
 * 初始化 CN 状态机。
 */
void sl_cn_nmt_init(SlCnmStateMachine* sm);

/**
 * 向 CN 状态机投递事件。
 * 在 PreOperational_2 中处理 ReadyToOperate 双标志握手。
 * @return 0 状态迁移或标志更新成功；SL_ERR_NMT_STATE 表示等待另一侧握手/事件无效
 */
int sl_cn_nmt_process(SlCnmStateMachine* sm, SlNmtEvent event);

/**
 * 将 NMT 状态码转为可读字符串。
 */
const char* sl_nmt_state_name(SlNmtState state);

/**
 * 将 NMT 事件转为可读字符串。
 */
const char* sl_nmt_event_name(SlNmtEvent event);

/**
 * 将 NMT 命令 ID 转为可读字符串。
 */
const char* sl_nmt_cmd_name(SlNmtCommandId cmd);

#ifdef __cplusplus
}
#endif

#endif /* SL_NMT_H */
