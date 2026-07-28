/**
 * xenomai4/main.c - Xenomai 4 实时 PLC 运行时示例
 *
 * 演示在 Xenomai 4 (Cobalt) 实时框架下运行 PLC 运行时：
 * - 使用 cobalt_init() 初始化 Cobalt 核心
 * - 创建高优先级 RT 任务执行 PLC 扫描
 * - RT 安全的日志记录（环形缓冲区，不使用 printf）
 * - 最坏情况执行时间 (WCET) 统计
 * - SIGTERM 优雅关闭
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* Xenomai 4 Cobalt 头文件 */
#include <sys/cobalt.h>
#include <rtdm/rtdm.h>

#include "plc_runtime.h"

/* ========== RT 安全日志（环形缓冲区） ========== */

#define RT_LOG_RING_SIZE  256
#define RT_LOG_MSG_MAX    128

typedef struct {
  char     messages[RT_LOG_RING_SIZE][RT_LOG_MSG_MAX];
  uint32_t head;
  uint32_t tail;
  uint32_t count;
} RtLogRing;

static RtLogRing g_rt_log;

static void rt_log_init(void)
{
  memset(&g_rt_log, 0, sizeof(g_rt_log));
}

/* RT 安全：将日志写入环形缓冲区（无 malloc、无 printf） */
static void rt_log_write(const char* msg)
{
  uint32_t next = (g_rt_log.head + 1) % RT_LOG_RING_SIZE;

  /* 缓冲区满时覆盖最旧的条目 */
  if (g_rt_log.count >= RT_LOG_RING_SIZE) {
    g_rt_log.tail = (g_rt_log.tail + 1) % RT_LOG_RING_SIZE;
  } else {
    g_rt_log.count++;
  }

  /* 简单字符串拷贝（RT 安全） */
  uint32_t i = 0;
  while (msg[i] && i < RT_LOG_MSG_MAX - 1) {
    g_rt_log.messages[g_rt_log.head][i] = msg[i];
    i++;
  }
  g_rt_log.messages[g_rt_log.head][i] = '\0';
  g_rt_log.head = next;
}

/* 非 RT 环境：消费并打印日志 */
static void rt_log_flush(void)
{
  while (g_rt_log.count > 0) {
    printf("[RT] %s\n", g_rt_log.messages[g_rt_log.tail]);
    g_rt_log.tail = (g_rt_log.tail + 1) % RT_LOG_RING_SIZE;
    g_rt_log.count--;
  }
}

/* ========== WCET 统计 ========== */

typedef struct {
  uint64_t min_us;
  uint64_t max_us;
  uint64_t total_us;
  uint64_t count;
  uint64_t last_us;
  uint64_t deadline_misses;
  uint64_t deadline_us;  /* 周期截止时间（微秒） */
} WcetStats;

static WcetStats g_wcet = {0, 0, 0, 0, 0, 0, 10000}; /* 默认 10ms 截止 */

static void wcet_update(uint64_t exec_us)
{
  g_wcet.last_us = exec_us;
  g_wcet.total_us += exec_us;
  g_wcet.count++;

  if (g_wcet.min_us == 0 || exec_us < g_wcet.min_us) {
    g_wcet.min_us = exec_us;
  }
  if (exec_us > g_wcet.max_us) {
    g_wcet.max_us = exec_us;
  }
  if (exec_us > g_wcet.deadline_us) {
    g_wcet.deadline_misses++;
  }
}

/* ========== 全局状态 ========== */

static volatile int g_running = 1;
static PlcRuntime g_runtime;

/* ========== 信号处理 ========== */

static void signal_handler(int signo)
{
  (void)signo;
  /* 注意：信号处理中避免调用非 async-signal-safe 函数 */
  g_running = 0;
}

/* ========== RT 任务 ========== */

/* PLC 扫描 RT 任务入口 */
static void plc_rt_task(void* arg)
{
  PlcRuntime* rt = (PlcRuntime*)arg;
  uint32_t period_us = 10000; /* 10ms 周期 */

  rt_log_write("PLC RT 任务启动");

  while (g_running) {
    uint64_t t0 = plc_platform_tick_us();

    /* 执行 PLC 扫描 */
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);

    uint64_t t1 = plc_platform_tick_us();
    wcet_update(t1 - t0);

    /* 等待下一个周期 */
    uint64_t elapsed = t1 - t0;
    if (elapsed < period_us) {
      /* 使用 Cobalt 的精确休眠 */
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (long)(period_us - elapsed) * 1000;
      /* cobalt_thread_sleep_ns 在 RT 线程中使用 */
      cobalt_thread_sleep(ns2ts(&ts));
    }
  }

  rt_log_write("PLC RT 任务停止");
}

/* ========== WCET 统计打印 ========== */

static void print_wcet_stats(void)
{
  if (g_wcet.count == 0) {
    printf("[Xenomai4] 暂无 WCET 数据\n");
    return;
  }

  uint64_t avg_us = g_wcet.total_us / g_wcet.count;
  double jitter_us = 0.0;

  if (g_wcet.count > 1) {
    jitter_us = (double)(g_wcet.max_us - g_wcet.min_us) / 2.0;
  }

  printf("[Xenomai4] WCET 统计:\n");
  printf("  平均执行: %llu us\n", (unsigned long long)avg_us);
  printf("  最小执行: %llu us\n", (unsigned long long)g_wcet.min_us);
  printf("  最大执行: %llu us (WCET)\n", (unsigned long long)g_wcet.max_us);
  printf("  执行抖动: ~%.0f us\n", jitter_us);
  printf("  截止超时: %llu 次 / %llu 次\n",
         (unsigned long long)g_wcet.deadline_misses,
         (unsigned long long)g_wcet.count);
  printf("  截止时间: %llu us\n", (unsigned long long)g_wcet.deadline_us);
}

/* ========== 主函数 ========== */

int main(void)
{
  int ret;

  printf("Smart PLC Runtime - Xenomai 4 RT 示例\n");
  printf("======================================\n");

  /* 初始化 RT 日志 */
  rt_log_init();

  /* 安装信号处理（SIGTERM 用于优雅关闭） */
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  /* 初始化 Cobalt 核心（必须在任何 RT 操作之前） */
  ret = cobalt_init();
  if (ret != 0) {
    fprintf(stderr, "[Xenomai4] cobalt_init() 失败: %d\n", ret);
    fprintf(stderr, "[Xenomai4] 确保已加载 Xenomai 内核模块\n");
    return EXIT_FAILURE;
  }
  printf("[Xenomai4] Cobalt 核心已初始化\n");

  /* 初始化运行时 */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "servo_position", VAR_TYPE_DINT, VAR_ATTR_INOUT,
                   sizeof(plc_dint), "伺服位置 (脉冲)");
  plc_var_register(vt, "servo_velocity", VAR_TYPE_REAL, VAR_ATTR_OUTPUT,
                   sizeof(plc_real), "伺服速度 (RPM)");
  plc_var_register(vt, "limit_switch", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "限位开关");

  printf("[Xenomai4] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_ENCODER, "ENC_0", "servo_position", 0x00);
  plc_io_register(io, IO_TYPE_DO,      "PWM_0", "servo_velocity", 0x10);
  plc_io_register(io, IO_TYPE_DI,      "DI_0",  "limit_switch",  0x20);

  plc_io_bind(io, 0, vt); /* ENC_0 -> servo_position */
  plc_io_bind(io, 1, vt); /* PWM_0 -> servo_velocity */
  plc_io_bind(io, 2, vt); /* DI_0  -> limit_switch */

  /* 创建任务（非 RT，由 RT 任务直接调用调度器） */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  10, 255, NULL, &g_runtime);

  /* 加载并启动 */
  ret = plc_runtime_load(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[Xenomai4] 加载生成代码失败\n");
    return EXIT_FAILURE;
  }

  /* 创建 RT 线程（最高优先级 99） */
  pthread_t rt_thread;
  pthread_attr_t attr;
  struct sched_param param;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  /* 设置 Xenomai 实时调度策略 */
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = 99; /* 最高优先级 */
  pthread_attr_setschedparam(&attr, &param);

  /* 绑定到 Cobalt 核心 */
  pthread_attr_setname_np(&attr, "plc-rt-task");

  ret = pthread_create(&rt_thread, &attr, (void*(*)(void*))plc_rt_task,
                       &g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[Xenomai4] 创建 RT 线程失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  pthread_attr_destroy(&attr);

  printf("[Xenomai4] RT 任务已启动 (优先级 99, 周期 10ms)\n");
  printf("[Xenomai4] 进入主循环 (SIGTERM/SIGINT 退出)\n\n");

  /* 主循环：非 RT，负责消费日志和打印统计 */
  uint32_t last_stats_tick = plc_platform_tick_ms();

  while (g_running) {
    /* 消费 RT 日志（非 RT 环境打印） */
    rt_log_flush();

    uint32_t now = plc_platform_tick_ms();
    if (now - last_stats_tick >= 1000) {
      print_wcet_stats();
      last_stats_tick = now;
    }

    usleep(1000); /* 1ms 轮询 */
  }

  /* 等待 RT 线程退出 */
  printf("\n[Xenomai4] 等待 RT 线程退出...\n");
  pthread_join(rt_thread, NULL);

  /* 关闭 Cobalt */
  printf("[Xenomai4] 正在关闭 Cobalt 核心...\n");
  cobalt_deinit();

  printf("[Xenomai4] PLC 运行时已停止\n");
  return EXIT_SUCCESS;
}
