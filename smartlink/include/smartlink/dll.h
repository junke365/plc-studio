/**
 * dll.h - DLL (Data Link Layer) 周期调度与节点管理定义
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.4 ~ 4.6 章。
 * DLL 层负责：
 *   - 等时周期调度（SoC / PReq / PRes 时序）
 *   - 异步阶段管理（SoA 授权、ASnd 收发）
 *   - MN 侧节点信息维护
 *
 * 循环结构：
 *   一个"复用周期" (multiplied cycle) 由多个"分频周期" (prescaled cycle) 组成，
 *   每个分频周期包含一个等时段（PReq/PRes 交换）和一个异步段（SoA + ASnd）。
 */

#ifndef SL_DLL_H
#define SL_DLL_H

#include "smartlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== DLL 常量 ========== */

#define SL_DLL_MAX_NODES        254   /* 最大节点数 */
#define SL_DLL_MAX_ASND_SERVICES 16  /* 每节点最大 ASnd 服务数 */
#define SL_DLL_MAX_PRESCALERS   128  /* 最大分频因子 */

/* ========== 周期参数 ========== */

/**
 * 循环周期参数。
 * 实际 SoC 周期 = cycleLen * prescaledCycle；复用周期 = SoC 周期 * multipliedCycle。
 */
typedef struct
{
  uint32_t cycleLen;          /* 等时周期长度 [ns]，典型 100000 (100us) */
  uint16_t multipliedCycle;   /* 复用周期因子 (>=1) */
  uint16_t prescaledCycle;    /* 分频周期因子 (>=1) */
} SlCycleParam;

/**
 * 异步阶段参数。
 */
typedef struct
{
  uint16_t asyncMtu;          /* 异步帧最大载荷 (默认 1500) */
  uint8_t  asyncSlotId;       /* 当前异步槽节点 ID */
  uint8_t  asyncSlotPriority; /* 异步槽优先级 */
  uint16_t asyncSlotTimeout;  /* 异步槽超时 [us] */
} SlAsyncParam;

/* ========== 节点信息（MN 维护） ========== */

/**
 * MN 视角的节点信息。
 * 每个 CN 一个条目，由 IdentRes/StatusRes/PDO 接收状态维护。
 */
typedef struct
{
  uint8_t   nodeId;            /* 节点 ID */
  uint8_t   nodeType;          /* 0=MN 1=CN */
  uint8_t   nmtState;          /* 当前 NMT 状态码（低位字节） */
  uint8_t   prescaler;         /* 节点使用的分频因子 */
  bool      configured;        /* 已配置 */
  bool      connected;         /* 已连接（周期性收到其帧） */
  bool      presChaining;      /* 支持 PRes 链 */
  uint8_t   aMacAddress[6];    /* 节点 MAC 地址 */
  uint16_t  minCycleTime;      /* CN 支持的最小周期 [us] */
  uint16_t  maxCycleTime;      /* CN 支持的最大周期 [us] */
  uint16_t  minAsyncMtu;       /* CN 支持的最小异步 MTU */
  uint16_t  maxAsyncMtu;       /* CN 支持的最大异步 MTU */
  uint8_t   aReserved[4];
} SlDllNodeInfo;

/* ========== ASnd 服务信息 ========== */

/**
 * 某节点的 ASnd 服务表条目。
 */
typedef struct
{
  uint8_t serviceId;   /* ASnd 服务 ID (IdentRes/StatusRes/NmtCmd/Sdo/...) */
  uint8_t prio;        /* 服务优先级 */
  uint16_t time;       /* 请求周期 [us]，0 表示单次 */
} SlAsndServiceInfo;

/* ========== DLL 接口 ========== */

/**
 * 初始化 DLL 层（清空节点表）。
 */
int sl_dll_init(void);

/**
 * 注册/更新一个节点到 DLL 节点表。
 */
int sl_dll_register_node(const SlDllNodeInfo* info);

/**
 * 删除节点。
 */
int sl_dll_unregister_node(uint8_t nodeId);

/**
 * 查询节点信息。
 */
int sl_dll_get_node(uint8_t nodeId, SlDllNodeInfo* info);

/**
 * 按索引遍历节点表（供周期调度轮询所有节点）。
 * @param index 表内索引（0 起）
 * @return SL_ERR_OK 有节点；SL_ERR_NODE_NOT_FOUND 超出末尾
 */
int sl_dll_get_node_at(uint32_t index, SlDllNodeInfo* info);

/**
 * 设置当前周期参数。
 */
int sl_dll_set_cycle_param(const SlCycleParam* param);

/**
 * 设置异步阶段参数。
 */
int sl_dll_set_async_param(const SlAsyncParam* param);

#ifdef __cplusplus
}
#endif

#endif /* SL_DLL_H */
