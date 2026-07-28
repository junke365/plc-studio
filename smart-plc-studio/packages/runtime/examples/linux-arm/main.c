/**
 * linux-arm/main.c - ARM Linux PLC 运行时示例
 *
 * 演示在 ARM Linux 平台上运行 PLC 运行时：
 * - 初始化运行时并注册变量
 * - 配置 4 路 DI + 2 路 DO + 2 路 AI + 1 路 AO
 * - 创建周期任务
 * - 主循环扫描 + 统计输出
 * - SIGINT 信号处理实现优雅关闭
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "plc_runtime.h"

/* ========== 全局状态 ========== */

static volatile int g_running = 1;
static PlcRuntime g_runtime;

/* ========== 信号处理 ========== */

static void signal_handler(int signo)
{
  (void)signo;
  printf("[ARM] 收到 SIGINT 信号，正在优雅关闭...\n");
  g_running = 0;
}

/* ========== 任务回调 ========== */

/* MainTask 回调：10ms 周期，执行 PLC 逻辑 */
static void main_task_callback(void* ctx)
{
  PlcRuntime* rt = (PlcRuntime*)ctx;
  plc_runtime_scan(rt);
}

/* CommTask 回调：100ms 周期，处理通信 */
static void comm_task_callback(void* ctx)
{
  (void)ctx;
  /* 通信任务：处理 Modbus RTU/TCP 请求 */
}

/* ========== 统计打印 ========== */

static void print_stats(PlcRuntime* rt)
{
  PlcStats stats;
  plc_runtime_get_stats(rt, &stats);

  printf("[ARM] 运行时间: %u 秒 | 周期数: %u | "
         "周期时间: %u us | 最大: %u us | 错误: %u\n",
         stats.uptime_ms / 1000,
         stats.cycle_count,
         stats.cycle_time_us,
         stats.max_cycle_time_us,
         stats.error_count);
}

/* ========== 主函数 ========== */

int main(void)
{
  int ret;

  printf("Smart PLC Runtime - ARM Linux 示例\n");
  printf("==================================\n");

  /* 安装信号处理 */
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  /* 初始化运行时 */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "motor_speed", VAR_TYPE_REAL, VAR_ATTR_OUTPUT,
                   sizeof(plc_real), "电机转速 (RPM)");
  plc_var_register(vt, "sensor_value", VAR_TYPE_INT, VAR_ATTR_INPUT,
                   sizeof(plc_int), "传感器读数");
  plc_var_register(vt, "start_button", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "启动按钮");
  plc_var_register(vt, "stop_button", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "停止按钮");
  plc_var_register(vt, "alarm_output", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "报警输出");

  printf("[ARM] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O 通道 */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);

  /* 4 路数字量输入 */
  plc_io_register(io, IO_TYPE_DI, "DI_0", "start_button", 0x00);
  plc_io_register(io, IO_TYPE_DI, "DI_1", "stop_button",  0x01);
  plc_io_register(io, IO_TYPE_DI, "DI_2", NULL,           0x02);
  plc_io_register(io, IO_TYPE_DI, "DI_3", NULL,           0x03);

  /* 2 路数字量输出 */
  plc_io_register(io, IO_TYPE_DO, "DO_0", "alarm_output", 0x10);
  plc_io_register(io, IO_TYPE_DO, "DO_1", NULL,           0x11);

  /* 2 路模拟量输入 */
  plc_io_register(io, IO_TYPE_AI, "AI_0", "sensor_value", 0x20);
  plc_io_register(io, IO_TYPE_AI, "AI_1", NULL,           0x21);

  /* 1 路模拟量输出 */
  plc_io_register(io, IO_TYPE_AO, "AO_0", "motor_speed",  0x30);

  /* 绑定变量到 I/O 通道 */
  plc_io_bind(io, 0, vt);  /* DI_0 -> start_button */
  plc_io_bind(io, 1, vt);  /* DI_1 -> stop_button */
  plc_io_bind(io, 6, vt);  /* AI_0 -> sensor_value */
  plc_io_bind(io, 8, vt);  /* DO_0 -> alarm_output */
  plc_io_bind(io, 10, vt); /* AO_0 -> motor_speed */

  printf("[ARM] I/O 通道: 4 DI + 2 DO + 2 AI + 1 AO\n");

  /* 创建任务 */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);

  int main_task_id = plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                                     10, 200, main_task_callback, &g_runtime);
  int comm_task_id = plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                                     100, 100, comm_task_callback, &g_runtime);

  if (main_task_id < 0 || comm_task_id < 0) {
    fprintf(stderr, "[ARM] 创建任务失败\n");
    return EXIT_FAILURE;
  }

  printf("[ARM] 任务: MainTask(%d ms), CommTask(%d ms)\n",
         (int)sched->tasks[main_task_id].interval_ms,
         (int)sched->tasks[comm_task_id].interval_ms);

  /* 加载生成代码 */
  ret = plc_runtime_load(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[ARM] 加载生成代码失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  /* 启动运行时 */
  ret = plc_runtime_start(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[ARM] 启动运行时失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  printf("[ARM] PLC 运行时已启动，进入主循环 (Ctrl+C 退出)\n\n");

  /* 主循环 */
  uint32_t last_stats_tick = plc_platform_tick_ms();

  while (g_running) {
    plc_task_schedule(sched);

    /* 每秒打印一次统计 */
    uint32_t now = plc_platform_tick_ms();
    if (now - last_stats_tick >= 1000) {
      print_stats(&g_runtime);
      last_stats_tick = now;
    }

    /* 1ms 基础 tick */
    plc_platform_delay_ms(1);
  }

  /* 优雅关闭 */
  printf("\n[ARM] 正在停止 PLC 运行时...\n");
  plc_runtime_stop(&g_runtime);
  printf("[ARM] PLC 运行时已停止，退出程序\n");

  return EXIT_SUCCESS;
}
