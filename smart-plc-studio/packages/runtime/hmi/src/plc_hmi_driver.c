/**
 * plc_hmi_driver.c - 显示驱动管理
 *
 * 管理显示驱动的注册、初始化和刷新
 */

#include "plc_hmi_driver.h"
#include <string.h>

/* 驱动注册表 */
static const PlcHmiDriver* g_drivers[PLC_HMI_DRV_COUNT];
static PlcHmiDriverType g_active_type = PLC_HMI_DRV_RAW;
static bool g_initialized[PLC_HMI_DRV_COUNT];

/* ========== 内置 RAW 驱动（纯内存帧缓冲） ========== */

static int raw_init(uint16_t w, uint16_t h, uint8_t bpp) {
  (void)w; (void)h; (void)bpp;
  return 0;
}
static void raw_deinit(void) {}
static void raw_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp) {
  (void)fb; (void)w; (void)h; (void)bpp;
}
static uint16_t raw_get_width(void) { return 0; }
static uint16_t raw_get_height(void) { return 0; }

static const PlcHmiDriver g_raw_driver = {
  "raw-fb",
  raw_init,
  raw_deinit,
  raw_flush,
  raw_get_width,
  raw_get_height
};

/* ========== 注册接口 ========== */

int plc_hmi_driver_register(PlcHmiDriverType type, const PlcHmiDriver* driver)
{
  if (type >= PLC_HMI_DRV_COUNT || !driver) return -1;
  g_drivers[type] = driver;
  return 0;
}

/* ========== 初始化 ========== */

int plc_hmi_driver_init(PlcHmiDriverType type, uint16_t w,
                        uint16_t h, uint8_t bpp)
{
  if (type >= PLC_HMI_DRV_COUNT) return -1;

  /* RAW 驱动未注册时自动注册内置实现 */
  if (!g_drivers[PLC_HMI_DRV_RAW]) {
    g_drivers[PLC_HMI_DRV_RAW] = &g_raw_driver;
  }

  const PlcHmiDriver* drv = g_drivers[type];
  if (!drv || !drv->init) {
    /* 回退到RAW模式（不输出到屏幕） */
    if (type != PLC_HMI_DRV_RAW) {
      g_active_type = PLC_HMI_DRV_RAW;
      return 0;
    }
    return -1;
  }

  int ret = drv->init(w, h, bpp);
  if (ret == 0) {
    g_active_type = type;
    g_initialized[type] = true;
  }
  return ret;
}

/* ========== 刷新 ========== */

void plc_hmi_driver_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  const PlcHmiDriver* drv = g_drivers[g_active_type];
  if (drv && drv->flush) {
    drv->flush(fb, w, h, bpp);
  }
}

/* ========== 查询 ========== */

const PlcHmiDriver* plc_hmi_driver_get(PlcHmiDriverType type)
{
  if (type >= PLC_HMI_DRV_COUNT) return NULL;
  return g_drivers[type];
}

PlcHmiDriverType plc_hmi_driver_get_active(void)
{
  return g_active_type;
}
