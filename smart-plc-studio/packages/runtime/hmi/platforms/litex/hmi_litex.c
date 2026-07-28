/**
 * hmi_litex.c - LiteX FPGA 显示驱动
 *
 * 将 PLC HMI 帧缓冲区渲染到 LiteX FPGA 视频帧缓冲。
 * 支持两种模式：
 *   1. 直接帧缓冲（FB）：写入 SDRAM 中的像素缓冲区，由 LiteX Video 核心扫描输出
 *   2. LiteX Video Core（VIDEO）：通过 LiteX Video 的 DMA 输出
 *
 * 硬件假设：
 *   - 视频帧缓冲基址：由链接脚本或 CSR 提供
 *   - 像素格式：ARGB8888（32bpp）或 RGB565（16bpp）
 *   - 屏幕尺寸：通常 800x480 或 640x480
 */

#include "plc_hmi.h"
#include "plc_hmi_driver.h"

#ifdef PLC_USE_LITEX

/* ========== 帧缓冲地址 ========== */

/**
 * LiteX 视频帧缓冲基址。
 * 可由链接脚本定义（-DPLC_LITEX_FB_BASE=0x40000000），
 * 或从 LiteX 生成的 mem.h 中读取。
 */
#ifndef PLC_LITEX_FB_BASE
#ifdef __has_include
#if __has_include("mem.h")
#include "mem.h"
#ifndef PLC_LITEX_FB_BASE
#define PLC_LITEX_FB_BASE MEM_MAIN_RAM_BASE
#endif
#else
/* 默认 SDRAM 基址 + 偏移（假设前 4MB 留给程序，之后为帧缓冲） */
#define PLC_LITEX_FB_BASE  0x40400000UL
#endif
#else
#define PLC_LITEX_FB_BASE  0x40400000UL
#endif
#endif

/* ========== 内部状态 ========== */

static uint16_t g_scr_w = 800;
static uint16_t g_scr_h = 480;
static uint8_t  g_scr_bpp = 32;

/* ========== 驱动实现 ========== */

static int litex_fb_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  g_scr_w = w;
  g_scr_h = h;
  g_scr_bpp = bpp;

  /* 帧缓冲位于 FPGA 内存映射地址，无需额外初始化 */
  /* 清屏 */
  volatile uint32_t* fb = (volatile uint32_t*)PLC_LITEX_FB_BASE;
  uint32_t pixel_count = (uint32_t)w * h;
  for (uint32_t i = 0; i < pixel_count; i++) {
    fb[i] = 0xFF000000; /* 黑色背景 */
  }

  plc_platform_log(PLC_LOG_INFO,
    "LiteX FB 驱动初始化: %dx%d@%dbpp FB=0x%08lX",
    w, h, bpp, (unsigned long)PLC_LITEX_FB_BASE);
  return 0;
}

static void litex_fb_deinit(void)
{
  /* LiteX 帧缓冲无需释放 */
}

static void litex_fb_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  if (!fb) return;

  if (bpp == 32) {
    /* ARGB8888 → 直接拷贝到 FPGA 帧缓冲 */
    volatile uint32_t* dst = (volatile uint32_t*)PLC_LITEX_FB_BASE;
    const uint32_t* src = (const uint32_t*)fb;
    uint32_t pixel_count = (uint32_t)w * h;
    for (uint32_t i = 0; i < pixel_count; i++) {
      dst[i] = src[i];
    }
  } else if (bpp == 16) {
    /* RGB565 → 转换为 ARGB8888 */
    volatile uint32_t* dst = (volatile uint32_t*)PLC_LITEX_FB_BASE;
    const uint16_t* src = (const uint16_t*)fb;
    uint32_t pixel_count = (uint32_t)w * h;
    for (uint32_t i = 0; i < pixel_count; i++) {
      uint16_t c = src[i];
      uint8_t r = ((c >> 11) & 0x1F) << 3;
      uint8_t g = ((c >> 5) & 0x3F) << 2;
      uint8_t b = (c & 0x1F) << 3;
      dst[i] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
  }
}

static uint16_t litex_fb_get_width(void)  { return g_scr_w; }
static uint16_t litex_fb_get_height(void) { return g_scr_h; }

static const PlcHmiDriver g_litex_driver = {
  "litex-fb",
  litex_fb_init,
  litex_fb_deinit,
  litex_fb_flush,
  litex_fb_get_width,
  litex_fb_get_height
};

/* ========== 公共接口 ========== */

/**
 * 初始化 LiteX HMI 驱动
 * @param w 屏幕宽度
 * @param h 屏幕高度
 * @return 0=成功
 */
int plc_hmi_litex_init(uint16_t w, uint16_t h)
{
  plc_hmi_driver_register(PLC_HMI_DRV_LITEX, &g_litex_driver);
  return plc_hmi_driver_init(PLC_HMI_DRV_LITEX, w, h, 32);
}

#endif /* PLC_USE_LITEX */
