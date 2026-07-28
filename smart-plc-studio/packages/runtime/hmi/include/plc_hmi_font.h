/**
 * plc_hmi_font.h - 内置位图字体
 *
 * 8x16 ASCII字体，支持字符32-126
 * 提供字符/字符串绘制和尺寸测量功能
 */

#ifndef PLC_HMI_FONT_H
#define PLC_HMI_FONT_H

#include <stdint.h>
#include "plc_hmi.h"

/* ========== 字体常量 ========== */

#define PLC_HMI_FONT_WIDTH   8
#define PLC_HMI_FONT_HEIGHT  16
#define PLC_HMI_FONT_FIRST   32   /* 第一个可显示字符 (空格) */
#define PLC_HMI_FONT_LAST    126  /* 最后一个可显示字符 (~) */
#define PLC_HMI_FONT_COUNT   (PLC_HMI_FONT_LAST - PLC_HMI_FONT_FIRST + 1)

/* ========== 字体接口函数 ========== */

/**
 * 绘制单个字符
 * @param screen 屏幕信息
 * @param x 左上角X
 * @param y 左上角Y
 * @param ch 字符
 * @param color 文字颜色
 * @param scale 缩放倍数 (1=8x16, 2=16x32, ...)
 * @return 字符绘制后的X偏移 (width * scale)
 */
uint16_t plc_hmi_font_draw_char(const PlcHmiScreen* screen,
                                 int16_t x, int16_t y,
                                 char ch, uint32_t color,
                                 uint8_t scale);

/**
 * 绘制字符串
 * @param screen 屏幕信息
 * @param x 起始X
 * @param y 起始Y
 * @param str 字符串
 * @param color 文字颜色
 * @param scale 缩放倍数
 * @return 字符串总宽度像素
 */
uint16_t plc_hmi_font_draw_string(const PlcHmiScreen* screen,
                                   int16_t x, int16_t y,
                                   const char* str, uint32_t color,
                                   uint8_t scale);

/**
 * 获取字符串像素宽度
 * @param str 字符串
 * @param scale 缩放倍数
 * @return 像素宽度
 */
uint16_t plc_hmi_font_get_width(const char* str, uint8_t scale);

/**
 * 获取字体像素高度
 * @param scale 缩放倍数
 * @return 像素高度
 */
uint16_t plc_hmi_font_get_height(uint8_t scale);

/**
 * 获取字符像素宽度
 * @param scale 缩放倍数
 * @return 单字符像素宽度
 */
uint16_t plc_hmi_font_get_char_width(uint8_t scale);

#endif /* PLC_HMI_FONT_H */
