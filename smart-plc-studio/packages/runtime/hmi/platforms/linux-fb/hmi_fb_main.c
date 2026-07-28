/**
 * hmi_fb_main.c - Linux帧缓冲主程序
 *
 * 初始化帧缓冲驱动和HMI系统，运行主循环
 * 支持信号处理用于清理资源
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"

/* 外部函数：注册fbdev驱动 */
extern int plc_hmi_fbdev_register(void);

/* 外部函数：加载演示界面 */
extern void plc_hmi_demo_init(void);
extern void plc_hmi_demo_update(void* var_table, uint32_t size);

static volatile int g_running = 1;

static void signal_handler(int sig)
{
  (void)sig;
  g_running = 0;
  printf("\n[HMI-FB] 收到信号，正在退出...\n");
}

int main(int argc, char* argv[])
{
  (void)argc; (void)argv;

  /* 注册信号处理 */
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  printf("Smart PLC HMI - Linux Framebuffer\n");
  printf("==================================\n");

  /* 注册fbdev驱动 */
  plc_hmi_fbdev_register();

  /* 初始化HMI配置 */
  PlcHmiConfig cfg = {0};
  cfg.screen_width = 800;
  cfg.screen_height = 480;
  cfg.bpp = 32;
  cfg.fps_target = 60;

  /* 使用模拟PLC变量表 */
  static int32_t var_table[64];
  cfg.var_table = var_table;
  cfg.var_table_size = sizeof(var_table);

  /* 初始化HMI系统 */
  int ret = plc_hmi_init(&cfg);
  if (ret < 0) {
    fprintf(stderr, "[HMI-FB] HMI初始化失败: %d\n", ret);
    return 1;
  }

  /* 初始化显示驱动 */
  ret = plc_hmi_driver_init(PLC_HMI_DRV_FBDEV, 800, 480, 32);
  if (ret < 0) {
    fprintf(stderr, "[HMI-FB] 驱动初始化失败: %d\n", ret);
    return 1;
  }

  /* 加载演示界面 */
  plc_hmi_demo_init();

  /* 启动HMI */
  plc_hmi_start();

  printf("[HMI-FB] 运行中 (Ctrl+C退出)\n");

  /* 主循环: ~60fps */
  while (g_running) {
    plc_hmi_demo_update(var_table, sizeof(var_table));
    plc_hmi_update();
    usleep(16666); /* 16.67ms */
  }

  /* 清理 */
  plc_hmi_stop();
  printf("[HMI-FB] 已退出\n");

  return 0;
}
