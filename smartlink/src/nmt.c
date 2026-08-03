/**
 * nmt.c - NMT 状态机实现
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.3 章。
 * 提供两个层次：
 *   1. sl_nmt_state_transition() - 无状态迁移表（单事件驱动）
 *   2. SlCnmStateMachine          - CN 状态机封装，处理 ReadyToOperate
 *      双标志握手（应用 EnterReadyToOperate + MN EnableReadyToOperate）
 */

#include "smartlink/nmt.h"

/* ========== 纯状态迁移表 ========== */

static SlNmtState state_transition(SlNmtState state, SlNmtEvent event);

int sl_nmt_state_transition(SlNmtState state, SlNmtEvent event,
                             SlNmtState* pNewState)
{
  SlNmtState newState;

  if (pNewState == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  newState = state_transition(state, event);
  if (newState == state) {
    /* 无迁移不代表错误：很多事件在目标状态是合法但无动作的 */
    *pNewState = state;
    return SL_ERR_NMT_STATE;
  }

  *pNewState = newState;
  return SL_ERR_OK;
}

static SlNmtState state_transition(SlNmtState state, SlNmtEvent event)
{
  switch (state) {
    case SL_NMT_GS_OFFLINE:
      switch (event) {
        case SL_NMT_EVENT_SW_RESET:
        case SL_NMT_EVENT_START_UP:
        case SL_NMT_EVENT_SWITCH_ON:
        case SL_NMT_EVENT_REQUEST_RESET_APP:
        case SL_NMT_EVENT_REQUEST_RESET_COM:
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_GS_INITIALISING:
      switch (event) {
        case SL_NMT_EVENT_SWITCH_ON:       /* 内部初始化完成 */
        case SL_NMT_EVENT_INTERNAL_RESET_APP:
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        default:
          return state;
      }

    case SL_NMT_GS_RESET_APPLICATION:
      switch (event) {
        case SL_NMT_EVENT_INTERNAL_RESET_COM:
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        default:
          return state;
      }

    case SL_NMT_GS_RESET_COMMUNICATION:
      switch (event) {
        case SL_NMT_EVENT_INTERNAL_RESET_CONFIG:
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        default:
          return state;
      }

    case SL_NMT_GS_RESET_CONFIGURATION:
      switch (event) {
        case SL_NMT_EVENT_INTERNAL_RESET_APP:
        case SL_NMT_EVENT_SWITCH_ON:       /* 配置复位完成 */
          return SL_NMT_CS_NOT_ACTIVE;
        default:
          return state;
      }

    case SL_NMT_CS_NOT_ACTIVE:
      switch (event) {
        case SL_NMT_EVENT_RECEIVE_SOC:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_RECEIVE_SOA:
          return SL_NMT_CS_BASIC_ETHERNET;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_BASIC_ETHERNET:
      switch (event) {
        case SL_NMT_EVENT_RECEIVE_SOC:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_PRE_OPERATIONAL_1:
      switch (event) {
        case SL_NMT_EVENT_RECEIVE_SOC:
          return SL_NMT_CS_PRE_OPERATIONAL_2;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return SL_NMT_CS_PRE_OPERATIONAL_2;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_PRE_OPERATIONAL_2:
      switch (event) {
        /* 双握手事件：见 SlCnmStateMachine，此处仅处理 MN 已允许的情况 */
        case SL_NMT_EVENT_ENTER_READY_TO_OPERATE:
          return SL_NMT_CS_READY_TO_OPERATE;
        case SL_NMT_EVENT_TO_OPERATIONAL:
        case SL_NMT_EVENT_START_NODE:
          return SL_NMT_CS_OPERATIONAL;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_READY_TO_OPERATE:
      switch (event) {
        case SL_NMT_EVENT_TO_OPERATIONAL:
        case SL_NMT_EVENT_START_NODE:
          return SL_NMT_CS_OPERATIONAL;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return SL_NMT_CS_PRE_OPERATIONAL_2;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_STOP_NODE:
          return SL_NMT_CS_STOPPED;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_OPERATIONAL:
      switch (event) {
        case SL_NMT_EVENT_STOP_NODE:
          return SL_NMT_CS_STOPPED;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return SL_NMT_CS_PRE_OPERATIONAL_2;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_TO_READY_TO_OPERATE:
          return SL_NMT_CS_READY_TO_OPERATE;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case SL_NMT_CS_STOPPED:
      switch (event) {
        case SL_NMT_EVENT_START_NODE:
        case SL_NMT_EVENT_RESYNC:
          return SL_NMT_CS_OPERATIONAL;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return SL_NMT_CS_PRE_OPERATIONAL_2;
        case SL_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return SL_NMT_CS_PRE_OPERATIONAL_1;
        case SL_NMT_EVENT_TO_READY_TO_OPERATE:
          return SL_NMT_CS_READY_TO_OPERATE;
        case SL_NMT_EVENT_REQUEST_RESET_APP:
          return SL_NMT_GS_RESET_APPLICATION;
        case SL_NMT_EVENT_REQUEST_RESET_COM:
          return SL_NMT_GS_RESET_COMMUNICATION;
        case SL_NMT_EVENT_REQUEST_RESET_CONFIG:
          return SL_NMT_GS_RESET_CONFIGURATION;
        case SL_NMT_EVENT_SW_RESET:
          return SL_NMT_GS_INITIALISING;
        default:
          return state;
      }

    default:
      return state;
  }
}

/* ========== CN 状态机封装（双握手） ========== */

void sl_cn_nmt_init(SlCnmStateMachine* sm)
{
  sm->state = SL_NMT_GS_OFFLINE;
  sm->mnReadyToOperate = false;
  sm->appReadyToOperate = false;
}

int sl_cn_nmt_process(SlCnmStateMachine* sm, SlNmtEvent event)
{
  SlNmtState newState;

  if (sm == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* 跟踪双握手标志 */
  if (event == SL_NMT_EVENT_ENABLE_READY_TO_OPERATE) {
    sm->mnReadyToOperate = true;
  }
  if (event == SL_NMT_EVENT_ENTER_READY_TO_OPERATE) {
    sm->appReadyToOperate = true;
  }

  /* PreOperational_2 的 ReadyToOperate 迁移需要双标志 */
  if (sm->state == SL_NMT_CS_PRE_OPERATIONAL_2 &&
      (event == SL_NMT_EVENT_ENABLE_READY_TO_OPERATE ||
       event == SL_NMT_EVENT_ENTER_READY_TO_OPERATE)) {
    if (sm->mnReadyToOperate && sm->appReadyToOperate) {
      sm->state = SL_NMT_CS_READY_TO_OPERATE;
      return SL_ERR_OK;
    }
    return SL_ERR_NMT_STATE;   /* 等待另一侧握手 */
  }

  newState = state_transition(sm->state, event);
  if (newState != sm->state) {
    sm->state = newState;
    /* 离开 PreOp2 或复位时清除握手标志 */
    if (sm->state == SL_NMT_GS_INITIALISING ||
        sm->state == SL_NMT_GS_RESET_APPLICATION ||
        sm->state == SL_NMT_GS_RESET_COMMUNICATION ||
        sm->state == SL_NMT_GS_RESET_CONFIGURATION) {
      sm->mnReadyToOperate = false;
      sm->appReadyToOperate = false;
    }
    return SL_ERR_OK;
  }

  return SL_ERR_NMT_STATE;
}

/* ========== 名称映射 ========== */

const char* sl_nmt_state_name(SlNmtState state)
{
  switch (state) {
    case SL_NMT_GS_OFFLINE:             return "GS_Offline";
    case SL_NMT_GS_INITIALISING:        return "GS_Initialising";
    case SL_NMT_GS_RESET_APPLICATION:   return "GS_ResetApplication";
    case SL_NMT_GS_RESET_COMMUNICATION: return "GS_ResetCommunication";
    case SL_NMT_GS_RESET_CONFIGURATION: return "GS_ResetConfiguration";
    case SL_NMT_CS_NOT_ACTIVE:          return "CS_NotActive";
    case SL_NMT_CS_BASIC_ETHERNET:      return "CS_BasicEthernet";
    case SL_NMT_CS_PRE_OPERATIONAL_1:   return "CS_PreOperational1";
    case SL_NMT_CS_PRE_OPERATIONAL_2:   return "CS_PreOperational2";
    case SL_NMT_CS_READY_TO_OPERATE:    return "CS_ReadyToOperate";
    case SL_NMT_CS_OPERATIONAL:         return "CS_Operational";
    case SL_NMT_CS_STOPPED:             return "CS_Stopped";
    default:                             return "Unknown";
  }
}

const char* sl_nmt_event_name(SlNmtEvent event)
{
  switch (event) {
    case SL_NMT_EVENT_SW_RESET:                  return "SwReset";
    case SL_NMT_EVENT_START_UP:                  return "StartUp";
    case SL_NMT_EVENT_SWITCH_OFF:                return "SwitchOff";
    case SL_NMT_EVENT_SWITCH_ON:                 return "SwitchOn";
    case SL_NMT_EVENT_TO_PRE_OPERATIONAL_1:      return "ToPreOperational1";
    case SL_NMT_EVENT_TO_PRE_OPERATIONAL_2:      return "ToPreOperational2";
    case SL_NMT_EVENT_TO_READY_TO_OPERATE:       return "ToReadyToOperate";
    case SL_NMT_EVENT_TO_OPERATIONAL:            return "ToOperational";
    case SL_NMT_EVENT_TO_STOPPED:                return "ToStopped";
    case SL_NMT_EVENT_REQUEST_RESET_APP:         return "RequestResetApp";
    case SL_NMT_EVENT_REQUEST_RESET_COM:         return "RequestResetCom";
    case SL_NMT_EVENT_REQUEST_RESET_CONFIG:      return "RequestResetConfig";
    case SL_NMT_EVENT_ENTER_READY_TO_OPERATE:    return "EnterReadyToOperate";
    case SL_NMT_EVENT_STOP_NODE:                 return "StopNode";
    case SL_NMT_EVENT_INTERNAL_RESET_APP:        return "InternalResetApp";
    case SL_NMT_EVENT_INTERNAL_RESET_COM:        return "InternalResetCom";
    case SL_NMT_EVENT_INTERNAL_RESET_CONFIG:     return "InternalResetConfig";
    case SL_NMT_EVENT_ENTER_OPERATIONAL:         return "EnterOperational";
    case SL_NMT_EVENT_NODE_ERROR:                return "NodeError";
    case SL_NMT_EVENT_CRITICAL_ERROR:            return "CriticalError";
    case SL_NMT_EVENT_ERROR_RESET:               return "ErrorReset";
    case SL_NMT_EVENT_RECEIVE_SOC:               return "ReceiveSoC";
    case SL_NMT_EVENT_RECEIVE_SOA:               return "ReceiveSoA";
    case SL_NMT_EVENT_ENABLE_READY_TO_OPERATE:   return "EnableReadyToOperate";
    case SL_NMT_EVENT_MN_ENTER_OPERATIONAL:      return "MNEnterOperational";
    case SL_NMT_EVENT_START_NODE:                return "StartNode";
    case SL_NMT_EVENT_RESYNC:                    return "Resync";
    case SL_NMT_EVENT_DISABLE_READY_TO_OPERATE:  return "DisableReadyToOperate";
    case SL_NMT_EVENT_DISABLE_OPERATIONAL:       return "DisableOperational";
    default:                                      return "Unknown";
  }
}

const char* sl_nmt_cmd_name(SlNmtCommandId cmd)
{
  switch (cmd) {
    case SL_NMT_CMD_SW_RESET:                return "SwReset";
    case SL_NMT_CMD_START_UP:                return "StartUp";
    case SL_NMT_CMD_STOP:                    return "Stop";
    case SL_NMT_CMD_ENTER_PRE_OPERATIONAL_1: return "EnterPreOperational1";
    case SL_NMT_CMD_ENTER_PRE_OPERATIONAL_2: return "EnterPreOperational2";
    case SL_NMT_CMD_START_NODE:              return "StartNode";
    case SL_NMT_CMD_STOP_NODE:               return "StopNode";
    case SL_NMT_CMD_ENTER_READY_TO_OPERATE:  return "EnterReadyToOperate";
    case SL_NMT_CMD_ENTER_OPERATIONAL:       return "EnterOperational";
    case SL_NMT_CMD_RESYNC:                  return "Resync";
    default:                                  return "Unknown";
  }
}
