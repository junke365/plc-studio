/**
 * hmi_sdl2_main.c - SDL2模拟器主程序
 *
 * 完整的SDL2 HMI模拟器，包含：
 * - PLC变量模拟（正弦波+定时器）
 * - FPS计数器覆盖层
 * - 变量检查器面板
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_driver.h"

#ifdef PLC_USE_SDL2

/* 外部函数 */
extern int  plc_hmi_sdl2_register(void);
extern int  plc_hmi_sdl2_poll_events(void);
extern void plc_hmi_demo_init(void);
extern void plc_hmi_demo_update(void* var_table, uint32_t size);

/* 模拟PLC变量表 */
#define VAR_TABLE_SIZE  64
static int32_t g_var_table[VAR_TABLE_SIZE];

/* 变量索引定义（与demo对应） */
#define VAR_MOTOR_RUN     0   /* bool: 电机运行 */
#define VAR_MOTOR_SPEED   1   /* int: 转速 RPM */
#define VAR_TEMPERATURE   2   /* int: 温度 *10 */
#define VAR_PROGRESS      3   /* int: 进度 0-100 */
#define VAR_TREND_VAL     4   /* int: 趋势值 */
#define VAR_STATUS_LED1   5   /* bool: LED1 */
#define VAR_STATUS_LED2   6   /* bool: LED2 */

/* 模拟更新计数器 */
static uint32_t g_tick = 0;
static bool     g_motor_running = false;
static uint32_t g_last_tick = 0;

static void simulate_plc_variables(void)
{
  g_tick++;

  /* 电机转速模拟: 正弦波 1000-2000 RPM */
  if (g_motor_running) {
    double rad = (double)g_tick * 0.05;
    g_var_table[VAR_MOTOR_SPEED] = (int32_t)(1500.0 + 500.0 * sin(rad));
  } else {
    g_var_table[VAR_MOTOR_SPEED] = 0;
  }

  /* 温度模拟: 慢速正弦 25-35°C (x10) */
  {
    double rad = (double)g_tick * 0.01;
    g_var_table[VAR_TEMPERATURE] = (int32_t)(300.0 + 50.0 * sin(rad));
  }

  /* 进度模拟: 锯齿波 0-100 */
  g_var_table[VAR_PROGRESS] = (int32_t)(g_tick % 101);

  /* 趋势值: 随机扰动的正弦 */
  {
    double rad = (double)g_tick * 0.08;
    int32_t noise = (int32_t)(g_tick * 7 % 20) - 10;
    g_var_table[VAR_TREND_VAL] = (int32_t)(50.0 + 30.0 * sin(rad)) + noise;
  }

  /* LED闪烁 */
  g_var_table[VAR_STATUS_LED1] = (g_tick / 30) % 2;
  g_var_table[VAR_STATUS_LED2] = (g_tick / 50) % 2;
}

/* 处理按钮回调 - 电机启停 */
static void on_motor_start(PlcHmiWidget* wd)
{
  (void)wd;
  g_motor_running = true;
  g_var_table[VAR_MOTOR_RUN] = 1;
  printf("[SIM] 电机启动\n");
}

static void on_motor_stop(PlcHmiWidget* wd)
{
  (void)wd;
  g_motor_running = false;
  g_var_table[VAR_MOTOR_RUN] = 0;
  printf("[SIM] 电机停止\n");
}

int main(int argc, char* argv[])
{
  (void)argc; (void)argv;

  printf("Smart PLC HMI - SDL2 Simulator\n");
  printf("==============================\n");

  /* 清零变量表 */
  memset(g_var_table, 0, sizeof(g_var_table));

  /* 注册SDL2驱动 */
  plc_hmi_sdl2_register();

  /* 初始化HMI */
  PlcHmiConfig cfg = {0};
  cfg.screen_width = 800;
  cfg.screen_height = 480;
  cfg.bpp = 32;
  cfg.fps_target = 60;
  cfg.var_table = g_var_table;
  cfg.var_table_size = sizeof(g_var_table);

  if (plc_hmi_init(&cfg) < 0) {
    fprintf(stderr, "HMI初始化失败\n");
    return 1;
  }

  if (plc_hmi_driver_init(PLC_HMI_DRV_SDL2, 800, 480, 32) < 0) {
    fprintf(stderr, "SDL2驱动初始化失败\n");
    return 1;
  }

  /* 加载演示界面 */
  plc_hmi_demo_init();

  /* 绑定按钮回调 */
  PlcHmiWidget* btn_start = plc_hmi_widget_get(1); /* 假设ID 1是启动按钮 */
  PlcHmiWidget* btn_stop  = plc_hmi_widget_get(2); /* 假设ID 2是停止按钮 */
  if (btn_start) btn_start->on_click = on_motor_start;
  if (btn_stop)  btn_stop->on_click = on_motor_stop;

  plc_hmi_start();

  g_last_tick = SDL_GetTicks();

  printf("运行中 (关闭窗口或按ESC退出)\n");

  /* 主循环 */
  bool running = true;
  while (running) {
    /* 处理SDL事件 */
    if (plc_hmi_sdl2_poll_events() != 0) {
      running = false;
    }

    /* 检查ESC键退出 */
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_ESCAPE]) {
      running = false;
    }

    /* 模拟PLC变量 */
    simulate_plc_variables();

    /* 更新HMI */
    plc_hmi_update();

    /* ~60fps */
    SDL_Delay(16);
  }

  plc_hmi_stop();
  printf("已退出\n");

  return 0;
}

#endif /* PLC_USE_SDL2 */
