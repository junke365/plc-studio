/**
 * plc_runtime.h - PLC 运行时核心
 *
 * 整合所有模块，提供统一的运行时生命周期管理
 * 支持：启动、停止、暂停、复位
 */

#ifndef PLC_RUNTIME_H
#define PLC_RUNTIME_H

#include "plc_platform.h"
#include "plc_var.h"
#include "plc_task.h"
#include "plc_io.h"
#include "plc_comm.h"
#include "plc_debug.h"

/* ========== 运行时状态 ========== */

typedef enum {
  PLC_STATE_INIT,
  PLC_STATE_STOPPED,
  PLC_STATE_RUNNING,
  PLC_STATE_PAUSED,
  PLC_STATE_ERROR,
} PlcState;

/* 运行时统计 */
typedef struct {
  uint32_t       uptime_ms;
  uint32_t       cycle_count;
  uint32_t       cycle_time_us;
  uint32_t       max_cycle_time_us;
  uint32_t       error_count;
  uint32_t       watchdog_resets;
} PlcStats;

/* ========== 生成代码接口（由编译器生成） ========== */

/**
 * 生成代码初始化（注册变量、配置 I/O 映射等）
 */
extern void generated_init(PlcVarTable* var_table, PlcIoConfig* io_config);

/**
 * 生成代码主函数（在周期任务中调用）
 * 执行所有 POU 的 BODY
 */
extern void generated_main(void);

/**
 * 获取 POU 数量
 */
extern uint32_t generated_pou_count(void);

/**
 * 获取 POU 名称
 */
extern const char* generated_pou_name(uint32_t index);

/* ========== 运行时上下文 ========== */

typedef struct {
  PlcState         state;
  PlcVarTable      var_table;
  PlcTaskScheduler task_scheduler;
  PlcIoConfig      io_config;
  PlcDebugger      debugger;
  PlcStats         stats;
  uint32_t         config_task_period_ms;
  uint8_t          config_task_priority;
  bool             watchdog_enabled;
  uint32_t         watchdog_timeout_ms;
  uint32_t         watchdog_last_feed;
} PlcRuntime;

/* ========== 接口函数 ========== */

/**
 * 初始化运行时
 */
void plc_runtime_init(PlcRuntime* rt);

/**
 * 加载生成的代码
 */
int plc_runtime_load(PlcRuntime* rt);

/**
 * 启动 PLC 扫描
 */
int plc_runtime_start(PlcRuntime* rt);

/**
 * 停止 PLC 扫描
 */
int plc_runtime_stop(PlcRuntime* rt);

/**
 * 暂停 PLC 扫描
 */
int plc_runtime_pause(PlcRuntime* rt);

/**
 * 恢复 PLC 扫描
 */
int plc_runtime_resume(PlcRuntime* rt);

/**
 * 复位运行时
 */
int plc_runtime_reset(PlcRuntime* rt);

/**
 * 主扫描循环（在平台主循环中调用）
 * 1. 读输入
 * 2. 执行任务调度
 * 3. 写输出
 * 4. 喂看门狗
 */
void plc_runtime_scan(PlcRuntime* rt);

/**
 * 获取运行时状态
 */
PlcState plc_runtime_get_state(PlcRuntime* rt);

/**
 * 获取运行时统计
 */
void plc_runtime_get_stats(PlcRuntime* rt, PlcStats* stats);

/**
 * 获取全局变量表
 */
PlcVarTable* plc_runtime_get_var_table(PlcRuntime* rt);

/**
 * 获取任务调度器
 */
PlcTaskScheduler* plc_runtime_get_scheduler(PlcRuntime* rt);

/**
 * 获取 I/O 配置
 */
PlcIoConfig* plc_runtime_get_io_config(PlcRuntime* rt);

/**
 * 获取调试器
 */
PlcDebugger* plc_runtime_get_debugger(PlcRuntime* rt);

#endif /* PLC_RUNTIME_H */
