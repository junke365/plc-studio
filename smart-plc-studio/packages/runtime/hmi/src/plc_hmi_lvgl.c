#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_driver.h"

#ifdef PLC_USE_LVGL

#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t* g_lv_widgets[PLC_HMI_MAX_WIDGETS];

/* ========== LVGL 显示驱动回调（嵌入式模式用） ========== */

struct LvglDisplay {
  lv_display_t* disp;
  lv_color_t* buf1;
  lv_color_t* buf2;
  uint16_t w;
  uint16_t h;
};

static struct LvglDisplay g_lvgl_disp = {NULL, NULL, NULL, 0, 0};

static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
  const PlcHmiScreen* scr = plc_hmi_get_screen();
  if (!scr || !scr->framebuffer) {
    lv_display_flush_ready(disp);
    return;
  }

  int32_t x, y;
  for (y = area->y1; y <= area->y2; y++) {
    for (x = area->x1; x <= area->x2; x++) {
      uint32_t offset = (uint32_t)y * scr->stride + (uint32_t)x * 4;
      ((uint32_t*)scr->framebuffer)[offset / 4] = lv_color_to_u32(*(lv_color_t*)px_map);
      px_map += sizeof(lv_color_t);
    }
  }

  lv_display_flush_ready(disp);
}

static uint16_t lvgl_get_width(void)
{
  return g_lvgl_disp.disp ? (uint16_t)lv_display_get_horizontal_resolution(g_lvgl_disp.disp) : 0;
}

static uint16_t lvgl_get_height(void)
{
  return g_lvgl_disp.disp ? (uint16_t)lv_display_get_vertical_resolution(g_lvgl_disp.disp) : 0;
}

static int lvgl_embedded_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;

  g_lvgl_disp.w = w;
  g_lvgl_disp.h = h;

  /* 分配双缓冲 */
  size_t buf_size = (size_t)w * h * sizeof(lv_color_t);
  g_lvgl_disp.buf1 = (lv_color_t*)malloc(buf_size);
  g_lvgl_disp.buf2 = (lv_color_t*)malloc(buf_size);
  if (!g_lvgl_disp.buf1 || !g_lvgl_disp.buf2) {
    free(g_lvgl_disp.buf1);
    free(g_lvgl_disp.buf2);
    return -1;
  }

  g_lvgl_disp.disp = lv_display_create((int32_t)w, (int32_t)h);
  if (!g_lvgl_disp.disp) return -1;

  lv_display_set_flush_cb(g_lvgl_disp.disp, lvgl_flush_cb);
  lv_display_set_buffers(g_lvgl_disp.disp,
    g_lvgl_disp.buf1, g_lvgl_disp.buf2,
    buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

  return 0;
}

static void lvgl_embedded_deinit(void)
{
  if (g_lvgl_disp.disp) {
    lv_display_delete(g_lvgl_disp.disp);
    g_lvgl_disp.disp = NULL;
  }
  free(g_lvgl_disp.buf1);
  free(g_lvgl_disp.buf2);
  g_lvgl_disp.buf1 = NULL;
  g_lvgl_disp.buf2 = NULL;
}

static void lvgl_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)fb; (void)w; (void)h; (void)bpp;
}

static const PlcHmiDriver g_lvgl_embedded_driver = {
  "lvgl_embed",
  lvgl_embedded_init,
  lvgl_embedded_deinit,
  lvgl_flush,
  lvgl_get_width,
  lvgl_get_height
};

/* ========== 仿真模式驱动（LVGL + SDL，由 sim_main 管理显示） ========== */

static int lvgl_sim_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)w; (void)h; (void)bpp;
  /* 显示由 SDL 窗口管理，此处只记录信息 */
  const PlcHmiScreen* scr = plc_hmi_get_screen();
  if (scr) {
    g_lvgl_disp.w = scr->width;
    g_lvgl_disp.h = scr->height;
  }
  return 0;
}

static void lvgl_sim_deinit(void) {}

static uint16_t lvgl_sim_get_width(void)  { return g_lvgl_disp.w; }
static uint16_t lvgl_sim_get_height(void) { return g_lvgl_disp.h; }

static const PlcHmiDriver g_lvgl_sim_driver = {
  "lvgl_sim",
  lvgl_sim_init,
  lvgl_sim_deinit,
  lvgl_flush,
  lvgl_sim_get_width,
  lvgl_sim_get_height
};

/* ========== 控件创建 ========== */

static lv_obj_t* create_label(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, wd->data.label.text);
  lv_obj_set_style_text_color(label, lv_color_hex(wd->data.label.text_color & 0xFFFFFF), 0);
  return label;
}

static lv_obj_t* create_button(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_set_size(btn, wd->w, wd->h);
  lv_obj_set_style_bg_color(btn, lv_color_hex(wd->data.button.normal_color & 0xFFFFFF), 0);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, wd->data.button.text);
  lv_obj_set_style_text_color(label, lv_color_hex(wd->data.button.text_color & 0xFFFFFF), 0);
  lv_obj_center(label);

  return btn;
}

static lv_obj_t* create_switch(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* sw = lv_switch_create(parent);
  lv_obj_set_size(sw, wd->w, wd->h);
  if (wd->data.switch_data.state) {
    lv_obj_add_state(sw, LV_STATE_CHECKED);
  }
  return sw;
}

static lv_obj_t* create_slider(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* slider = lv_slider_create(parent);
  lv_obj_set_size(slider, wd->w, wd->h);
  lv_slider_set_range(slider, wd->data.slider.min_val, wd->data.slider.max_val);
  lv_slider_set_value(slider, wd->data.slider.cur_val, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lv_color_hex(wd->data.slider.track_color & 0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(wd->data.slider.fill_color & 0xFFFFFF), LV_PART_INDICATOR);
  return slider;
}

static lv_obj_t* create_gauge(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* scale = lv_scale_create(parent);
  lv_obj_set_size(scale, wd->w, wd->h);

  lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_range(scale, wd->data.gauge.min_val, wd->data.gauge.max_val);
  lv_scale_set_total_tick_count(scale, 11);
  lv_scale_set_major_tick_every(scale, 2);

  lv_scale_set_label_show(scale, true);

  /* 添加指针 */
  lv_obj_t* needle = lv_line_create(scale);
  lv_scale_set_line_needle_value(scale, needle, -20, wd->data.gauge.cur_val);
  lv_obj_set_style_line_color(needle, lv_color_hex(wd->data.gauge.needle_color & 0xFFFFFF), 0);
  lv_obj_set_style_line_width(needle, 4, 0);

  lv_obj_set_style_bg_color(scale, lv_color_hex(wd->data.gauge.scale_color & 0xFFFFFF), 0);

  return scale;
}

static lv_obj_t* create_bar(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* bar = lv_bar_create(parent);
  lv_obj_set_size(bar, wd->w, wd->h);
  lv_bar_set_range(bar, wd->data.bar.min_val, wd->data.bar.max_val);
  lv_bar_set_value(bar, wd->data.bar.cur_val, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar, lv_color_hex(wd->data.bar.bg_color & 0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_color_hex(wd->data.bar.bar_color & 0xFFFFFF), LV_PART_INDICATOR);
  return bar;
}

static lv_obj_t* create_value_display(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* label = lv_label_create(parent);
  char buf[64];
  snprintf(buf, sizeof(buf), wd->data.value_display.format,
    wd->data.value_display.value);
  lv_label_set_text(label, buf);
  lv_obj_set_style_text_color(label, lv_color_hex(wd->data.value_display.value_color & 0xFFFFFF), 0);
  return label;
}

static lv_obj_t* create_trend_chart(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* chart = lv_chart_create(parent);
  lv_obj_set_size(chart, wd->w, wd->h);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, 64);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,
    wd->data.trend_chart.min_val, wd->data.trend_chart.max_val);

  lv_obj_set_style_bg_color(chart, lv_color_hex(0x000000), 0);
  return chart;
}

static lv_obj_t* create_rectangle(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* obj = lv_obj_create(parent);
  lv_obj_set_size(obj, wd->w, wd->h);
  lv_obj_set_style_bg_color(obj, lv_color_hex(wd->data.rectangle.fill_color & 0xFFFFFF), 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(wd->data.rectangle.border_color & 0xFFFFFF), 0);
  lv_obj_set_style_border_width(obj, wd->data.rectangle.border_width, 0);
  return obj;
}

static lv_obj_t* create_circle(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* obj = lv_obj_create(parent);
  uint16_t dia = wd->w < wd->h ? wd->w : wd->h;
  lv_obj_set_size(obj, dia, dia);
  lv_obj_set_style_bg_color(obj, lv_color_hex(wd->data.circle.fill_color & 0xFFFFFF), 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(wd->data.circle.border_color & 0xFFFFFF), 0);
  lv_obj_set_style_border_width(obj, wd->data.circle.border_width, 0);
  lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(obj, true, 0);
  return obj;
}

/* ========== 控件工厂 ========== */

typedef lv_obj_t* (*WidgetFactory)(lv_obj_t*, PlcHmiWidget*);

static WidgetFactory get_factory(uint8_t type)
{
  switch (type) {
    case PLC_HMI_WIDGET_LABEL:         return create_label;
    case PLC_HMI_WIDGET_BUTTON:        return create_button;
    case PLC_HMI_WIDGET_SWITCH:        return create_switch;
    case PLC_HMI_WIDGET_SLIDER:        return create_slider;
    case PLC_HMI_WIDGET_GAUGE:         return create_gauge;
    case PLC_HMI_WIDGET_BAR:           return create_bar;
    case PLC_HMI_WIDGET_VALUE_DISPLAY: return create_value_display;
    case PLC_HMI_WIDGET_TREND_CHART:   return create_trend_chart;
    case PLC_HMI_WIDGET_RECTANGLE:     return create_rectangle;
    case PLC_HMI_WIDGET_CIRCLE:        return create_circle;
    default:                           return NULL;
  }
}

/* ========== 公共接口 ========== */

void plc_hmi_lvgl_create_widgets(lv_obj_t* parent)
{
  memset(g_lv_widgets, 0, sizeof(g_lv_widgets));

  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    PlcHmiWidget* wd = plc_hmi_widget_get(i);
    if (!wd) continue;

    WidgetFactory factory = get_factory(wd->type);
    if (factory) {
      lv_obj_t* obj = factory(parent, wd);
      if (obj) {
        lv_obj_set_pos(obj, wd->x, wd->y);
        g_lv_widgets[i] = obj;
      }
    }
  }
}

void plc_hmi_lvgl_sync_to_widgets(void)
{
  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    if (!g_lv_widgets[i]) continue;
    PlcHmiWidget* wd = plc_hmi_widget_get(i);
    if (!wd) continue;

    switch (wd->type) {
      case PLC_HMI_WIDGET_SWITCH:
        wd->data.switch_data.state = lv_obj_has_state(g_lv_widgets[i], LV_STATE_CHECKED);
        break;
      case PLC_HMI_WIDGET_SLIDER:
        wd->data.slider.cur_val = (int32_t)lv_slider_get_value(g_lv_widgets[i]);
        break;
      case PLC_HMI_WIDGET_BAR:
        wd->data.bar.cur_val = (int32_t)lv_bar_get_value(g_lv_widgets[i]);
        break;
      default:
        break;
    }
  }
}

void plc_hmi_lvgl_sync_from_widgets(void)
{
  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    if (!g_lv_widgets[i]) continue;
    PlcHmiWidget* wd = plc_hmi_widget_get(i);
    if (!wd) continue;

    switch (wd->type) {
      case PLC_HMI_WIDGET_LABEL:
        if (wd->data.label.text) {
          lv_label_set_text(g_lv_widgets[i], wd->data.label.text);
        }
        break;
      case PLC_HMI_WIDGET_BUTTON:
        if (wd->data.button.text) {
          lv_obj_t* label = lv_obj_get_child(g_lv_widgets[i], 0);
          if (label) lv_label_set_text(label, wd->data.button.text);
        }
        break;
      case PLC_HMI_WIDGET_SWITCH:
        if (wd->data.switch_data.state) {
          lv_obj_add_state(g_lv_widgets[i], LV_STATE_CHECKED);
        } else {
          lv_obj_remove_state(g_lv_widgets[i], LV_STATE_CHECKED);
        }
        break;
      case PLC_HMI_WIDGET_SLIDER:
        lv_slider_set_value(g_lv_widgets[i], wd->data.slider.cur_val, LV_ANIM_OFF);
        break;
      case PLC_HMI_WIDGET_BAR:
        lv_bar_set_value(g_lv_widgets[i], wd->data.bar.cur_val, LV_ANIM_OFF);
        break;
      case PLC_HMI_WIDGET_VALUE_DISPLAY: {
        char buf[64];
        snprintf(buf, sizeof(buf), wd->data.value_display.format,
          wd->data.value_display.value);
        lv_label_set_text(g_lv_widgets[i], buf);
        break;
      }
      default:
        break;
    }
  }
}

/**
 * 初始化 LVGL 嵌入式驱动
 * 使用自定义 framebuffer（用于 STM32/ESP32 等嵌入式平台）
 */
int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                       void* buf1, void* buf2)
{
  (void)buf1; (void)buf2;

  lv_init();

  plc_hmi_driver_register(PLC_HMI_DRV_LVGL, &g_lvgl_embedded_driver);
  memset(g_lv_widgets, 0, sizeof(g_lv_widgets));

  return plc_hmi_driver_init(PLC_HMI_DRV_LVGL, display_w, display_h, 32);
}

/**
 * 初始化 LVGL 仿真驱动
 * 显示由 SDL 管理，此驱动只做空操作（用于 PC 仿真）
 */
int plc_hmi_lvgl_sim_init(uint16_t display_w, uint16_t display_h)
{
  plc_hmi_driver_register(PLC_HMI_DRV_LVGL, &g_lvgl_sim_driver);
  memset(g_lv_widgets, 0, sizeof(g_lv_widgets));

  return plc_hmi_driver_init(PLC_HMI_DRV_LVGL, display_w, display_h, 32);
}

void plc_hmi_lvgl_tick(void)
{
  lv_timer_handler();
}

#endif /* PLC_USE_LVGL */
