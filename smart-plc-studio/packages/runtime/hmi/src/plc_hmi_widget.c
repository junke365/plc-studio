/**
 * plc_hmi_widget.c - 控件系统实现
 *
 * 控件创建/销毁、各类型渲染、绘图原语、命中测试、属性操作、绑定同步
 */

#include "plc_hmi_widget.h"
#include "plc_hmi_font.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ========== 内置8x16字体数据 ========== */
/* 字符32-126，每字符16字节，按行扫描（MSB在左） */

static const uint8_t g_font_data[PLC_HMI_FONT_COUNT][PLC_HMI_FONT_HEIGHT] = {
  /* 32 空格 */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 33 ! */    {0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
  /* 34 " */    {0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 35 # */    {0x00,0x00,0x00,0x6C,0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
  /* 36 $ */    {0x18,0x18,0x7C,0xC6,0xC2,0xC0,0x7C,0x06,0x06,0x86,0xC6,0x7C,0x18,0x18,0x00,0x00},
  /* 37 % */    {0x00,0x00,0x00,0x00,0xC2,0xC6,0x0C,0x18,0x30,0x60,0xC6,0x86,0x00,0x00,0x00,0x00},
  /* 38 & */    {0x00,0x00,0x38,0x6C,0x6C,0x38,0x76,0xDC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
  /* 39 ' */    {0x00,0x30,0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 40 ( */    {0x00,0x00,0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x18,0x0C,0x00,0x00,0x00,0x00},
  /* 41 ) */    {0x00,0x00,0x30,0x18,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x18,0x30,0x00,0x00,0x00,0x00},
  /* 42 * */    {0x00,0x00,0x00,0x00,0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 43 + */    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 44 , */    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x30,0x00,0x00,0x00},
  /* 45 - */    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 46 . */    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
  /* 47 / */    {0x00,0x00,0x00,0x00,0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,0x00,0x00,0x00},
  /* 48 0 */    {0x00,0x00,0x7C,0xC6,0xC6,0xCE,0xDE,0xF6,0xE6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 49 1 */    {0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00},
  /* 50 2 */    {0x00,0x00,0x7C,0xC6,0x06,0x0C,0x18,0x30,0x60,0xC0,0xC6,0xFE,0x00,0x00,0x00,0x00},
  /* 51 3 */    {0x00,0x00,0x7C,0xC6,0x06,0x06,0x3C,0x06,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 52 4 */    {0x00,0x00,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x0C,0x1E,0x00,0x00,0x00,0x00},
  /* 53 5 */    {0x00,0x00,0xFE,0xC0,0xC0,0xC0,0xFC,0x06,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 54 6 */    {0x00,0x00,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 55 7 */    {0x00,0x00,0xFE,0xC6,0x06,0x06,0x0C,0x18,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00},
  /* 56 8 */    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 57 9 */    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7E,0x06,0x06,0x06,0x0C,0x78,0x00,0x00,0x00,0x00},
  /* 58 : */    {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
  /* 59 ; */    {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00},
  /* 60 < */    {0x00,0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,0x00},
  /* 61 = */    {0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 62 > */    {0x00,0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,0x00},
  /* 63 ? */    {0x00,0x00,0x7C,0xC6,0xC6,0x0C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
  /* 64 @ */    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xDE,0xDE,0xDE,0xDC,0xC0,0x7C,0x00,0x00,0x00,0x00},
  /* 65 A */    {0x00,0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00},
  /* 66 B */    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0xFC,0x00,0x00,0x00,0x00},
  /* 67 C */    {0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0x00,0x00,0x00,0x00},
  /* 68 D */    {0x00,0x00,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0x00,0x00,0x00,0x00},
  /* 69 E */    {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00},
  /* 70 F */    {0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00},
  /* 71 G */    {0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xDE,0xC6,0xC6,0x66,0x3A,0x00,0x00,0x00,0x00},
  /* 72 H */    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00},
  /* 73 I */    {0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00},
  /* 74 J */    {0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0xCC,0xCC,0xCC,0x78,0x00,0x00,0x00,0x00},
  /* 75 K */    {0x00,0x00,0xE6,0x66,0x6C,0x6C,0x78,0x78,0x6C,0x66,0x66,0xE6,0x00,0x00,0x00,0x00},
  /* 76 L */    {0x00,0x00,0xF0,0x60,0x60,0x60,0x60,0x60,0x60,0x62,0x66,0xFE,0x00,0x00,0x00,0x00},
  /* 77 M */    {0x00,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00},
  /* 78 N */    {0x00,0x00,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00},
  /* 79 O */    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 80 P */    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00},
  /* 81 Q */    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0C,0x0E,0x00,0x00},
  /* 82 R */    {0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0x66,0xE6,0x00,0x00,0x00,0x00},
  /* 83 S */    {0x00,0x00,0x7C,0xC6,0xC6,0x60,0x38,0x0C,0x06,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 84 T */    {0x00,0x00,0xFF,0xDB,0x99,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00},
  /* 85 U */    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 86 V */    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,0x00},
  /* 87 W */    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xD6,0xD6,0xD6,0xFE,0xEE,0x6C,0x00,0x00,0x00,0x00},
  /* 88 X */    {0x00,0x00,0xC6,0xC6,0x6C,0x7C,0x38,0x38,0x7C,0x6C,0xC6,0xC6,0x00,0x00,0x00,0x00},
  /* 89 Y */    {0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00},
  /* 90 Z */    {0x00,0x00,0xFE,0xC6,0x86,0x0C,0x18,0x30,0x60,0xC2,0xC6,0xFE,0x00,0x00,0x00,0x00},
  /* 91 [ */    {0x00,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,0x00,0x00,0x00},
  /* 92 \ */    {0x00,0x00,0x00,0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,0x00,0x00,0x00,0x00},
  /* 93 ] */    {0x00,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,0x00,0x00,0x00},
  /* 94 ^ */    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 95 _ */    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},
  /* 96 ` */    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  /* 97 a */    {0x00,0x00,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
  /* 98 b */    {0x00,0x00,0xE0,0x60,0x60,0x78,0x6C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00},
  /* 99 c */    {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 100 d */   {0x00,0x00,0x1C,0x0C,0x0C,0x3C,0x6C,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
  /* 101 e */   {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xFE,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 102 f */   {0x00,0x00,0x1C,0x36,0x32,0x30,0x78,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0x00},
  /* 103 g */   {0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,0x00,0x00},
  /* 104 h */   {0x00,0x00,0xE0,0x60,0x60,0x6C,0x76,0x66,0x66,0x66,0x66,0xE6,0x00,0x00,0x00,0x00},
  /* 105 i */   {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00},
  /* 106 j */   {0x00,0x00,0x06,0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00,0x00},
  /* 107 k */   {0x00,0x00,0xE0,0x60,0x60,0x66,0x6C,0x78,0x78,0x6C,0x66,0xE6,0x00,0x00,0x00,0x00},
  /* 108 l */   {0x00,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00},
  /* 109 m */   {0x00,0x00,0x00,0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0xD6,0xC6,0x00,0x00,0x00,0x00},
  /* 110 n */   {0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00},
  /* 111 o */   {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 112 p */   {0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,0x00},
  /* 113 q */   {0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0x0C,0x1E,0x00,0x00},
  /* 114 r */   {0x00,0x00,0x00,0x00,0x00,0xDC,0x76,0x66,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,0x00},
  /* 115 s */   {0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00,0x00,0x00,0x00},
  /* 116 t */   {0x00,0x00,0x10,0x30,0x30,0xFC,0x30,0x30,0x30,0x30,0x36,0x1C,0x00,0x00,0x00,0x00},
  /* 117 u */   {0x00,0x00,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,0x00},
  /* 118 v */   {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,0x00},
  /* 119 w */   {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xD6,0xD6,0xD6,0xFE,0x6C,0x00,0x00,0x00,0x00},
  /* 120 x */   {0x00,0x00,0x00,0x00,0x00,0xC6,0x6C,0x38,0x38,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},
  /* 121 y */   {0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0x7E,0x06,0x0C,0xF8,0x00,0x00},
  /* 122 z */   {0x00,0x00,0x00,0x00,0x00,0xFE,0xCC,0x18,0x30,0x60,0xC6,0xFE,0x00,0x00,0x00,0x00},
  /* 123 { */   {0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00},
  /* 124 | */   {0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00},
  /* 125 } */   {0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00},
  /* 126 ~ */   {0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* ========== 控件存储 ========== */

static PlcHmiWidget g_widgets[PLC_HMI_MAX_WIDGETS];
static bool         g_widget_used[PLC_HMI_MAX_WIDGETS];
static uint16_t     g_widget_count = 0;

/* ========== 绑定存储 ========== */

static PlcHmiBinding g_bindings[PLC_HMI_MAX_BINDINGS];
static uint16_t      g_binding_count = 0;

/* ========== 绘图原语 ========== */

/** 像素写入（ARGB8888） */
static inline void put_pixel(uint8_t* fb, uint32_t stride,
                              int16_t x, int16_t y,
                              uint16_t w, uint16_t h,
                              uint32_t color)
{
  if (x < 0 || y < 0 || x >= w || y >= h) return;
  uint32_t offset = (uint32_t)y * stride + (uint32_t)x * 4;
  *(uint32_t*)(fb + offset) = color;
}

/** 填充矩形 */
static void fill_rect(const PlcHmiScreen* scr,
                       int16_t x, int16_t y,
                       uint16_t w, uint16_t h,
                       uint32_t color)
{
  uint8_t* fb = (uint8_t*)scr->framebuffer;
  for (uint16_t row = 0; row < h; row++) {
    int16_t py = y + row;
    if (py < 0 || py >= scr->height) continue;
    uint32_t offset = (uint32_t)py * scr->stride + (uint32_t)x * 4;
    uint32_t line_w = w;
    if (x + line_w > scr->width) line_w = scr->width - x;
    for (uint32_t col = 0; col < line_w; col++) {
      *(uint32_t*)(fb + offset + col * 4) = color;
    }
  }
}

/** 水平线 */
static void draw_hline(const PlcHmiScreen* scr,
                        int16_t x, int16_t y, uint16_t w,
                        uint32_t color)
{
  fill_rect(scr, x, y, w, 1, color);
}

/** 垂直线 */
static void draw_vline(const PlcHmiScreen* scr,
                        int16_t x, int16_t y, uint16_t h,
                        uint32_t color)
{
  fill_rect(scr, x, y, 1, h, color);
}

/** 画边框矩形 */
static void draw_rect(const PlcHmiScreen* scr,
                       int16_t x, int16_t y,
                       uint16_t w, uint16_t h,
                       uint32_t color, uint8_t thickness)
{
  for (uint8_t t = 0; t < thickness; t++) {
    draw_hline(scr, x + t, y + t, w - 2 * t, color);
    draw_hline(scr, x + t, y + h - 1 - t, w - 2 * t, color);
    draw_vline(scr, x + t, y + t, h - 2 * t, color);
    draw_vline(scr, x + w - 1 - t, y + t, h - 2 * t, color);
  }
}

/** 填充圆（Bresenham） */
static void draw_circle_filled(const PlcHmiScreen* scr,
                                int16_t cx, int16_t cy,
                                uint16_t r, uint32_t color)
{
  int16_t x = 0, y = (int16_t)r;
  int16_t d = 1 - (int16_t)r;

  while (x <= y) {
    draw_hline(scr, cx - x, cy + y, 2 * x + 1, color);
    draw_hline(scr, cx - x, cy - y, 2 * x + 1, color);
    draw_hline(scr, cx - y, cy + x, 2 * y + 1, color);
    draw_hline(scr, cx - y, cy - x, 2 * y + 1, color);
    x++;
    if (d < 0) {
      d += 2 * x + 1;
    } else {
      y--;
      d += 2 * (x - y) + 1;
    }
  }
}

/** 画空心圆 */
static void draw_circle(const PlcHmiScreen* scr,
                         int16_t cx, int16_t cy,
                         uint16_t r, uint32_t color)
{
  int16_t x = 0, y = (int16_t)r;
  int16_t d = 1 - (int16_t)r;

  while (x <= y) {
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx + x, cy + y, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx - x, cy + y, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx + x, cy - y, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx - x, cy - y, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx + y, cy + x, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx - y, cy + x, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx + y, cy - x, scr->width, scr->height, color);
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, cx - y, cy - x, scr->width, scr->height, color);
    x++;
    if (d < 0) {
      d += 2 * x + 1;
    } else {
      y--;
      d += 2 * (x - y) + 1;
    }
  }
}

/* ========== 字体渲染 ========== */

uint16_t plc_hmi_font_draw_char(const PlcHmiScreen* scr,
                                 int16_t x, int16_t y,
                                 char ch, uint32_t color,
                                 uint8_t scale)
{
  if (ch < PLC_HMI_FONT_FIRST || ch > PLC_HMI_FONT_LAST) {
    return PLC_HMI_FONT_WIDTH * scale;
  }

  uint16_t idx = (uint16_t)(ch - PLC_HMI_FONT_FIRST);
  const uint8_t* glyph = g_font_data[idx];

  for (uint8_t row = 0; row < PLC_HMI_FONT_HEIGHT; row++) {
    uint8_t bits = glyph[row];
    for (uint8_t col = 0; col < 8; col++) {
      if (bits & (0x80 >> col)) {
        /* 绘制缩放像素块 */
        for (uint8_t sy = 0; sy < scale; sy++) {
          for (uint8_t sx = 0; sx < scale; sx++) {
            put_pixel((uint8_t*)scr->framebuffer, scr->stride,
                      x + (int16_t)(col * scale + sx),
                      y + (int16_t)(row * scale + sy),
                      scr->width, scr->height, color);
          }
        }
      }
    }
  }
  return PLC_HMI_FONT_WIDTH * scale;
}

uint16_t plc_hmi_font_draw_string(const PlcHmiScreen* scr,
                                   int16_t x, int16_t y,
                                   const char* str, uint32_t color,
                                   uint8_t scale)
{
  if (!str || !scr) return 0;
  int16_t start_x = x;
  while (*str) {
    x += plc_hmi_font_draw_char(scr, x, y, *str, color, scale);
    str++;
  }
  return (uint16_t)(x - start_x);
}

uint16_t plc_hmi_font_get_width(const char* str, uint8_t scale)
{
  if (!str) return 0;
  uint16_t len = 0;
  while (*str++) len++;
  return len * PLC_HMI_FONT_WIDTH * scale;
}

uint16_t plc_hmi_font_get_height(uint8_t scale)
{
  return PLC_HMI_FONT_HEIGHT * scale;
}

uint16_t plc_hmi_font_get_char_width(uint8_t scale)
{
  return PLC_HMI_FONT_WIDTH * scale;
}

/* ========== 初始化 ========== */

void plc_hmi_widget_init(void)
{
  memset(g_widgets, 0, sizeof(g_widgets));
  memset(g_widget_used, 0, sizeof(g_widget_used));
  memset(g_bindings, 0, sizeof(g_bindings));
  g_widget_count = 0;
  g_binding_count = 0;
}

/* ========== 创建/销毁 ========== */

uint16_t plc_hmi_widget_create(PlcHmiWidgetType type,
                                int16_t x, int16_t y,
                                uint16_t w, uint16_t h)
{
  if (type >= PLC_HMI_WIDGET_COUNT) return PLC_HMI_WIDGET_ID_INVALID;

  /* 查找空闲槽位 */
  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    if (!g_widget_used[i]) {
      PlcHmiWidget* wd = &g_widgets[i];
      memset(wd, 0, sizeof(PlcHmiWidget));
      wd->id = i;
      wd->type = type;
      wd->x = x;
      wd->y = y;
      wd->w = w;
      wd->h = h;
      wd->visible = true;
      wd->enabled = true;
      wd->state = PLC_HMI_STATE_NORMAL;
      wd->parent_id = PLC_HMI_WIDGET_ID_INVALID;
      g_widget_used[i] = true;
      g_widget_count++;

      /* 设置各类型默认值 */
      switch (type) {
        case PLC_HMI_WIDGET_LABEL:
          wd->data.label.font_size = 1;
          wd->data.label.text_color = PLC_HMI_COLOR_WHITE;
          break;
        case PLC_HMI_WIDGET_BUTTON:
          wd->data.button.normal_color = PLC_HMI_ARGB(0xFF, 0x33, 0x66, 0xCC);
          wd->data.button.pressed_color = PLC_HMI_ARGB(0xFF, 0x22, 0x44, 0x88);
          wd->data.button.text_color = PLC_HMI_COLOR_WHITE;
          wd->data.button.font_size = 1;
          break;
        case PLC_HMI_WIDGET_SWITCH:
          wd->data.switch_data.on_color = PLC_HMI_COLOR_GREEN;
          wd->data.switch_data.off_color = PLC_HMI_COLOR_GRAY;
          break;
        case PLC_HMI_WIDGET_SLIDER:
          wd->data.slider.min_val = 0;
          wd->data.slider.max_val = 100;
          wd->data.slider.track_color = PLC_HMI_COLOR_DARK_GRAY;
          wd->data.slider.fill_color = PLC_HMI_COLOR_BLUE;
          break;
        case PLC_HMI_WIDGET_GAUGE:
          wd->data.gauge.min_val = 0;
          wd->data.gauge.max_val = 100;
          wd->data.gauge.needle_color = PLC_HMI_COLOR_RED;
          wd->data.gauge.scale_color = PLC_HMI_COLOR_WHITE;
          break;
        case PLC_HMI_WIDGET_VALUE_DISPLAY:
          wd->data.value_display.font_size = 2;
          wd->data.value_display.value_color = PLC_HMI_COLOR_CYAN;
          wd->data.value_display.unit_color = PLC_HMI_COLOR_LIGHT_GRAY;
          strcpy(wd->data.value_display.format, "%d");
          break;
        case PLC_HMI_WIDGET_BAR:
          wd->data.bar.min_val = 0;
          wd->data.bar.max_val = 100;
          wd->data.bar.bar_color = PLC_HMI_COLOR_BLUE;
          wd->data.bar.bg_color = PLC_HMI_COLOR_DARK_GRAY;
          wd->data.bar.vertical = false;
          break;
        case PLC_HMI_WIDGET_PROGRESS_BAR:
          wd->data.progress_bar.fill_color = PLC_HMI_COLOR_GREEN;
          wd->data.progress_bar.bg_color = PLC_HMI_COLOR_DARK_GRAY;
          break;
        case PLC_HMI_WIDGET_TREND_CHART:
          wd->data.trend_chart.min_val = 0;
          wd->data.trend_chart.max_val = 100;
          wd->data.trend_chart.line_color = PLC_HMI_COLOR_GREEN;
          wd->data.trend_chart.grid_color = PLC_HMI_ARGB(0xFF, 0x33, 0x33, 0x33);
          break;
        case PLC_HMI_WIDGET_RECTANGLE:
          wd->data.rectangle.fill_color = PLC_HMI_COLOR_DARK_GRAY;
          wd->data.rectangle.border_color = PLC_HMI_COLOR_GRAY;
          wd->data.rectangle.border_width = 1;
          wd->data.rectangle.filled = true;
          break;
        case PLC_HMI_WIDGET_CIRCLE:
          wd->data.circle.fill_color = PLC_HMI_COLOR_BLUE;
          wd->data.circle.border_color = PLC_HMI_COLOR_CYAN;
          wd->data.circle.border_width = 1;
          wd->data.circle.filled = true;
          break;
        default:
          break;
      }
      return i;
    }
  }
  return PLC_HMI_WIDGET_ID_INVALID;
}

void plc_hmi_widget_destroy(uint16_t id)
{
  if (id >= PLC_HMI_MAX_WIDGETS || !g_widget_used[id]) return;

  /* 移除相关绑定 */
  plc_hmi_binding_remove_by_widget(id);

  g_widget_used[id] = false;
  g_widget_count--;
}

PlcHmiWidget* plc_hmi_widget_get(uint16_t id)
{
  if (id >= PLC_HMI_MAX_WIDGETS || !g_widget_used[id]) return NULL;
  return &g_widgets[id];
}

uint16_t plc_hmi_widget_get_count(void)
{
  return g_widget_count;
}

/* ========== 属性操作 ========== */

int plc_hmi_widget_set_prop(uint16_t id, const char* prop, const char* value)
{
  PlcHmiWidget* wd = plc_hmi_widget_get(id);
  if (!wd || !prop || !value) return -1;

  if (strcmp(prop, PLC_HMI_PROP_VISIBLE) == 0) {
    wd->visible = (value[0] == '1' || value[0] == 't');
  } else if (strcmp(prop, PLC_HMI_PROP_ENABLED) == 0) {
    wd->enabled = (value[0] == '1' || value[0] == 't');
  } else if (strcmp(prop, PLC_HMI_PROP_X) == 0) {
    wd->x = (int16_t)atoi(value);
  } else if (strcmp(prop, PLC_HMI_PROP_Y) == 0) {
    wd->y = (int16_t)atoi(value);
  } else if (strcmp(prop, PLC_HMI_PROP_W) == 0) {
    wd->w = (uint16_t)atoi(value);
  } else if (strcmp(prop, PLC_HMI_PROP_H) == 0) {
    wd->h = (uint16_t)atoi(value);
  } else if (strcmp(prop, PLC_HMI_PROP_TEXT) == 0) {
    switch (wd->type) {
      case PLC_HMI_WIDGET_LABEL:
        strncpy(wd->data.label.text, value, PLC_HMI_MAX_STR_LEN - 1);
        break;
      case PLC_HMI_WIDGET_BUTTON:
        strncpy(wd->data.button.text, value, PLC_HMI_MAX_STR_LEN - 1);
        break;
      default:
        break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_VALUE) == 0) {
    int32_t v = atoi(value);
    switch (wd->type) {
      case PLC_HMI_WIDGET_SWITCH:
        wd->data.switch_data.state = (v != 0);
        break;
      case PLC_HMI_WIDGET_SLIDER:
        wd->data.slider.cur_val = v;
        break;
      case PLC_HMI_WIDGET_GAUGE:
        wd->data.gauge.cur_val = v;
        break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY:
        wd->data.value_display.value = v;
        break;
      case PLC_HMI_WIDGET_BAR:
        wd->data.bar.cur_val = v;
        break;
      case PLC_HMI_WIDGET_PROGRESS_BAR:
        wd->data.progress_bar.value = v;
        if (v < 0) v = 0;
        if (v > 100) v = 100;
        snprintf(wd->data.progress_bar.text, PLC_HMI_MAX_STR_LEN, "%d%%", v);
        break;
      case PLC_HMI_WIDGET_TREND_CHART: {
        /* 追加到环形缓冲 */
        PlcHmiTrendChartData* td = &wd->data.trend_chart;
        td->values[td->head] = v;
        td->head = (td->head + 1) % 64;
        if (td->count < 64) td->count++;
        break;
      }
      default:
        break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_MIN) == 0) {
    int32_t v = atoi(value);
    switch (wd->type) {
      case PLC_HMI_WIDGET_SLIDER:   wd->data.slider.min_val = v; break;
      case PLC_HMI_WIDGET_GAUGE:    wd->data.gauge.min_val = v; break;
      case PLC_HMI_WIDGET_BAR:      wd->data.bar.min_val = v; break;
      case PLC_HMI_WIDGET_TREND_CHART: wd->data.trend_chart.min_val = v; break;
      default: break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_MAX) == 0) {
    int32_t v = atoi(value);
    switch (wd->type) {
      case PLC_HMI_WIDGET_SLIDER:   wd->data.slider.max_val = v; break;
      case PLC_HMI_WIDGET_GAUGE:    wd->data.gauge.max_val = v; break;
      case PLC_HMI_WIDGET_BAR:      wd->data.bar.max_val = v; break;
      case PLC_HMI_WIDGET_TREND_CHART: wd->data.trend_chart.max_val = v; break;
      default: break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_COLOR) == 0) {
    uint32_t c = (uint32_t)strtoul(value, NULL, 16);
    switch (wd->type) {
      case PLC_HMI_WIDGET_LABEL:         wd->data.label.text_color = c; break;
      case PLC_HMI_WIDGET_BUTTON:        wd->data.button.text_color = c; break;
      case PLC_HMI_WIDGET_GAUGE:         wd->data.gauge.needle_color = c; break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY: wd->data.value_display.value_color = c; break;
      case PLC_HMI_WIDGET_TREND_CHART:   wd->data.trend_chart.line_color = c; break;
      default: break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_BG_COLOR) == 0) {
    uint32_t c = (uint32_t)strtoul(value, NULL, 16);
    switch (wd->type) {
      case PLC_HMI_WIDGET_BUTTON:       wd->data.button.normal_color = c; break;
      case PLC_HMI_WIDGET_BAR:          wd->data.bar.bg_color = c; break;
      case PLC_HMI_WIDGET_PROGRESS_BAR: wd->data.progress_bar.bg_color = c; break;
      default: break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_FONT_SIZE) == 0) {
    uint8_t s = (uint8_t)atoi(value);
    if (s == 0) s = 1;
    switch (wd->type) {
      case PLC_HMI_WIDGET_LABEL:         wd->data.label.font_size = s; break;
      case PLC_HMI_WIDGET_BUTTON:        wd->data.button.font_size = s; break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY: wd->data.value_display.font_size = s; break;
      default: break;
    }
  } else if (strcmp(prop, PLC_HMI_PROP_UNIT) == 0) {
    if (wd->type == PLC_HMI_WIDGET_VALUE_DISPLAY) {
      strncpy(wd->data.value_display.unit, value, 7);
      wd->data.value_display.unit[7] = '\0';
    }
  } else if (strcmp(prop, PLC_HMI_PROP_FORMAT) == 0) {
    if (wd->type == PLC_HMI_WIDGET_VALUE_DISPLAY) {
      strncpy(wd->data.value_display.format, value, 15);
      wd->data.value_display.format[15] = '\0';
    }
  } else {
    return -2;
  }
  return 0;
}

int plc_hmi_widget_get_prop(uint16_t id, const char* prop,
                            char* buf, uint32_t buf_size)
{
  PlcHmiWidget* wd = plc_hmi_widget_get(id);
  if (!wd || !prop || !buf || buf_size == 0) return -1;

  if (strcmp(prop, PLC_HMI_PROP_VISIBLE) == 0) {
    snprintf(buf, buf_size, "%d", wd->visible ? 1 : 0);
  } else if (strcmp(prop, PLC_HMI_PROP_ENABLED) == 0) {
    snprintf(buf, buf_size, "%d", wd->enabled ? 1 : 0);
  } else if (strcmp(prop, PLC_HMI_PROP_X) == 0) {
    snprintf(buf, buf_size, "%d", wd->x);
  } else if (strcmp(prop, PLC_HMI_PROP_Y) == 0) {
    snprintf(buf, buf_size, "%d", wd->y);
  } else if (strcmp(prop, PLC_HMI_PROP_W) == 0) {
    snprintf(buf, buf_size, "%u", wd->w);
  } else if (strcmp(prop, PLC_HMI_PROP_H) == 0) {
    snprintf(buf, buf_size, "%u", wd->h);
  } else if (strcmp(prop, PLC_HMI_PROP_VALUE) == 0) {
    switch (wd->type) {
      case PLC_HMI_WIDGET_SWITCH:
        snprintf(buf, buf_size, "%d", wd->data.switch_data.state ? 1 : 0);
        break;
      case PLC_HMI_WIDGET_SLIDER:
        snprintf(buf, buf_size, "%d", wd->data.slider.cur_val);
        break;
      case PLC_HMI_WIDGET_GAUGE:
        snprintf(buf, buf_size, "%d", wd->data.gauge.cur_val);
        break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY:
        snprintf(buf, buf_size, "%d", wd->data.value_display.value);
        break;
      case PLC_HMI_WIDGET_BAR:
        snprintf(buf, buf_size, "%d", wd->data.bar.cur_val);
        break;
      case PLC_HMI_WIDGET_PROGRESS_BAR:
        snprintf(buf, buf_size, "%d", wd->data.progress_bar.value);
        break;
      default:
        buf[0] = '\0';
        break;
    }
  } else {
    return -2;
  }
  return 0;
}

void plc_hmi_widget_set_visible(uint16_t id, bool visible)
{
  PlcHmiWidget* wd = plc_hmi_widget_get(id);
  if (wd) wd->visible = visible;
}

void plc_hmi_widget_set_enabled(uint16_t id, bool enabled)
{
  PlcHmiWidget* wd = plc_hmi_widget_get(id);
  if (wd) {
    wd->enabled = enabled;
    if (!enabled) wd->state = PLC_HMI_STATE_DISABLED;
    else if (wd->state == PLC_HMI_STATE_DISABLED) wd->state = PLC_HMI_STATE_NORMAL;
  }
}

/* ========== 命中测试 ========== */

static bool point_in_rect(int16_t px, int16_t py, const PlcHmiWidget* wd)
{
  return (px >= wd->x && px < wd->x + (int16_t)wd->w &&
          py >= wd->y && py < wd->y + (int16_t)wd->h);
}

uint16_t plc_hmi_widget_handle_input(int16_t x, int16_t y, bool pressed)
{
  /* 反向遍历（后创建的控件在上面） */
  for (int16_t i = PLC_HMI_MAX_WIDGETS - 1; i >= 0; i--) {
    if (!g_widget_used[i]) continue;
    PlcHmiWidget* wd = &g_widgets[i];
    if (!wd->visible || !wd->enabled) continue;

    if (point_in_rect(x, y, wd)) {
      if (pressed) {
        wd->state = PLC_HMI_STATE_PRESSED;

        /* 按钮类型触发点击 */
        if (wd->type == PLC_HMI_WIDGET_BUTTON && wd->on_click) {
          wd->on_click(wd);
        }
        /* 开关切换 */
        if (wd->type == PLC_HMI_WIDGET_SWITCH) {
          wd->data.switch_data.state = !wd->data.switch_data.state;
          if (wd->on_change) {
            wd->on_change(wd, wd->data.switch_data.state ? 1 : 0);
          }
        }
        /* 滑块拖动 */
        if (wd->type == PLC_HMI_WIDGET_SLIDER) {
          int32_t range = wd->data.slider.max_val - wd->data.slider.min_val;
          int32_t new_val;
          if (wd->w > wd->h) {
            /* 水平滑块 */
            new_val = wd->data.slider.min_val +
                      (int32_t)((int32_t)(x - wd->x) * range / wd->w);
          } else {
            /* 垂直滑块 */
            new_val = wd->data.slider.max_val -
                      (int32_t)((int32_t)(y - wd->y) * range / wd->h);
          }
          wd->data.slider.cur_val = new_val;
          if (wd->on_change) wd->on_change(wd, new_val);
        }
      } else {
        wd->state = PLC_HMI_STATE_NORMAL;
      }
      return wd->id;
    }
  }
  return PLC_HMI_WIDGET_ID_INVALID;
}

/* ========== 渲染 ========== */

/** 渲染标题栏 */
static void render_title_bar(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  /* 背景 */
  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_ARGB(0xFF, 0x1A, 0x1A, 0x2E));
  /* 底部高亮线 */
  draw_hline(scr, wd->x, wd->y + wd->h - 1, wd->w, PLC_HMI_COLOR_CYAN);
  /* 文本居中 */
  uint16_t tw = plc_hmi_font_get_width(wd->data.label.text, wd->data.label.font_size);
  uint16_t th = plc_hmi_font_get_height(wd->data.label.font_size);
  int16_t tx = wd->x + (int16_t)(wd->w - tw) / 2;
  int16_t ty = wd->y + (int16_t)(wd->h - th) / 2;
  plc_hmi_font_draw_string(scr, tx, ty, wd->data.label.text,
                            wd->data.label.text_color, wd->data.label.font_size);
}

/** 渲染标签 */
static void render_label(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  plc_hmi_font_draw_string(scr, wd->x, wd->y, wd->data.label.text,
                            wd->data.label.text_color, wd->data.label.font_size);
}

/** 渲染按钮 */
static void render_button(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  uint32_t bg = (wd->state == PLC_HMI_STATE_PRESSED) ?
                wd->data.button.pressed_color : wd->data.button.normal_color;
  if (!wd->enabled) bg = PLC_HMI_ARGB(0xFF, 0x55, 0x55, 0x55);

  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, bg);
  draw_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_ARGB(0xFF, 0xFF, 0xFF, 0xFF), 1);

  /* 文本居中 */
  uint8_t fs = wd->data.button.font_size;
  uint16_t tw = plc_hmi_font_get_width(wd->data.button.text, fs);
  uint16_t th = plc_hmi_font_get_height(fs);
  int16_t tx = wd->x + (int16_t)(wd->w - tw) / 2;
  int16_t ty = wd->y + (int16_t)(wd->h - th) / 2;
  plc_hmi_font_draw_string(scr, tx, ty, wd->data.button.text,
                            wd->data.button.text_color, fs);
}

/** 渲染开关 */
static void render_switch(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  uint32_t bg = wd->data.switch_data.state ?
                wd->data.switch_data.on_color : wd->data.switch_data.off_color;
  /* 圆角矩形背景 */
  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_DARK_GRAY);
  draw_rect(scr, wd->x, wd->y, wd->w, wd->h, bg, 2);

  /* 滑块 */
  uint16_t knob_r = wd->h / 2 - 2;
  int16_t knob_x;
  if (wd->data.switch_data.state) {
    knob_x = wd->x + (int16_t)wd->w - (int16_t)knob_r - 2;
  } else {
    knob_x = wd->x + (int16_t)knob_r + 2;
  }
  int16_t knob_y = wd->y + (int16_t)wd->h / 2;
  draw_circle_filled(scr, knob_x, knob_y, knob_r, bg);
}

/** 渲染滑块 */
static void render_slider(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  int32_t range = wd->data.slider.max_val - wd->data.slider.min_val;
  if (range <= 0) range = 1;

  bool horiz = (wd->w >= wd->h);
  uint16_t track_h = horiz ? 6 : wd->w;
  uint16_t track_w = horiz ? wd->w : 6;

  int16_t track_x = wd->x + (horiz ? 0 : (int16_t)(wd->w - track_w) / 2);
  int16_t track_y = wd->y + (horiz ? (int16_t)(wd->h - track_h) / 2 : 0);

  /* 轨道背景 */
  fill_rect(scr, track_x, track_y, track_w, track_h, wd->data.slider.track_color);

  /* 填充部分 */
  int32_t pct = (wd->data.slider.cur_val - wd->data.slider.min_val) * 100 / range;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  if (horiz) {
    uint16_t fill_w = (uint16_t)((uint32_t)track_w * (uint32_t)pct / 100);
    fill_rect(scr, track_x, track_y, fill_w, track_h, wd->data.slider.fill_color);
  } else {
    uint16_t fill_h = (uint16_t)((uint32_t)track_h * (uint32_t)pct / 100);
    int16_t fill_y = track_y + (int16_t)track_h - (int16_t)fill_h;
    fill_rect(scr, track_x, fill_y, track_w, fill_h, wd->data.slider.fill_color);
  }
}

/** 渲染仪表 */
static void render_gauge(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  int16_t cx = wd->x + (int16_t)wd->w / 2;
  int16_t cy = wd->y + (int16_t)wd->h - 4;
  uint16_t r = (wd->w < wd->h ? wd->w : wd->h) / 2 - 4;

  /* 外圆弧背景（半圆） */
  draw_circle(scr, cx, cy, r, wd->data.gauge.scale_color);
  draw_circle(scr, cx, cy, r - 2, wd->data.gauge.scale_color);
  /* 底部填充半圆 */
  fill_rect(scr, cx - (int16_t)r, cy, r * 2, 2, wd->data.gauge.scale_color);

  /* 红区指示 */
  if (wd->data.gauge.redzone_start > wd->data.gauge.min_val) {
    int32_t rz_range = wd->data.gauge.max_val - wd->data.gauge.min_val;
    int32_t rz_pct = (wd->data.gauge.redzone_start - wd->data.gauge.min_val) * 100 / rz_range;
    /* 简化的红区：在右上象限画一条红色弧线 */
    int16_t rz_angle_x = cx + (int16_t)((int32_t)r * 7 / 10);
    int16_t rz_angle_y = cy - (int16_t)((int32_t)r * 7 / 10);
    fill_rect(scr, rz_angle_x, rz_angle_y, (int16_t)r / 2, 3, PLC_HMI_COLOR_RED);
  }

  /* 指针 */
  int32_t val_range = wd->data.gauge.max_val - wd->data.gauge.min_val;
  if (val_range <= 0) val_range = 1;
  int32_t pct = (wd->data.gauge.cur_val - wd->data.gauge.min_val) * 100 / val_range;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  /* 从180°(左)到0°(右)映射 */
  int32_t angle = 180 - pct * 180 / 100;
  /* 简单的三角函数近似（用整数） */
  int32_t rad_x = 0, rad_y = 0;
  if (angle <= 90) {
    rad_x = (int32_t)r * 9 / 10 * (90 - angle) / 90;
    rad_y = -(int32_t)r * 9 / 10 * angle / 90;
  } else {
    rad_x = -(int32_t)r * 9 / 10 * (angle - 90) / 90;
    rad_y = -(int32_t)r * 9 / 10 * (180 - angle) / 90;
  }

  /* 画指针线（简化） */
  int16_t px = cx + (int16_t)rad_x;
  int16_t py = cy + (int16_t)rad_y;
  /* 从中心到指针的线段（Bresenham简化版） */
  int16_t dx = px - cx;
  int16_t dy = py - cy;
  int16_t steps = (dx > -dx ? dx : -dx) > (dy > -dy ? dy : -dy) ?
                  (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy);
  if (steps == 0) steps = 1;
  for (int16_t s = 0; s <= steps; s++) {
    int16_t lx = cx + dx * s / steps;
    int16_t ly = cy + dy * s / steps;
    put_pixel((uint8_t*)scr->framebuffer, scr->stride, lx, ly,
              scr->width, scr->height, wd->data.gauge.needle_color);
  }

  /* 中心圆点 */
  draw_circle_filled(scr, cx, cy, 4, wd->data.gauge.needle_color);

  /* 数值显示在底部 */
  char val_buf[16];
  snprintf(val_buf, sizeof(val_buf), "%d", wd->data.gauge.cur_val);
  uint16_t tw = plc_hmi_font_get_width(val_buf, 1);
  plc_hmi_font_draw_string(scr, cx - (int16_t)tw / 2, cy + 6,
                            val_buf, wd->data.gauge.scale_color, 1);
}

/** 渲染数值显示 */
static void render_value_display(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  char val_buf[32];
  snprintf(val_buf, sizeof(val_buf), wd->data.value_display.format,
           wd->data.value_display.value);
  uint8_t fs = wd->data.value_display.font_size;

  uint16_t vw = plc_hmi_font_get_width(val_buf, fs);
  uint16_t vh = plc_hmi_font_get_height(fs);
  int16_t vx = wd->x + (int16_t)(wd->w - vw) / 2;
  int16_t vy = wd->y;

  plc_hmi_font_draw_string(scr, vx, vy, val_buf,
                            wd->data.value_display.value_color, fs);

  /* 单位 */
  if (wd->data.value_display.unit[0]) {
    int16_t ux = vx + (int16_t)vw + 4;
    plc_hmi_font_draw_string(scr, ux, vy + (int16_t)vh / 4,
                              wd->data.value_display.unit,
                              wd->data.value_display.unit_color, 1);
  }
}

/** 渲染柱状条 */
static void render_bar(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  /* 背景 */
  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, wd->data.bar.bg_color);

  int32_t range = wd->data.bar.max_val - wd->data.bar.min_val;
  if (range <= 0) range = 1;
  int32_t pct = (wd->data.bar.cur_val - wd->data.bar.min_val) * 100 / range;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  if (wd->data.bar.vertical) {
    uint16_t fill_h = (uint16_t)((uint32_t)wd->h * (uint32_t)pct / 100);
    int16_t fill_y = wd->y + (int16_t)wd->h - (int16_t)fill_h;
    fill_rect(scr, wd->x, fill_y, wd->w, fill_h, wd->data.bar.bar_color);
  } else {
    uint16_t fill_w = (uint16_t)((uint32_t)wd->w * (uint32_t)pct / 100);
    fill_rect(scr, wd->x, wd->y, fill_w, wd->h, wd->data.bar.bar_color);
  }

  draw_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_LIGHT_GRAY, 1);
}

/** 渲染进度条 */
static void render_progress_bar(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, wd->data.progress_bar.bg_color);

  int32_t pct = wd->data.progress_bar.value;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  uint16_t fill_w = (uint16_t)((uint32_t)(wd->w - 2) * (uint32_t)pct / 100);
  fill_rect(scr, wd->x + 1, wd->y + 1, fill_w, wd->h - 2, wd->data.progress_bar.fill_color);

  /* 文本 */
  if (wd->data.progress_bar.text[0]) {
    uint16_t tw = plc_hmi_font_get_width(wd->data.progress_bar.text, 1);
    uint16_t th = plc_hmi_font_get_height(1);
    int16_t tx = wd->x + (int16_t)(wd->w - tw) / 2;
    int16_t ty = wd->y + (int16_t)(wd->h - th) / 2;
    plc_hmi_font_draw_string(scr, tx, ty, wd->data.progress_bar.text,
                              PLC_HMI_COLOR_WHITE, 1);
  }

  draw_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_LIGHT_GRAY, 1);
}

/** 渲染趋势图 */
static void render_trend_chart(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  PlcHmiTrendChartData* td = &wd->data.trend_chart;

  /* 背景 */
  fill_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_BLACK);

  /* 网格线 */
  uint8_t grid_rows = 4;
  for (uint8_t i = 0; i <= grid_rows; i++) {
    int16_t gy = wd->y + (int16_t)((uint32_t)wd->h * i / grid_rows);
    draw_hline(scr, wd->x, gy, wd->w, td->grid_color);
  }

  /* 数据线 */
  if (td->count < 2) {
    draw_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_GRAY, 1);
    return;
  }

  int32_t val_range = td->max_val - td->min_val;
  if (val_range <= 0) val_range = 1;

  uint8_t start_idx = (td->head + 64 - td->count) % 64;
  int16_t prev_x = wd->x;
  int32_t prev_val = td->values[start_idx];
  int16_t prev_y = wd->y + (int16_t)wd->h -
                   (int16_t)((int32_t)(prev_val - td->min_val) * wd->h / val_range);

  for (uint8_t i = 1; i < td->count; i++) {
    uint8_t idx = (start_idx + i) % 64;
    int16_t cur_x = wd->x + (int16_t)((uint32_t)wd->w * i / (td->count - 1));
    int16_t cur_y = wd->y + (int16_t)wd->h -
                    (int16_t)((int32_t)(td->values[idx] - td->min_val) * wd->h / val_range);

    /* 简化Bresenham画线 */
    int16_t dx = cur_x - prev_x;
    int16_t dy = cur_y - prev_y;
    int16_t steps = (dx > -dx ? dx : -dx) > (dy > -dy ? dy : -dy) ?
                    (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy);
    if (steps == 0) steps = 1;
    for (int16_t s = 0; s <= steps; s++) {
      int16_t lx = prev_x + dx * s / steps;
      int16_t ly = prev_y + dy * s / steps;
      put_pixel((uint8_t*)scr->framebuffer, scr->stride, lx, ly,
                scr->width, scr->height, td->line_color);
    }
    prev_x = cur_x;
    prev_y = cur_y;
  }

  draw_rect(scr, wd->x, wd->y, wd->w, wd->h, PLC_HMI_COLOR_GRAY, 1);
}

/** 渲染矩形 */
static void render_rectangle(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  if (wd->data.rectangle.filled) {
    fill_rect(scr, wd->x, wd->y, wd->w, wd->h, wd->data.rectangle.fill_color);
  }
  if (wd->data.rectangle.border_width > 0) {
    draw_rect(scr, wd->x, wd->y, wd->w, wd->h,
              wd->data.rectangle.border_color, wd->data.rectangle.border_width);
  }
}

/** 渲染圆形 */
static void render_circle_wd(const PlcHmiScreen* scr, PlcHmiWidget* wd)
{
  int16_t cx = wd->x + (int16_t)wd->w / 2;
  int16_t cy = wd->y + (int16_t)wd->h / 2;
  uint16_t r = (wd->w < wd->h ? wd->w : wd->h) / 2;

  if (wd->data.circle.filled) {
    draw_circle_filled(scr, cx, cy, r, wd->data.circle.fill_color);
  }
  if (wd->data.circle.border_width > 0) {
    for (uint8_t t = 0; t < wd->data.circle.border_width; t++) {
      draw_circle(scr, cx, cy, r - t, wd->data.circle.border_color);
    }
  }
}

/* 主渲染函数 */
void plc_hmi_widget_render(const PlcHmiScreen* scr)
{
  if (!scr || !scr->framebuffer) return;

  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    if (!g_widget_used[i]) continue;
    PlcHmiWidget* wd = &g_widgets[i];
    if (!wd->visible) continue;

    switch (wd->type) {
      case PLC_HMI_WIDGET_LABEL:         render_label(scr, wd); break;
      case PLC_HMI_WIDGET_BUTTON:        render_button(scr, wd); break;
      case PLC_HMI_WIDGET_SWITCH:        render_switch(scr, wd); break;
      case PLC_HMI_WIDGET_SLIDER:        render_slider(scr, wd); break;
      case PLC_HMI_WIDGET_GAUGE:         render_gauge(scr, wd); break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY: render_value_display(scr, wd); break;
      case PLC_HMI_WIDGET_BAR:           render_bar(scr, wd); break;
      case PLC_HMI_WIDGET_PROGRESS_BAR:  render_progress_bar(scr, wd); break;
      case PLC_HMI_WIDGET_TREND_CHART:   render_trend_chart(scr, wd); break;
      case PLC_HMI_WIDGET_RECTANGLE:     render_rectangle(scr, wd); break;
      case PLC_HMI_WIDGET_CIRCLE:        render_circle_wd(scr, wd); break;
      case PLC_HMI_WIDGET_GROUP:         break; /* 组不直接渲染 */
      default: break;
    }
  }
}

/* ========== 绑定系统 ========== */

int plc_hmi_binding_add(uint16_t widget_id, const char* prop,
                        const char* var_name, PlcHmiBindDirection direction)
{
  if (g_binding_count >= PLC_HMI_MAX_BINDINGS) return -1;
  if (!plc_hmi_widget_get(widget_id)) return -2;
  if (!prop || !var_name) return -3;

  /* 检查是否已存在相同绑定 */
  for (uint16_t i = 0; i < g_binding_count; i++) {
    if (g_bindings[i].active &&
        g_bindings[i].widget_id == widget_id &&
        strcmp(g_bindings[i].property, prop) == 0) {
      /* 更新已有绑定 */
      strncpy(g_bindings[i].var_name, var_name, 31);
      g_bindings[i].var_name[31] = '\0';
      g_bindings[i].direction = direction;
      return 0;
    }
  }

  PlcHmiBinding* b = &g_bindings[g_binding_count];
  b->widget_id = widget_id;
  strncpy(b->property, prop, 15);
  b->property[15] = '\0';
  strncpy(b->var_name, var_name, 31);
  b->var_name[31] = '\0';
  b->direction = direction;
  b->active = true;
  g_binding_count++;
  return 0;
}

/**
 * 从变量表中按名称查找变量的偏移量
 * 简化的变量表：每个变量为 "name:value" 格式的连续字符串块
 * 实际PLC运行时中，变量表是按名称索引的内存块
 * 这里使用简化约定：var_name直接作为偏移的字符串标识
 */
static int32_t find_var_value(void* var_table, uint32_t table_size,
                               const char* var_name)
{
  /*
   * 简化实现：变量表为连续的 int32_t 数组
   * var_name 格式: "N" 其中N是变量索引(0-based)
   * 实际项目中应替换为真正的变量名查找逻辑
   */
  if (!var_table || !var_name) return 0;

  uint32_t idx = 0;
  /* 尝试将变量名解析为数字索引 */
  if (var_name[0] >= '0' && var_name[0] <= '9') {
    idx = (uint32_t)atoi(var_name);
  }

  uint32_t max_vars = table_size / sizeof(int32_t);
  if (idx >= max_vars) return 0;

  return ((int32_t*)var_table)[idx];
}

static void set_var_value(void* var_table, uint32_t table_size,
                           const char* var_name, int32_t value)
{
  if (!var_table || !var_name) return;

  uint32_t idx = 0;
  if (var_name[0] >= '0' && var_name[0] <= '9') {
    idx = (uint32_t)atoi(var_name);
  }

  uint32_t max_vars = table_size / sizeof(int32_t);
  if (idx >= max_vars) return;

  ((int32_t*)var_table)[idx] = value;
}

void plc_hmi_binding_update_all(void* var_table, uint32_t var_table_size)
{
  if (!var_table) return;

  for (uint16_t i = 0; i < PLC_HMI_MAX_BINDINGS; i++) {
    if (!g_bindings[i].active) continue;
    PlcHmiBinding* b = &g_bindings[i];
    PlcHmiWidget* wd = plc_hmi_widget_get(b->widget_id);
    if (!wd) continue;

    int32_t val = find_var_value(var_table, var_table_size, b->var_name);
    char val_str[32];
    snprintf(val_str, sizeof(val_str), "%d", val);

    switch (b->direction) {
      case PLC_HMI_BIND_READ:
      case PLC_HMI_BIND_BIDIR:
        plc_hmi_widget_set_prop(b->widget_id, b->property, val_str);
        break;
      case PLC_HMI_BIND_WRITE: {
        char cur_str[32];
        if (plc_hmi_widget_get_prop(b->widget_id, b->property,
                                     cur_str, sizeof(cur_str)) == 0) {
          int32_t cur_val = atoi(cur_str);
          if (cur_val != val) {
            /* 值由用户交互改变，回写到PLC */
            set_var_value(var_table, var_table_size, b->var_name, cur_val);
          }
        }
        break;
      }
    }
  }
}

void plc_hmi_binding_remove_by_widget(uint16_t widget_id)
{
  for (uint16_t i = 0; i < PLC_HMI_MAX_BINDINGS; i++) {
    if (g_bindings[i].active && g_bindings[i].widget_id == widget_id) {
      g_bindings[i].active = false;
    }
  }
}

uint16_t plc_hmi_binding_get_count(void)
{
  uint16_t count = 0;
  for (uint16_t i = 0; i < PLC_HMI_MAX_BINDINGS; i++) {
    if (g_bindings[i].active) count++;
  }
  return count;
}
