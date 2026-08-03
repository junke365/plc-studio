/**
 * dll.c - DLL 层实现：周期参数管理 + MN 节点表
 *
 * 节点表由 MN 维护，每个 CN 一个条目。
 * 数据链路层周期调度逻辑（SoC/PReq/PRes 时序编排）将在 mn/ 主站模块中实现，
 * 本文件提供其依赖的节点信息存储与查询。
 */

#include "smartlink/dll.h"

#define SL_DLL_TABLE_SIZE  254   /* 节点表容量（节点 ID 1..254） */

typedef struct
{
  SlDllNodeInfo table[SL_DLL_TABLE_SIZE];
  uint32_t       count;
  SlCycleParam  cycleParam;
  SlAsyncParam  asyncParam;
  bool           initialized;
} SlDllCtx;

static SlDllCtx s_ctx;

int sl_dll_init(void)
{
  memset(&s_ctx, 0, sizeof(s_ctx));
  s_ctx.cycleParam.cycleLen = 100000;        /* 默认 100us */
  s_ctx.cycleParam.multipliedCycle = 1;
  s_ctx.cycleParam.prescaledCycle = 1;
  s_ctx.asyncParam.asyncMtu = SL_ASYNC_MTU;
  s_ctx.asyncParam.asyncSlotId = SL_ADR_MN_DEF;
  s_ctx.asyncParam.asyncSlotPriority = SL_ASYNC_PRIO_GENERIC;
  s_ctx.asyncParam.asyncSlotTimeout = 100;   /* 100us */
  s_ctx.initialized = true;
  return SL_ERR_OK;
}

int sl_dll_register_node(const SlDllNodeInfo* info)
{
  uint32_t i;
  uint32_t slot;

  if (!s_ctx.initialized || info == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (info->nodeId == SL_ADR_INVALID || info->nodeId > SL_MAX_NODE_ID) {
    return SL_ERR_INVALID_PARAM;
  }

  slot = s_ctx.count;
  for (i = 0; i < s_ctx.count; i++) {
    if (s_ctx.table[i].nodeId == info->nodeId) {
      slot = i;                              /* 已存在则更新 */
      break;
    }
  }
  if (slot >= SL_DLL_TABLE_SIZE) {
    return SL_ERR_NO_MEMORY;
  }

  s_ctx.table[slot] = *info;
  if (slot == s_ctx.count) {
    s_ctx.count++;
  }
  return SL_ERR_OK;
}

int sl_dll_unregister_node(uint8_t nodeId)
{
  uint32_t i;

  if (!s_ctx.initialized) {
    return SL_ERR_NOT_INITIALIZED;
  }

  for (i = 0; i < s_ctx.count; i++) {
    if (s_ctx.table[i].nodeId == nodeId) {
      /* 尾部元素前移，保持连续 */
      s_ctx.table[i] = s_ctx.table[s_ctx.count - 1];
      s_ctx.count--;
      return SL_ERR_OK;
    }
  }
  return SL_ERR_NODE_NOT_FOUND;
}

int sl_dll_get_node(uint8_t nodeId, SlDllNodeInfo* info)
{
  uint32_t i;

  if (!s_ctx.initialized || info == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  for (i = 0; i < s_ctx.count; i++) {
    if (s_ctx.table[i].nodeId == nodeId) {
      *info = s_ctx.table[i];
      return SL_ERR_OK;
    }
  }
  return SL_ERR_NODE_NOT_FOUND;
}

int sl_dll_get_node_at(uint32_t index, SlDllNodeInfo* info)
{
  if (!s_ctx.initialized || info == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (index >= s_ctx.count) {
    return SL_ERR_NODE_NOT_FOUND;
  }
  *info = s_ctx.table[index];
  return SL_ERR_OK;
}

int sl_dll_set_cycle_param(const SlCycleParam* param)
{
  if (!s_ctx.initialized || param == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (param->cycleLen == 0 || param->multipliedCycle == 0 ||
      param->prescaledCycle == 0) {
    return SL_ERR_INVALID_PARAM;
  }
  s_ctx.cycleParam = *param;
  return SL_ERR_OK;
}

int sl_dll_set_async_param(const SlAsyncParam* param)
{
  if (!s_ctx.initialized || param == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (param->asyncMtu == 0) {
    return SL_ERR_INVALID_PARAM;
  }
  s_ctx.asyncParam = *param;
  return SL_ERR_OK;
}
