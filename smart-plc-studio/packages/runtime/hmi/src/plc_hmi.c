/**
 * plc_hmi.c - HMI主引擎
 *
 * 帧缓冲区管理、FPS控制、更新循环、屏幕导航
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_input.h"
#include "plc_hmi_font.h"
#include <string.h>
#include <stdlib.h>

/* ========== 内部状态 ========== */

static struct {
  /* 配置 */
  PlcHmiConfig   config;
  bool           initialized;
  bool           running;

  /* 屏幕 */
  PlcHmiScreen   screen;
  void*          framebuffer;

  /* 帧率控制 */
  uint8_t        target_fps;
  uint8_t        actual_fps;
  uint32_t       last_frame_tick;
  uint32_t       frame_count;
  uint32_t       fps_calc_tick;

  /* 屏幕导航 */
  struct {
    char   name[32];
    uint8_t screen_id;
  } screens[PLC_HMI_MAX_SCREENS];
  uint8_t  screen_count;
  uint8_t  current_screen;
} g_hmi;

/* ========== 初始化 ========== */

int plc_hmi_init(const PlcHmiConfig* config)
{
  if (!config || config->screen_width == 0 || config->screen_height == 0) {
    return -1;
  }

  memset(&g_hmi, 0, sizeof(g_hmi));
  memcpy(&g_hmi.config, config, sizeof(PlcHmiConfig));

  /* 分配帧缓冲区 */
  uint32_t fb_size = (uint32_t)config->screen_width *
                     config->screen_height *
                     (config->bpp / 8);
  g_hmi.framebuffer = malloc(fb_size);
  if (!g_hmi.framebuffer) return -2;
  memset(g_hmi.framebuffer, 0, fb_size);

  /* 配置屏幕 */
  g_hmi.screen.width      = config->screen_width;
  g_hmi.screen.height     = config->screen_height;
  g_hmi.screen.bpp        = config->bpp;
  g_hmi.screen.framebuffer = g_hmi.framebuffer;
  g_hmi.screen.stride     = (uint32_t)config->screen_width * (config->bpp / 8);

  /* 帧率 */
  g_hmi.target_fps = config->fps_target ? config->fps_target : 60;
  if (g_hmi.target_fps > 120) g_hmi.target_fps = 120;
  if (g_hmi.target_fps < 1)   g_hmi.target_fps = 1;

  /* 初始化子系统 */
  plc_hmi_widget_init();
  plc_hmi_input_init();

  /* 默认屏幕导航入口 */
  g_hmi.screen_count = 0;
  g_hmi.current_screen = 0;

  /* 注册默认"main"屏幕 */
  plc_hmi_navigate("main");

  g_hmi.initialized = true;
  return 0;
}

/* ========== 启动/停止 ========== */

int plc_hmi_start(void)
{
  if (!g_hmi.initialized) return -1;

  /* 初始化显示驱动 */
  int ret = plc_hmi_driver_init(PLC_HMI_DRV_RAW,
                                 g_hmi.screen.width,
                                 g_hmi.screen.height,
                                 g_hmi.screen.bpp);
  if (ret < 0) return ret;

  g_hmi.running = true;
  g_hmi.last_frame_tick = plc_platform_tick_ms();
  g_hmi.fps_calc_tick = g_hmi.last_frame_tick;
  g_hmi.frame_count = 0;

  plc_platform_log(PLC_LOG_INFO, "HMI启动: %dx%d@%dfps",
                   g_hmi.screen.width, g_hmi.screen.height, g_hmi.target_fps);
  return 0;
}

void plc_hmi_stop(void)
{
  g_hmi.running = false;
  plc_platform_log(PLC_LOG_INFO, "HMI已停止");
}

/* ========== 更新循环 ========== */

void plc_hmi_update(void)
{
  if (!g_hmi.running) return;

  uint32_t now = plc_platform_tick_ms();
  uint32_t frame_interval = 1000 / g_hmi.target_fps;

  /* 帧率控制 */
  if ((now - g_hmi.last_frame_tick) < frame_interval) {
    return;
  }

  g_hmi.last_frame_tick = now;
  g_hmi.frame_count++;

  /* 计算实际FPS（每秒更新一次） */
  if ((now - g_hmi.fps_calc_tick) >= 1000) {
    g_hmi.actual_fps = (uint8_t)(g_hmi.frame_count * 1000 /
                                  (now - g_hmi.fps_calc_tick));
    g_hmi.frame_count = 0;
    g_hmi.fps_calc_tick = now;
  }

  /* 1. 轮询输入 */
  plc_hmi_input_poll();

  /* 2. 处理输入事件 */
  PlcHmiInputEvent ev;
  while (plc_hmi_input_get_event(&ev)) {
    if (ev.type == PLC_HMI_INPUT_TOUCH || ev.type == PLC_HMI_INPUT_MOUSE) {
      plc_hmi_widget_handle_input(ev.x, ev.y, ev.pressed);
    }
  }

  /* 3. 更新绑定（从PLC变量同步到控件） */
  if (g_hmi.config.var_table) {
    plc_hmi_binding_update_all(g_hmi.config.var_table,
                                g_hmi.config.var_table_size);
  }

  /* 4. 渲染控件 */
  plc_hmi_widget_render(&g_hmi.screen);

  /* 5. 刷新到显示 */
  plc_hmi_driver_flush(g_hmi.screen.framebuffer,
                        g_hmi.screen.width,
                        g_hmi.screen.height,
                        g_hmi.screen.bpp);
}

/* ========== 屏幕查询 ========== */

const PlcHmiScreen* plc_hmi_get_screen(void)
{
  return &g_hmi.screen;
}

/* ========== 帧率 ========== */

void plc_hmi_set_fps(uint8_t fps)
{
  if (fps < 1) fps = 1;
  if (fps > 120) fps = 120;
  g_hmi.target_fps = fps;
}

uint8_t plc_hmi_get_fps(void)
{
  return g_hmi.actual_fps;
}

bool plc_hmi_is_running(void)
{
  return g_hmi.running;
}

/* ========== 绑定PLC变量 ========== */

int plc_hmi_bind_variable(uint8_t screen_id, uint16_t widget_id,
                          const char* var_name, uint8_t direction)
{
  (void)screen_id;
  return plc_hmi_binding_add(widget_id, PLC_HMI_PROP_VALUE,
                              var_name, (PlcHmiBindDirection)direction);
}

/* ========== 截图 ========== */

int plc_hmi_screenshot(const char* filename)
{
  if (!filename || !g_hmi.framebuffer) return -1;

  /* 简易BMP保存 (32bpp) */
  FILE* fp = fopen(filename, "wb");
  if (!fp) return -2;

  uint16_t w = g_hmi.screen.width;
  uint16_t h = g_hmi.screen.height;
  uint32_t row_bytes = w * 4;
  uint32_t pad = (4 - (row_bytes % 4)) % 4;
  uint32_t img_size = (row_bytes + pad) * h;
  uint32_t file_size = 54 + img_size;

  /* BMP文件头 */
  uint8_t header[54];
  memset(header, 0, sizeof(header));
  header[0] = 'B'; header[1] = 'M';
  header[2] = file_size & 0xFF;
  header[3] = (file_size >> 8) & 0xFF;
  header[4] = (file_size >> 16) & 0xFF;
  header[5] = (file_size >> 24) & 0xFF;
  header[10] = 54;
  header[14] = 40;
  header[18] = w & 0xFF;
  header[19] = (w >> 8) & 0xFF;
  header[22] = h & 0xFF;
  header[23] = (h >> 8) & 0xFF;
  header[24] = 1;
  header[26] = 32;
  header[34] = img_size & 0xFF;
  header[35] = (img_size >> 8) & 0xFF;
  header[36] = (img_size >> 16) & 0xFF;
  header[37] = (img_size >> 24) & 0xFF;

  fwrite(header, 1, 54, fp);

  /* 写入像素数据（从底到顶，BMP格式） */
  uint8_t* fb = (uint8_t*)g_hmi.framebuffer;
  uint8_t zero_pad[4] = {0};
  for (int y = h - 1; y >= 0; y--) {
    fwrite(fb + (uint32_t)y * row_bytes, 1, row_bytes, fp);
    if (pad > 0) fwrite(zero_pad, 1, pad, fp);
  }

  fclose(fp);
  plc_platform_log(PLC_LOG_INFO, "截图已保存: %s", filename);
  return 0;
}

/* ========== 屏幕导航 ========== */

int plc_hmi_navigate(const char* screen_name)
{
  if (!screen_name) return -1;

  /* 查找已有屏幕 */
  for (uint8_t i = 0; i < g_hmi.screen_count; i++) {
    if (strcmp(g_hmi.screens[i].name, screen_name) == 0) {
      g_hmi.current_screen = i;
      return 0;
    }
  }

  /* 创建新屏幕 */
  if (g_hmi.screen_count >= PLC_HMI_MAX_SCREENS) return -3;

  uint8_t idx = g_hmi.screen_count;
  strncpy(g_hmi.screens[idx].name, screen_name, 31);
  g_hmi.screens[idx].name[31] = '\0';
  g_hmi.screens[idx].screen_id = idx;
  g_hmi.screen_count++;
  g_hmi.current_screen = idx;

  /* 清屏 */
  if (g_hmi.framebuffer) {
    memset(g_hmi.framebuffer, 0,
           (uint32_t)g_hmi.screen.width * g_hmi.screen.height *
           (g_hmi.screen.bpp / 8));
  }

  return 0;
}
