/**
 * hmi_fb.c - Linux帧缓冲显示驱动
 *
 * 通过/dev/fb0实现帧缓冲区直接显示
 * 支持触摸屏输入（/dev/input/eventX）
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"
#include "plc_hmi_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <signal.h>

/* ========== 帧缓冲区状态 ========== */

static int     g_fb_fd = -1;
static void*   g_fb_mem = NULL;
static size_t  g_fb_size = 0;
static uint16_t g_fb_w = 0;
static uint16_t g_fb_h = 0;
static uint8_t  g_fb_bpp = 0;

static struct fb_var_screeninfo g_vinfo;
static struct fb_fix_screeninfo g_finfo;

/* ========== 帧缓冲驱动 ========== */

static int fbdev_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  const char* fb_path = "/dev/fb0";
  g_fb_fd = open(fb_path, O_RDWR);
  if (g_fb_fd < 0) {
    fprintf(stderr, "[HMI-FB] 无法打开 %s\n", fb_path);
    return -1;
  }

  /* 获取屏幕信息 */
  if (ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &g_vinfo) < 0) {
    fprintf(stderr, "[HMI-FB] 获取屏幕信息失败\n");
    close(g_fb_fd);
    g_fb_fd = -1;
    return -2;
  }

  ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &g_finfo);

  g_fb_w = g_vinfo.xres;
  g_fb_h = g_vinfo.yres;
  g_fb_bpp = g_vinfo.bits_per_pixel;

  printf("[HMI-FB] 帧缓冲: %dx%d, %dbpp, stride=%u\n",
         g_fb_w, g_fb_h, g_fb_bpp, g_finfo.line_length);

  g_fb_size = g_finfo.line_length * g_fb_h;
  g_fb_mem = mmap(NULL, g_fb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  g_fb_fd, 0);
  if (g_fb_mem == MAP_FAILED) {
    fprintf(stderr, "[HMI-FB] mmap失败\n");
    close(g_fb_fd);
    g_fb_fd = -1;
    return -3;
  }

  /* 清屏 */
  memset(g_fb_mem, 0, g_fb_size);

  return 0;
}

static void fbdev_deinit(void)
{
  if (g_fb_mem && g_fb_mem != MAP_FAILED) {
    munmap(g_fb_mem, g_fb_size);
    g_fb_mem = NULL;
  }
  if (g_fb_fd >= 0) {
    close(g_fb_fd);
    g_fb_fd = -1;
  }
}

static void fbdev_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  if (!g_fb_mem || !fb) return;

  /* 格式匹配时直接拷贝 */
  if (bpp == g_fb_bpp) {
    uint32_t copy_w = (w < g_fb_w) ? w : g_fb_w;
    uint32_t copy_h = (h < g_fb_h) ? h : g_fb_h;
    uint32_t src_stride = (uint32_t)w * (bpp / 8);
    uint32_t dst_stride = g_finfo.line_length;

    for (uint32_t row = 0; row < copy_h; row++) {
      memcpy((uint8_t*)g_fb_mem + row * dst_stride,
             (const uint8_t*)fb + row * src_stride,
             copy_w * (bpp / 8));
    }
  }
}

static uint16_t fbdev_get_width(void)  { return g_fb_w; }
static uint16_t fbdev_get_height(void) { return g_fb_h; }

static const PlcHmiDriver g_fbdev_driver = {
  "linux-fbdev",
  fbdev_init,
  fbdev_deinit,
  fbdev_flush,
  fbdev_get_width,
  fbdev_get_height
};

/* ========== 触摸输入驱动 ========== */

static int  g_input_fd = -1;

static int touch_init(void)
{
  /* 尝试多个常见的输入设备 */
  const char* devices[] = {
    "/dev/input/event0",
    "/dev/input/event1",
    "/dev/input/event2",
    "/dev/input/event3",
    NULL
  };

  for (int i = 0; devices[i]; i++) {
    g_input_fd = open(devices[i], O_RDONLY | O_NONBLOCK);
    if (g_input_fd >= 0) {
      printf("[HMI-FB] 触摸设备: %s\n", devices[i]);
      return 0;
    }
  }

  fprintf(stderr, "[HMI-FB] 未找到触摸设备\n");
  return -1;
}

static void touch_poll_event(PlcHmiInputEvent* event, bool* has_event)
{
  *has_event = false;
  if (g_input_fd < 0) return;

  struct input_event ie;
  ssize_t n = read(g_input_fd, &ie, sizeof(ie));
  if (n != sizeof(ie)) return;

  if (ie.type == EV_ABS) {
    if (ie.code == ABS_X) {
      event->type = PLC_HMI_INPUT_TOUCH;
      event->x = (int16_t)ie.value;
      event->pressed = true;
      *has_event = true;
    } else if (ie.code == ABS_Y) {
      event->type = PLC_HMI_INPUT_TOUCH;
      event->y = (int16_t)ie.value;
      event->pressed = true;
      *has_event = true;
    }
  } else if (ie.type == EV_KEY && ie.code == BTN_TOUCH) {
    event->type = PLC_HMI_INPUT_TOUCH;
    event->pressed = (ie.value != 0);
    *has_event = true;
  }
}

static void touch_deinit(void)
{
  if (g_input_fd >= 0) {
    close(g_input_fd);
    g_input_fd = -1;
  }
}

static const PlcHmiInputDriver g_touch_driver = {
  "fbdev-touch",
  touch_init,
  touch_poll_event,
  touch_deinit
};

/* ========== 导出初始化函数 ========== */

int plc_hmi_fbdev_register(void)
{
  plc_hmi_driver_register(PLC_HMI_DRV_FBDEV, &g_fbdev_driver);
  plc_hmi_input_register_driver(PLC_HMI_INPUT_TOUCH, &g_touch_driver);
  return 0;
}
