#include "plc_motion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(x) Sleep(x)
#else
#include <unistd.h>
#define sleep_ms(x) usleep((x) * 1000)
#endif

/* ==================== 示例 G-Code ==================== */
static const char *gcode_demo[] = {
  "G21 G90 G54",                 /* 公制, 绝对坐标, G54 坐标系 */
  "G17",                         /* XY 平面 */
  "G0 Z5",                       /* 快速定位到安全高度 */
  "G0 X0 Y0",                    /* 快速定位到原点 */
  "G1 Z-1 F100",                 /* 下刀 */
  "G1 X50 F200",                 /* 直线插补到 X50 */
  "G1 Y50",                      /* 直线插补到 Y50 */
  "G1 X0",                       /* 回到 X0 */
  "G1 Y0",                       /* 回到 Y0 */
  "G2 X50 Y0 I25 J0 F150",      /* 圆弧 (G2 CW) */
  "G0 Z5",                       /* 抬刀 */
  "M2",                          /* 程序结束 */
};

#define DEMO_LINES (sizeof(gcode_demo) / sizeof(gcode_demo[0]))

int main(void)
{
  printf("=== PLC-CNC 运动控制系统 示例 ===\n\n");

  /* CNC 配置 */
  CncConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.axisCount = 3;                /* X, Y, Z */
  cfg.groupCount = 1;
  cfg.servoCycleSec = 0.001f;       /* 1ms 伺服周期 */
  cfg.maxFeedRate = 5000.0f;        /* 5000 mm/min */
  cfg.rapidRate = 10000.0f;         /* 10000 mm/min */
  cfg.defaultAccel = 500.0f;        /* 500 mm/s² */
  cfg.defaultJerk = 5000.0f;        /* 5000 mm/s³ */
  cfg.junctionDeviation = 0.1f;     /* G64 拐角偏差 0.1mm */

  /* 初始化 CNC 系统 */
  CncSystem cnc;
  if (plc_cnc_init(&cnc, &cfg) != 0) {
    printf("CNC 初始化失败!\n");
    return -1;
  }

  /* 添加轴 */
  MotorConfig motorCfg;
  memset(&motorCfg, 0, sizeof(motorCfg));
  motorCfg.drvType = MOTOR_DRV_STEPDIR;
  motorCfg.maxVelocity = 500.0f;
  motorCfg.maxAcceleration = 1000.0f;
  motorCfg.softLimitPos = 200.0f;
  motorCfg.softLimitNeg = -200.0f;
  motorCfg.drv.stepdir.pulsePerMm = 80.0f;
  motorCfg.drv.stepdir.maxPulseFreq = 100000;

  plc_cnc_addAxis(&cnc, AXIS_X, "X", &motorCfg);
  plc_cnc_addAxis(&cnc, AXIS_Y, "Y", &motorCfg);
  plc_cnc_addAxis(&cnc, AXIS_Z, "Z", &motorCfg);

  /* 使能所有轴 */
  for (uint32_t i = 0; i < cnc.axisCount; i++) {
    plc_axis_enable(&cnc.axes[i], true);
  }

  printf("系统初始化完成\n");
  printf("  轴数: %u\n", cnc.axisCount);
  printf("  运动学: %s\n", plc_kinematics_name(&cnc.kins));
  printf("\n");

  /* 测试: MDI 单行命令 */
  printf("--- MDI 测试: G0 X10 Y10 ---\n");
  plc_cnc_mdi(&cnc, "G0 X10 Y10");
  printf("  规划器队列: %u 段\n", plc_planner_queued(&cnc.planner));

  /* 清空规划器 */
  plc_planner_clear(&cnc.planner);

  /* 测试: 加载 G-Code 并运行 */
  printf("\n--- 加载 G-Code (%u 行) ---\n", (unsigned)DEMO_LINES);
  plc_cnc_loadGCode(&cnc, gcode_demo, (uint32_t)DEMO_LINES);
  plc_cnc_start(&cnc);

  /* 运行循环 (模拟伺服循环) */
  printf("\n--- 运行 G-Code ---\n");
  int lastProgress = -1;
  uint32_t cycleCount = 0;

  while (cnc.status != CNC_STATUS_IDLE && cnc.status != CNC_STATUS_STOP) {
    plc_cnc_update(&cnc, cfg.servoCycleSec);

    /* 每 100 步打印进度 */
    cycleCount++;
    if (cycleCount % 100 == 0) {
      int pct = (int)(plc_interp_getProgress(&cnc.interpolator) * 100);
      if (pct != lastProgress) {
        lastProgress = pct;
        printf("\r  执行进度: %d%% | 线号: %u/%u | 段队列: %u",
               pct,
               (unsigned)cnc.currentLine,
               (unsigned)cnc.gcodeLineCount,
               (unsigned)plc_planner_queued(&cnc.planner));
        fflush(stdout);
      }
    }

    sleep_ms(1);  /* 模拟 1ms 伺服周期 */
  }

  printf("\n\n--- 运行完成 ---\n");
  printf("  总周期数: %u\n", (unsigned)cycleCount);
  printf("  插补步数: %u\n", (unsigned)cnc.interpolator.stepCount);

  /* 打印最终位置 */
  printf("\n--- 最终位置 (工作坐标系) ---\n");
  for (uint32_t i = 0; i < cnc.axisCount; i++) {
    printf("  %s: %.3f mm (指令: %.3f)\n",
           cnc.axes[i].name,
           cnc.axes[i].actualPosition,
           cnc.axes[i].commandPosition);
  }

  /* 清理 */
  plc_cnc_deinit(&cnc);
  printf("\n=== 示例结束 ===\n");

  return 0;
}
