/**
 * plc_hmi_driver.h - 显示驱动接口
 *
 * 定义帧缓冲区显示驱动的抽象层，支持多种后端：
 * fbdev（Linux帧缓冲）、Win32 GDI、SDL2、LVGL、RAW（原始内存）
 */

#ifndef PLC_HMI_DRIVER_H
#define PLC_HMI_DRIVER_H

#include <stdint.h>

/* ========== 驱动类型枚举 ========== */

typedef enum {
  PLC_HMI_DRV_FBDEV = 0,  /* Linux /dev/fb0 */
  PLC_HMI_DRV_WIN32,      /* Win32 GDI */
  PLC_HMI_DRV_SDL2,       /* SDL2 渲染 */
  PLC_HMI_DRV_LVGL,       /* LVGL 集成 */
  PLC_HMI_DRV_RAW,        /* 原始帧缓冲（仅内存） */
  PLC_HMI_DRV_COUNT
} PlcHmiDriverType;

/* ========== 驱动接口结构 ========== */

/**
 * 显示驱动描述
 * 每个后端需实现此结构中的函数指针
 */
typedef struct {
  const char* name;                                              /* 驱动名称 */
  int  (*init)(uint16_t width, uint16_t height, uint8_t bpp);   /* 初始化显示 */
  void (*deinit)(void);                                          /* 释放资源 */
  void (*flush)(const void* fb, uint16_t w, uint16_t h,         /* 刷新帧缓冲 */
                uint8_t bpp);
  uint16_t (*get_width)(void);                                   /* 获取宽度 */
  uint16_t (*get_height)(void);                                  /* 获取高度 */
} PlcHmiDriver;

/* ========== 全局接口函数 ========== */

/**
 * 注册显示驱动
 * @param type 驱动类型
 * @param driver 驱动实现指针
 * @return 0成功, 负数失败
 */
int plc_hmi_driver_register(PlcHmiDriverType type, const PlcHmiDriver* driver);

/**
 * 初始化指定类型的显示驱动
 * @param type 驱动类型
 * @param width 屏幕宽度
 * @param height 屏幕高度
 * @param bpp 位深度
 * @return 0成功, 负数失败
 */
int plc_hmi_driver_init(PlcHmiDriverType type, uint16_t width,
                        uint16_t height, uint8_t bpp);

/**
 * 刷新帧缓冲到显示
 * @param fb 帧缓冲区指针
 * @param w 宽度
 * @param h 高度
 * @param bpp 位深度
 */
void plc_hmi_driver_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp);

/**
 * 获取指定类型的驱动描述
 * @param type 驱动类型
 * @return 驱动指针，未注册返回NULL
 */
const PlcHmiDriver* plc_hmi_driver_get(PlcHmiDriverType type);

/**
 * 获取当前活动的驱动类型
 * @return 当前驱动类型
 */
PlcHmiDriverType plc_hmi_driver_get_active(void);

#endif /* PLC_HMI_DRIVER_H */
