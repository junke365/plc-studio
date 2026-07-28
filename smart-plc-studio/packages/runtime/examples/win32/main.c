#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "plc_runtime.h"
#include "plc_win_rt.h"

#ifdef BUILD_MOTION
#include "plc_motion.h"
#endif

/* 存根：无生成代码时使用 */
#include "plc_var.h"
#include "plc_io.h"
void generated_init(PlcVarTable *vt, PlcIoConfig *io) { (void)vt; (void)io; }
void generated_main(void) {}
uint32_t generated_pou_count(void) { return 0; }
const char *generated_pou_name(uint32_t idx) { (void)idx; return ""; }

static volatile int g_running = 1;

static PlcRuntime g_runtime;

static CncSystem g_cnc;

static void servo_callback(void *ctx)
{
  PlcRuntime *rt = (PlcRuntime *)ctx;
  plc_runtime_scan(rt);
}

static BOOL WINAPI console_handler(DWORD dwCtrlType)
{
  (void)dwCtrlType;
  printf("\n[Win32] 收到关闭信号，正在停止...\n");
  g_running = 0;
  return TRUE;
}

static void print_runtime_stats(void)
{
  PlcStats stats;
  plc_runtime_get_stats(&g_runtime, &stats);
  printf("[Win32] 运行 %u s | 周期 %u us | 最大 %u us | 错误 %u\n",
         stats.uptime_ms / 1000, stats.cycle_time_us,
         stats.max_cycle_time_us, stats.error_count);
}

static void print_cnc_status(void)
{
#ifdef BUILD_MOTION
  if (g_cnc.status != CNC_STATUS_IDLE && g_cnc.status != CNC_STATUS_STOP) {
    static int lastPct = -1;
    int pct = (int)(plc_interp_getProgress(&g_cnc.interpolator) * 100);
    if (pct != lastPct) {
      lastPct = pct;
      printf("\r[CNC] %d%% | 段 %u/%u | 队列 %u      ",
             pct, g_cnc.currentLine, g_cnc.gcodeLineCount,
             plc_planner_queued(&g_cnc.planner));
      fflush(stdout);
    }
  }
#else
  (void)0;
#endif
}

int main(void)
{
  /* 注册控制台事件 */
  SetConsoleCtrlHandler(console_handler, TRUE);

  /* 设置控制台标题 */
  SetConsoleTitle("Smart PLC Runtime - Win32");

  /* 启用 ANSI 转义序列 (Win10+) */
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(hOut, &mode)) {
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }

  printf("========================================\n");
  printf("  Smart PLC Runtime - Win32 示例\n");
  printf("========================================\n\n");

  /* 初始化 Windows 实时平台 */
  plc_platform_init();

  /* 初始化 PLC 运行时 */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable *vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "cycle_count",   VAR_TYPE_UDINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_udint), "运行周期计数");
  plc_var_register(vt, "uptime_sec",    VAR_TYPE_UDINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_udint), "运行秒数");
  plc_var_register(vt, "cpu_load",      VAR_TYPE_REAL,  VAR_ATTR_OUTPUT,
                   sizeof(plc_real),  "CPU 负载 (%)");
  printf("[Win32] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O */
  PlcIoConfig *io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_DI, "DI_0", NULL, 0);
  plc_io_register(io, IO_TYPE_DI, "DI_1", NULL, 1);
  plc_io_register(io, IO_TYPE_DO, "DO_0", NULL, 0);
  plc_io_register(io, IO_TYPE_DO, "DO_1", NULL, 1);
  plc_io_register(io, IO_TYPE_AI, "AI_0", NULL, 0);
  plc_io_register(io, IO_TYPE_AO, "AO_0", NULL, 0);
  printf("[Win32] I/O: 2 DI + 2 DO + 1 AI + 1 AO (虚拟)\n");

  /* 加载生成代码 */
  int ret = plc_runtime_load(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[Win32] 加载失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  /* 启动运行时 */
  ret = plc_runtime_start(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[Win32] 启动失败: %d\n", ret);
    return EXIT_FAILURE;
  }

  /* 初始化 CNC */
#ifdef BUILD_MOTION
  CncConfig cncCfg;
  memset(&cncCfg, 0, sizeof(cncCfg));
  cncCfg.axisCount = 3;
  cncCfg.groupCount = 1;
  cncCfg.servoCycleSec = 0.001f;
  cncCfg.maxFeedRate = 5000.0f;
  cncCfg.rapidRate = 10000.0f;
  cncCfg.defaultAccel = 500.0f;
  cncCfg.defaultJerk = 5000.0f;
  cncCfg.junctionDeviation = 0.1f;

  if (plc_cnc_init(&g_cnc, &cncCfg) == 0) {
    MotorConfig motorCfg;
    memset(&motorCfg, 0, sizeof(motorCfg));
    motorCfg.drvType = MOTOR_DRV_STEPDIR;
    motorCfg.maxVelocity = 500.0f;
    motorCfg.maxAcceleration = 1000.0f;
    motorCfg.softLimitPos = 200.0f;
    motorCfg.softLimitNeg = -200.0f;
    motorCfg.drv.stepdir.pulsePerMm = 80.0f;
    motorCfg.drv.stepdir.maxPulseFreq = 100000;

    plc_cnc_addAxis(&g_cnc, 0, "X", &motorCfg);
    plc_cnc_addAxis(&g_cnc, 1, "Y", &motorCfg);
    plc_cnc_addAxis(&g_cnc, 2, "Z", &motorCfg);

    for (uint32_t i = 0; i < g_cnc.axisCount; i++) {
      plc_axis_enable(&g_cnc.axes[i], true);
    }
    printf("[CNC] 已初始化: 3 轴 (步进/方向)\n");
  } else {
    printf("[CNC] 初始化失败\n");
  }
#endif

  /* 创建伺服定时器 */
  ret = plc_win_rt_createServo(1000, servo_callback, &g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[Win32] 伺服定时器创建失败\n");
    plc_runtime_stop(&g_runtime);
    return EXIT_FAILURE;
  }

  ret = plc_win_rt_startServo();
  if (ret != 0) {
    fprintf(stderr, "[Win32] 伺服定时器启动失败\n");
    plc_win_rt_destroyServo();
    plc_runtime_stop(&g_runtime);
    return EXIT_FAILURE;
  }

  printf("\n[Win32] 伺服定时器已启动 (1ms)\n");
  printf("[Win32] 系统运行中 (Ctrl+C 退出)\n");

  /* MDI 测试 */
#ifdef BUILD_MOTION
  if (g_cnc.status >= CNC_STATUS_IDLE) {
    printf("\n--- CNC MDI 测试 ---\n");
    plc_cnc_mdi(&g_cnc, "G1 X50 Y50 F500");
    printf("[CNC] MDI 队列: %u 段\n", plc_planner_queued(&g_cnc.planner));
  }
#endif

  /* 主循环 */
  uint32_t lastStatsTick = plc_platform_tick_ms();

  while (g_running) {
    uint32_t now = plc_platform_tick_ms();

    if (now - lastStatsTick >= 5000) {
      print_runtime_stats();
      lastStatsTick = now;
    }

    print_cnc_status();

    Sleep(100);
  }

  printf("\n[Win32] 正在停止...\n");

  /* 停止伺服定时器 */
  plc_win_rt_stopServo();
  plc_win_rt_destroyServo();

  /* 停止 PLC 运行时 */
  plc_runtime_stop(&g_runtime);

  /* 销毁 CNC */
#ifdef BUILD_MOTION
  if (g_cnc.status >= CNC_STATUS_IDLE) {
    plc_cnc_deinit(&g_cnc);
  }
#endif

  printf("[Win32] 系统已停止\n");
  return EXIT_SUCCESS;
}
