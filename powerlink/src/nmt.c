/**
 * nmt.c - NMT 状态机实现
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.3 章。
 * 提供两个层次：
 *   1. plk_nmt_state_transition() - 无状态迁移表（单事件驱动）
 *   2. PlkCnmStateMachine          - CN 状态机封装，处理 ReadyToOperate
 *      双标志握手（应用 EnterReadyToOperate + MN EnableReadyToOperate）
 */

#include "plk/nmt.h"

/* ========== 纯状态迁移表 ========== */

static PlkNmtState state_transition(PlkNmtState state, PlkNmtEvent event);

int plk_nmt_state_transition(PlkNmtState state, PlkNmtEvent event,
                             PlkNmtState* pNewState)
{
  PlkNmtState newState;

  if (pNewState == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  newState = state_transition(state, event);
  if (newState == state) {
    /* 无迁移不代表错误：很多事件在目标状态是合法但无动作的 */
    *pNewState = state;
    return PLK_ERR_NMT_STATE;
  }

  *pNewState = newState;
  return PLK_ERR_OK;
}

static PlkNmtState state_transition(PlkNmtState state, PlkNmtEvent event)
{
  switch (state) {
    case PLK_NMT_GS_OFFLINE:
      switch (event) {
        case PLK_NMT_EVENT_SW_RESET:
        case PLK_NMT_EVENT_START_UP:
        case PLK_NMT_EVENT_SWITCH_ON:
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_GS_INITIALISING:
      switch (event) {
        case PLK_NMT_EVENT_SWITCH_ON:       /* 内部初始化完成 */
        case PLK_NMT_EVENT_INTERNAL_RESET_APP:
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        default:
          return state;
      }

    case PLK_NMT_GS_RESET_APPLICATION:
      switch (event) {
        case PLK_NMT_EVENT_INTERNAL_RESET_COM:
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        default:
          return state;
      }

    case PLK_NMT_GS_RESET_COMMUNICATION:
      switch (event) {
        case PLK_NMT_EVENT_INTERNAL_RESET_CONFIG:
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        default:
          return state;
      }

    case PLK_NMT_GS_RESET_CONFIGURATION:
      switch (event) {
        case PLK_NMT_EVENT_INTERNAL_RESET_APP:
        case PLK_NMT_EVENT_SWITCH_ON:       /* 配置复位完成 */
          return PLK_NMT_CS_NOT_ACTIVE;
        default:
          return state;
      }

    case PLK_NMT_CS_NOT_ACTIVE:
      switch (event) {
        case PLK_NMT_EVENT_RECEIVE_SOC:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_RECEIVE_SOA:
          return PLK_NMT_CS_BASIC_ETHERNET;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_BASIC_ETHERNET:
      switch (event) {
        case PLK_NMT_EVENT_RECEIVE_SOC:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_PRE_OPERATIONAL_1:
      switch (event) {
        case PLK_NMT_EVENT_RECEIVE_SOC:
          return PLK_NMT_CS_PRE_OPERATIONAL_2;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return PLK_NMT_CS_PRE_OPERATIONAL_2;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_PRE_OPERATIONAL_2:
      switch (event) {
        /* 双握手事件：见 PlkCnmStateMachine，此处仅处理 MN 已允许的情况 */
        case PLK_NMT_EVENT_ENTER_READY_TO_OPERATE:
          return PLK_NMT_CS_READY_TO_OPERATE;
        case PLK_NMT_EVENT_TO_OPERATIONAL:
        case PLK_NMT_EVENT_START_NODE:
          return PLK_NMT_CS_OPERATIONAL;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_READY_TO_OPERATE:
      switch (event) {
        case PLK_NMT_EVENT_TO_OPERATIONAL:
        case PLK_NMT_EVENT_START_NODE:
          return PLK_NMT_CS_OPERATIONAL;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return PLK_NMT_CS_PRE_OPERATIONAL_2;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_STOP_NODE:
          return PLK_NMT_CS_STOPPED;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_OPERATIONAL:
      switch (event) {
        case PLK_NMT_EVENT_STOP_NODE:
          return PLK_NMT_CS_STOPPED;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return PLK_NMT_CS_PRE_OPERATIONAL_2;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_TO_READY_TO_OPERATE:
          return PLK_NMT_CS_READY_TO_OPERATE;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    case PLK_NMT_CS_STOPPED:
      switch (event) {
        case PLK_NMT_EVENT_START_NODE:
        case PLK_NMT_EVENT_RESYNC:
          return PLK_NMT_CS_OPERATIONAL;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2:
          return PLK_NMT_CS_PRE_OPERATIONAL_2;
        case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1:
          return PLK_NMT_CS_PRE_OPERATIONAL_1;
        case PLK_NMT_EVENT_TO_READY_TO_OPERATE:
          return PLK_NMT_CS_READY_TO_OPERATE;
        case PLK_NMT_EVENT_REQUEST_RESET_APP:
          return PLK_NMT_GS_RESET_APPLICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_COM:
          return PLK_NMT_GS_RESET_COMMUNICATION;
        case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:
          return PLK_NMT_GS_RESET_CONFIGURATION;
        case PLK_NMT_EVENT_SW_RESET:
          return PLK_NMT_GS_INITIALISING;
        default:
          return state;
      }

    default:
      return state;
  }
}

/* ========== CN 状态机封装（双握手） ========== */

void plk_cn_nmt_init(PlkCnmStateMachine* sm)
{
  sm->state = PLK_NMT_GS_OFFLINE;
  sm->mnReadyToOperate = false;
  sm->appReadyToOperate = false;
}

int plk_cn_nmt_process(PlkCnmStateMachine* sm, PlkNmtEvent event)
{
  PlkNmtState newState;

  if (sm == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  /* 跟踪双握手标志 */
  if (event == PLK_NMT_EVENT_ENABLE_READY_TO_OPERATE) {
    sm->mnReadyToOperate = true;
  }
  if (event == PLK_NMT_EVENT_ENTER_READY_TO_OPERATE) {
    sm->appReadyToOperate = true;
  }

  /* PreOperational_2 的 ReadyToOperate 迁移需要双标志 */
  if (sm->state == PLK_NMT_CS_PRE_OPERATIONAL_2 &&
      (event == PLK_NMT_EVENT_ENABLE_READY_TO_OPERATE ||
       event == PLK_NMT_EVENT_ENTER_READY_TO_OPERATE)) {
    if (sm->mnReadyToOperate && sm->appReadyToOperate) {
      sm->state = PLK_NMT_CS_READY_TO_OPERATE;
      return PLK_ERR_OK;
    }
    return PLK_ERR_NMT_STATE;   /* 等待另一侧握手 */
  }

  newState = state_transition(sm->state, event);
  if (newState != sm->state) {
    sm->state = newState;
    /* 离开 PreOp2 或复位时清除握手标志 */
    if (sm->state == PLK_NMT_GS_INITIALISING ||
        sm->state == PLK_NMT_GS_RESET_APPLICATION ||
        sm->state == PLK_NMT_GS_RESET_COMMUNICATION ||
        sm->state == PLK_NMT_GS_RESET_CONFIGURATION) {
      sm->mnReadyToOperate = false;
      sm->appReadyToOperate = false;
    }
    return PLK_ERR_OK;
  }

  return PLK_ERR_NMT_STATE;
}

/* ========== 名称映射 ========== */

const char* plk_nmt_state_name(PlkNmtState state)
{
  switch (state) {
    case PLK_NMT_GS_OFFLINE:             return "GS_Offline";
    case PLK_NMT_GS_INITIALISING:        return "GS_Initialising";
    case PLK_NMT_GS_RESET_APPLICATION:   return "GS_ResetApplication";
    case PLK_NMT_GS_RESET_COMMUNICATION: return "GS_ResetCommunication";
    case PLK_NMT_GS_RESET_CONFIGURATION: return "GS_ResetConfiguration";
    case PLK_NMT_CS_NOT_ACTIVE:          return "CS_NotActive";
    case PLK_NMT_CS_BASIC_ETHERNET:      return "CS_BasicEthernet";
    case PLK_NMT_CS_PRE_OPERATIONAL_1:   return "CS_PreOperational1";
    case PLK_NMT_CS_PRE_OPERATIONAL_2:   return "CS_PreOperational2";
    case PLK_NMT_CS_READY_TO_OPERATE:    return "CS_ReadyToOperate";
    case PLK_NMT_CS_OPERATIONAL:         return "CS_Operational";
    case PLK_NMT_CS_STOPPED:             return "CS_Stopped";
    default:                             return "Unknown";
  }
}

const char* plk_nmt_event_name(PlkNmtEvent event)
{
  switch (event) {
    case PLK_NMT_EVENT_SW_RESET:                  return "SwReset";
    case PLK_NMT_EVENT_START_UP:                  return "StartUp";
    case PLK_NMT_EVENT_SWITCH_OFF:                return "SwitchOff";
    case PLK_NMT_EVENT_SWITCH_ON:                 return "SwitchOn";
    case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1:      return "ToPreOperational1";
    case PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2:      return "ToPreOperational2";
    case PLK_NMT_EVENT_TO_READY_TO_OPERATE:       return "ToReadyToOperate";
    case PLK_NMT_EVENT_TO_OPERATIONAL:            return "ToOperational";
    case PLK_NMT_EVENT_TO_STOPPED:                return "ToStopped";
    case PLK_NMT_EVENT_REQUEST_RESET_APP:         return "RequestResetApp";
    case PLK_NMT_EVENT_REQUEST_RESET_COM:         return "RequestResetCom";
    case PLK_NMT_EVENT_REQUEST_RESET_CONFIG:      return "RequestResetConfig";
    case PLK_NMT_EVENT_ENTER_READY_TO_OPERATE:    return "EnterReadyToOperate";
    case PLK_NMT_EVENT_STOP_NODE:                 return "StopNode";
    case PLK_NMT_EVENT_INTERNAL_RESET_APP:        return "InternalResetApp";
    case PLK_NMT_EVENT_INTERNAL_RESET_COM:        return "InternalResetCom";
    case PLK_NMT_EVENT_INTERNAL_RESET_CONFIG:     return "InternalResetConfig";
    case PLK_NMT_EVENT_ENTER_OPERATIONAL:         return "EnterOperational";
    case PLK_NMT_EVENT_NODE_ERROR:                return "NodeError";
    case PLK_NMT_EVENT_CRITICAL_ERROR:            return "CriticalError";
    case PLK_NMT_EVENT_ERROR_RESET:               return "ErrorReset";
    case PLK_NMT_EVENT_RECEIVE_SOC:               return "ReceiveSoC";
    case PLK_NMT_EVENT_RECEIVE_SOA:               return "ReceiveSoA";
    case PLK_NMT_EVENT_ENABLE_READY_TO_OPERATE:   return "EnableReadyToOperate";
    case PLK_NMT_EVENT_MN_ENTER_OPERATIONAL:      return "MNEnterOperational";
    case PLK_NMT_EVENT_START_NODE:                return "StartNode";
    case PLK_NMT_EVENT_RESYNC:                    return "Resync";
    case PLK_NMT_EVENT_DISABLE_READY_TO_OPERATE:  return "DisableReadyToOperate";
    case PLK_NMT_EVENT_DISABLE_OPERATIONAL:       return "DisableOperational";
    default:                                      return "Unknown";
  }
}

const char* plk_nmt_cmd_name(PlkNmtCommandId cmd)
{
  switch (cmd) {
    case PLK_NMT_CMD_SW_RESET:                return "SwReset";
    case PLK_NMT_CMD_START_UP:                return "StartUp";
    case PLK_NMT_CMD_STOP:                    return "Stop";
    case PLK_NMT_CMD_ENTER_PRE_OPERATIONAL_1: return "EnterPreOperational1";
    case PLK_NMT_CMD_ENTER_PRE_OPERATIONAL_2: return "EnterPreOperational2";
    case PLK_NMT_CMD_START_NODE:              return "StartNode";
    case PLK_NMT_CMD_STOP_NODE:               return "StopNode";
    case PLK_NMT_CMD_ENTER_READY_TO_OPERATE:  return "EnterReadyToOperate";
    case PLK_NMT_CMD_ENTER_OPERATIONAL:       return "EnterOperational";
    case PLK_NMT_CMD_RESYNC:                  return "Resync";
    default:                                  return "Unknown";
  }
}
