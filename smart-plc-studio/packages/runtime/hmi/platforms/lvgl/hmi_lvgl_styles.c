/**
 * hmi_lvgl_styles.c - LVGL工业暗色主题样式
 *
 * 定义按钮、标签等控件的视觉样式
 * 配色方案：深色背景 + 蓝/绿/灰色按钮 + 白色/青色文字
 */

#include "plc_hmi.h"

#ifdef PLC_USE_LVGL

#include "lvgl/lvgl.h"

/* ========== 主题颜色 ========== */

#define THEME_BG_DARK      0x1A1A2E   /* 深蓝黑背景 */
#define THEME_BG_PANEL     0x16213E   /* 面板背景 */
#define THEME_ACCENT_BLUE  0x3366CC   /* 主蓝色 */
#define THEME_ACCENT_GREEN 0x33CC66   /* 成功绿 */
#define THEME_ACCENT_RED   0xCC3333   /* 警告红 */
#define THEME_TEXT_WHITE   0xFFFFFF   /* 白色文字 */
#define THEME_TEXT_CYAN    0x00FFFF   /* 数值青色 */
#define THEME_TEXT_GRAY    0x999999   /* 灰色副文字 */
#define THEME_BORDER       0x444444   /* 边框灰 */
#define THEME_DISABLED     0x555555   /* 禁用灰 */

/* ========== 样式实例 ========== */

static lv_style_t g_style_btn_normal;
static lv_style_t g_style_btn_pressed;
static lv_style_t g_style_btn_disabled;
static lv_style_t g_style_label_title;
static lv_style_t g_style_label_value;
static lv_style_t g_style_label_unit;
static lv_style_t g_style_panel;
static lv_style_t g_style_slider_bg;
static lv_style_t g_style_slider_ind;
static bool g_styles_initialized = false;

/* ========== 初始化 ========== */

void plc_hmi_lvgl_styles_init(void)
{
  if (g_styles_initialized) return;

  /* 按钮正常样式 */
  lv_style_init(&g_style_btn_normal);
  lv_style_set_bg_color(&g_style_btn_normal, lv_color_hex(THEME_ACCENT_BLUE));
  lv_style_set_bg_opa(&g_style_btn_normal, LV_OPA_COVER);
  lv_style_set_border_color(&g_style_btn_normal, lv_color_hex(THEME_BORDER));
  lv_style_set_border_width(&g_style_btn_normal, 1);
  lv_style_set_radius(&g_style_btn_normal, 4);
  lv_style_set_text_color(&g_style_btn_normal, lv_color_hex(THEME_TEXT_WHITE));
  lv_style_set_text_font(&g_style_btn_normal, &lv_font_montserrat_16);

  /* 按钮按下样式 */
  lv_style_init(&g_style_btn_pressed);
  lv_style_set_bg_color(&g_style_btn_pressed, lv_color_hex(0x224488));
  lv_style_set_bg_opa(&g_style_btn_pressed, LV_OPA_COVER);
  lv_style_set_border_color(&g_style_btn_pressed, lv_color_hex(THEME_ACCENT_GREEN));
  lv_style_set_border_width(&g_style_btn_pressed, 2);
  lv_style_set_radius(&g_style_btn_pressed, 4);
  lv_style_set_text_color(&g_style_btn_pressed, lv_color_hex(THEME_TEXT_WHITE));

  /* 按钮禁用样式 */
  lv_style_init(&g_style_btn_disabled);
  lv_style_set_bg_color(&g_style_btn_disabled, lv_color_hex(THEME_DISABLED));
  lv_style_set_bg_opa(&g_style_btn_disabled, LV_OPA_COVER);
  lv_style_set_radius(&g_style_btn_disabled, 4);
  lv_style_set_text_color(&g_style_btn_disabled, lv_color_hex(THEME_TEXT_GRAY));

  /* 标题标签样式 */
  lv_style_init(&g_style_label_title);
  lv_style_set_text_color(&g_style_label_title, lv_color_hex(THEME_TEXT_WHITE));
  lv_style_set_text_font(&g_style_label_title, &lv_font_montserrat_20);

  /* 数值标签样式 */
  lv_style_init(&g_style_label_value);
  lv_style_set_text_color(&g_style_label_value, lv_color_hex(THEME_TEXT_CYAN));
  lv_style_set_text_font(&g_style_label_value, &lv_font_montserrat_24);

  /* 单位标签样式 */
  lv_style_init(&g_style_label_unit);
  lv_style_set_text_color(&g_style_label_unit, lv_color_hex(THEME_TEXT_GRAY));
  lv_style_set_text_font(&g_style_label_unit, &lv_font_montserrat_14);

  /* 面板样式 */
  lv_style_init(&g_style_panel);
  lv_style_set_bg_color(&g_style_panel, lv_color_hex(THEME_BG_PANEL));
  lv_style_set_bg_opa(&g_style_panel, LV_OPA_COVER);
  lv_style_set_border_color(&g_style_panel, lv_color_hex(THEME_BORDER));
  lv_style_set_border_width(&g_style_panel, 1);
  lv_style_set_radius(&g_style_panel, 6);
  lv_style_set_pad_all(&g_style_panel, 8);

  /* 滑块样式 */
  lv_style_init(&g_style_slider_bg);
  lv_style_set_bg_color(&g_style_slider_bg, lv_color_hex(THEME_DISABLED));
  lv_style_set_radius(&g_style_slider_bg, 3);

  lv_style_init(&g_style_slider_ind);
  lv_style_set_bg_color(&g_style_slider_ind, lv_color_hex(THEME_ACCENT_BLUE));
  lv_style_set_radius(&g_style_slider_ind, 3);

  g_styles_initialized = true;
}

/* ========== 获取样式 ========== */

const lv_style_t* plc_hmi_lvgl_get_style_btn_normal(void)
{
  return &g_style_btn_normal;
}

const lv_style_t* plc_hmi_lvgl_get_style_btn_pressed(void)
{
  return &g_style_btn_pressed;
}

const lv_style_t* plc_hmi_lvgl_get_style_btn_disabled(void)
{
  return &g_style_btn_disabled;
}

const lv_style_t* plc_hmi_lvgl_get_style_label_title(void)
{
  return &g_style_label_title;
}

const lv_style_t* plc_hmi_lvgl_get_style_label_value(void)
{
  return &g_style_label_value;
}

const lv_style_t* plc_hmi_lvgl_get_style_label_unit(void)
{
  return &g_style_label_unit;
}

const lv_style_t* plc_hmi_lvgl_get_style_panel(void)
{
  return &g_style_panel;
}

#endif /* PLC_USE_LVGL */
