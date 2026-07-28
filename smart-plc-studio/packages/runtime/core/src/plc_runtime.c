/**
 * plc_runtime.c - PLC 运行时核心实现
 *
 * 整合变量管理、任务调度、I/O 管理、通信、调试等模块
 * 提供统一的运行时生命周期管理：初始化、加载、启动、停止、暂停、恢复、复位
 */

#include "plc_runtime.h"
#include <string.h>

/* ========== 内部辅助 ========== */

/* 默认任务配置 */
#define DEFAULT_TASK_PERIOD_MS  10
#define DEFAULT_TASK_PRIORITY   128
#define DEFAULT_WATCHDOG_TIMEOUT_MS  5000

/* 主任务上下文（传给 generated_main 的 context） */
static PlcRuntime* s_runtime_ctx = NULL;

/**
 * 主任务回调：调用生成的用户逻辑
 */
static void main_task_callback(void* context) {
  (void)context;
  generated_main();
}

/* ========== 接口函数实现 ========== */

void plc_runtime_init(PlcRuntime* rt) {
  if (rt == NULL) return;

  memset(rt, 0, sizeof(PlcRuntime));

  /* 初始化各子模块 */
  plc_var_init(&rt->var_table);
  plc_task_init(&rt->task_scheduler);
  plc_io_init(&rt->io_config);
  plc_debug_init(&rt->debugger);
  plc_comm_init();

  /* 设置默认参数 */
  rt->state = PLC_STATE_INIT;
  rt->config_task_period_ms = DEFAULT_TASK_PERIOD_MS;
  rt->config_task_priority = DEFAULT_TASK_PRIORITY;
  rt->watchdog_enabled = true;
  rt->watchdog_timeout_ms = DEFAULT_WATCHDOG_TIMEOUT_MS;
  rt->watchdog_last_feed = 0;

  /* 保存全局上下文 */
  s_runtime_ctx = rt;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时初始化完成");
}

int plc_runtime_load(PlcRuntime* rt) {
  if (rt == NULL) return -1;
  if (rt->state != PLC_STATE_INIT && rt->state != PLC_STATE_STOPPED) return -1;

  /* 调用生成代码的初始化函数，注册变量和 I/O 映射 */
  generated_init(&rt->var_table, &rt->io_config);

  plc_platform_log(PLC_LOG_INFO, "PLC 代码加载完成，变量数: %u, I/O 通道数: %u",
                   rt->var_table.count, rt->io_config.channel_count);

  return 0;
}

int plc_runtime_start(PlcRuntime* rt) {
  if (rt == NULL) return -1;
  if (rt->state != PLC_STATE_STOPPED && rt->state != PLC_STATE_INIT) return -1;

  /* 绑定 I/O 通道到变量 */
  for (uint16_t i = 0; i < rt->io_config.channel_count; i++) {
    plc_io_bind(&rt->io_config, i, &rt->var_table);
  }

  /* 创建主任务：执行用户逻辑 */
  int task_id = plc_task_create(&rt->task_scheduler, "main_task",
                                 TASK_TYPE_CYCLIC,
                                 rt->config_task_period_ms,
                                 rt->config_task_priority,
                                 main_task_callback, rt);
  if (task_id < 0) {
    plc_platform_log(PLC_LOG_ERROR, "创建主任务失败");
    return -1;
  }

  /* 启动看门狗计时 */
  rt->watchdog_last_feed = plc_platform_tick_ms();

  /* 启动所有任务 */
  plc_task_start_all(&rt->task_scheduler);

  /* 更新运行时状态 */
  rt->state = PLC_STATE_RUNNING;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时已启动");
  return 0;
}

int plc_runtime_stop(PlcRuntime* rt) {
  if (rt == NULL) return -1;
  if (rt->state != PLC_STATE_RUNNING && rt->state != PLC_STATE_PAUSED) return -1;

  /* 停止所有任务 */
  plc_task_stop_all(&rt->task_scheduler);

  /* 关闭所有通信通道 */
  plc_comm_close(0);  /* 按需关闭各通道 */

  /* 更新状态 */
  rt->state = PLC_STATE_STOPPED;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时已停止");
  return 0;
}

int plc_runtime_pause(PlcRuntime* rt) {
  if (rt == NULL) return -1;
  if (rt->state != PLC_STATE_RUNNING) return -1;

  /* 暂停所有任务（不执行但不销毁） */
  plc_task_stop_all(&rt->task_scheduler);

  rt->state = PLC_STATE_PAUSED;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时已暂停");
  return 0;
}

int plc_runtime_resume(PlcRuntime* rt) {
  if (rt == NULL) return -1;
  if (rt->state != PLC_STATE_PAUSED) return -1;

  /* 恢复所有任务 */
  plc_task_start_all(&rt->task_scheduler);

  /* 重置看门狗 */
  rt->watchdog_last_feed = plc_platform_tick_ms();

  rt->state = PLC_STATE_RUNNING;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时已恢复");
  return 0;
}

int plc_runtime_reset(PlcRuntime* rt) {
  if (rt == NULL) return -1;

  /* 停止所有任务 */
  plc_task_stop_all(&rt->task_scheduler);

  /* 清零统计信息 */
  memset(&rt->stats, 0, sizeof(PlcStats));

  /* 清零变量数据区（保留变量定义但清除数据） */
  memset(rt->var_table.var_data, 0, sizeof(rt->var_table.var_data));
  rt->var_table.data_offset = 0;

  /* 重新初始化 I/O */
  plc_io_init(&rt->io_config);

  /* 重新初始化调试器 */
  plc_debug_init(&rt->debugger);

  /* 重置任务调度器 */
  plc_task_init(&rt->task_scheduler);

  /* 重置看门狗 */
  rt->watchdog_last_feed = plc_platform_tick_ms();

  rt->state = PLC_STATE_STOPPED;

  plc_platform_log(PLC_LOG_INFO, "PLC 运行时已复位");
  return 0;
}

void plc_runtime_scan(PlcRuntime* rt) {
  if (rt == NULL) return;
  if (rt->state != PLC_STATE_RUNNING) return;

  uint32_t scan_start = plc_platform_tick_ms();
  uint64_t scan_start_us = plc_platform_tick_us();

  /* 步骤 1：读取所有输入 */
  plc_io_read_inputs(&rt->io_config);

  /* 步骤 2：执行任务调度 */
  plc_task_schedule(&rt->task_scheduler);

  /* 步骤 3：写入所有输出 */
  plc_io_write_outputs(&rt->io_config);

  /* 步骤 4：更新统计信息 */
  rt->stats.cycle_count++;
  uint64_t scan_end_us = plc_platform_tick_us();
  uint32_t cycle_us = (uint32_t)(scan_end_us - scan_start_us);
  rt->stats.cycle_time_us = cycle_us;
  if (cycle_us > rt->stats.max_cycle_time_us) {
    rt->stats.max_cycle_time_us = cycle_us;
  }

  rt->stats.uptime_ms += plc_platform_tick_ms() - scan_start;

  /* 步骤 5：喂看门狗 */
  if (rt->watchdog_enabled) {
    rt->watchdog_last_feed = plc_platform_tick_ms();
  }
}

PlcState plc_runtime_get_state(PlcRuntime* rt) {
  if (rt == NULL) return PLC_STATE_INIT;
  return rt->state;
}

void plc_runtime_get_stats(PlcRuntime* rt, PlcStats* stats) {
  if (rt == NULL || stats == NULL) return;
  memcpy(stats, &rt->stats, sizeof(PlcStats));
}

PlcVarTable* plc_runtime_get_var_table(PlcRuntime* rt) {
  if (rt == NULL) return NULL;
  return &rt->var_table;
}

PlcTaskScheduler* plc_runtime_get_scheduler(PlcRuntime* rt) {
  if (rt == NULL) return NULL;
  return &rt->task_scheduler;
}

PlcIoConfig* plc_runtime_get_io_config(PlcRuntime* rt) {
  if (rt == NULL) return NULL;
  return &rt->io_config;
}

PlcDebugger* plc_runtime_get_debugger(PlcRuntime* rt) {
  if (rt == NULL) return NULL;
  return &rt->debugger;
}
