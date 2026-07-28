/**
 * linux-x86/main.c - x86 Linux PLC 运行时示例
 *
 * 演示在 x86 Linux 平台上运行 PLC 运行时：
 * - 端口映射方式的 I/O（inb/outb）
 * - Modbus TCP 服务器（端口 502）
 * - 周期时间统计
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "plc_runtime.h"

/* ========== x86 端口 I/O 直接访问（需 root 权限） ========== */

#ifdef PLATFORM_LINUX_X86
#include <sys/io.h>

static inline uint8_t x86_inb(uint16_t port)
{
  return inb(port);
}

static inline void x86_outb(uint16_t port, uint8_t val)
{
  outb(val, port);
}
#endif

/* ========== 全局状态 ========== */

static volatile int g_running = 1;
static PlcRuntime g_runtime;

/* 周期时间统计 */
typedef struct {
  uint32_t min_us;
  uint32_t max_us;
  uint32_t total_us;
  uint32_t count;
  uint32_t last_us;
} CycleTimeStats;

static CycleTimeStats g_cycle_stats = {0, 0, 0, 0, 0};

/* ========== 信号处理 ========== */

static void signal_handler(int signo)
{
  (void)signo;
  printf("[x86] 收到 SIGINT 信号，正在关闭...\n");
  g_running = 0;
}

/* ========== 周期时间统计 ========== */

static void update_cycle_stats(uint32_t cycle_us)
{
  g_cycle_stats.last_us = cycle_us;
  g_cycle_stats.total_us += cycle_us;
  g_cycle_stats.count++;

  if (g_cycle_stats.min_us == 0 || cycle_us < g_cycle_stats.min_us) {
    g_cycle_stats.min_us = cycle_us;
  }
  if (cycle_us > g_cycle_stats.max_us) {
    g_cycle_stats.max_us = cycle_us;
  }
}

static void print_cycle_stats(void)
{
  if (g_cycle_stats.count == 0) return;

  uint32_t avg_us = g_cycle_stats.total_us / g_cycle_stats.count;
  double jitter_us = 0.0;

  if (g_cycle_stats.count > 1) {
    /* 计算抖动 = (max - min) / 2 */
    jitter_us = (double)(g_cycle_stats.max_us - g_cycle_stats.min_us) / 2.0;
  }

  printf("[x86] 周期统计: 平均=%u us | 最小=%u us | 最大=%u us | "
         "抖动=~%.0f us | 总数=%u\n",
         avg_us,
         g_cycle_stats.min_us,
         g_cycle_stats.max_us,
         jitter_us,
         g_cycle_stats.count);
}

/* ========== 任务回调 ========== */

static void main_task_callback(void* ctx)
{
  PlcRuntime* rt = (PlcRuntime*)ctx;
  uint64_t t0 = plc_platform_tick_us();

  plc_runtime_scan(rt);

  uint64_t t1 = plc_platform_tick_us();
  update_cycle_stats((uint32_t)(t1 - t0));
}

static void comm_task_callback(void* ctx)
{
  (void)ctx;
  /* Modbus TCP 处理 */
}

/* ========== Modbus TCP 服务器（简化） ========== */

/* 注意：实际实现应使用 libmodbus 或自定义 TCP socket */
static void modbus_tcp_init(uint16_t port)
{
  printf("[x86] Modbus TCP 服务器初始化 (端口 %u)\n", port);
  printf("[x86] 注意：实际实现需集成 libmodbus 库\n");
}

/* ========== 主函数 ========== */

int main(void)
{
  int ret;

  printf("Smart PLC Runtime - x86 Linux 示例\n");
  printf("==================================\n");

  /* x86 端口访问权限（需 root） */
#ifdef PLATFORM_LINUX_X86
  if (ioperm(0x200, 16, 1) != 0) {
    printf("[x86] 警告：无法获取端口 I/O 权限，将使用模拟 I/O\n");
  }
#endif

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
  plc_var_register(vt, "conveyor_speed", VAR_TYPE_REAL, VAR_ATTR_OUTPUT,
                   sizeof(plc_real), "传送带速度 (m/s)");
  plc_var_register(vt, "temperature", VAR_TYPE_REAL, VAR_ATTR_INPUT,
                   sizeof(plc_real), "温度传感器 (°C)");
  plc_var_register(vt, "emergency_stop", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "紧急停止按钮");
  plc_var_register(vt, "conveyor_motor", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "传送带电机控制");

  printf("[x86] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O：端口映射 */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);

  /* 4 路 DI（端口 0x200-0x203） */
  plc_io_register(io, IO_TYPE_DI, "DI_0", "emergency_stop",  0x200);
  plc_io_register(io, IO_TYPE_DI, "DI_1", NULL,              0x201);
  plc_io_register(io, IO_TYPE_DI, "DI_2", NULL,              0x202);
  plc_io_register(io, IO_TYPE_DI, "DI_3", NULL,              0x203);

  /* 2 路 DO（端口 0x210-0x211） */
  plc_io_register(io, IO_TYPE_DO, "DO_0", "conveyor_motor", 0x210);
  plc_io_register(io, IO_TYPE_DO, "DO_1", NULL,              0x211);

  /* 2 路 AI（端口 0x220-0x221） */
  plc_io_register(io, IO_TYPE_AI, "AI_0", "temperature",    0x220);
  plc_io_register(io, IO_TYPE_AI, "AI_1", NULL,              0x221);

  /* 1 路 AO（端口 0x230） */
  plc_io_register(io, IO_TYPE_AO, "AO_0", "conveyor_speed", 0x230);

  /* 绑定 */
  plc_io_bind(io, 0, vt);  /* DI_0 -> emergency_stop */
  plc_io_bind(io, 4, vt);  /* DO_0 -> conveyor_motor */
  plc_io_bind(io, 6, vt);  /* AI_0 -> temperature */
  plc_io_bind(io, 9, vt);  /* AO_0 -> conveyor_speed */

  printf("[x86] I/O 配置: 4 DI + 2 DO + 2 AI + 1 AO (端口映射)\n");

  /* 创建任务 */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  10, 200, main_task_callback, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  100, 100, comm_task_callback, &g_runtime);

  printf("[x86] 任务: MainTask(10ms), CommTask(100ms)\n");

  /* Modbus TCP 初始化 */
  modbus_tcp_init(502);

  /* 加载并启动 */
  ret = plc_runtime_load(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[x86] 加载生成代码失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  ret = plc_runtime_start(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[x86] 启动运行时失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  printf("[x86] PLC 运行时已启动 (Ctrl+C 退出)\n\n");

  /* 主循环 */
  uint32_t last_stats_tick = plc_platform_tick_ms();

  while (g_running) {
    plc_task_schedule(sched);

    uint32_t now = plc_platform_tick_ms();
    if (now - last_stats_tick >= 1000) {
      PlcStats stats;
      plc_runtime_get_stats(&g_runtime, &stats);
      printf("[x86] 运行: %u s | 周期: %u us | 最大: %u us | 错误: %u\n",
             stats.uptime_ms / 1000, stats.cycle_time_us,
             stats.max_cycle_time_us, stats.error_count);
      print_cycle_stats();
      last_stats_tick = now;
    }

    plc_platform_delay_ms(1);
  }

  printf("\n[x86] 正在停止...\n");
  plc_runtime_stop(&g_runtime);
  printf("[x86] PLC 运行时已停止\n");

  return EXIT_SUCCESS;
}
