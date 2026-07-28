/**
 * hmi_lvgl.c - LVGL集成驱动
 *
 * 将PlcHmiWidget映射到LVGL对象：
 * - LABEL → lv_label_create
 * - BUTTON → lv_btn_create + child lv_label
 * - SWITCH → lv_switch_create
 * - SLIDER → lv_slider_create
 * - GAUGE → lv_gauge_create
 * - BAR/PROGRESS_BAR → lv_bar_create
 * - VALUE_DISPLAY → lv_label_create
 * - TREND_CHART → lv_chart_create
 * - RECTANGLE → lv_obj with bg
 */

#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include "plc_hmi_driver.h"

#ifdef PLC_USE_LVGL

#include "lvgl/lvgl.h"

/* LVGL对象映射表 */
static lv_obj_t* g_lv_widgets[PLC_HMI_MAX_WIDGETS];

/* ========== LVGL显示驱动回调 ========== */

static void lvgl_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area,
                           lv_color_t* color_p)
{
  const PlcHmiScreen* scr = plc_hmi_get_screen();
  if (!scr || !scr->framebuffer) return;

  int32_t x, y;
  for (y = area->y1; y <= area->y2; y++) {
    for (x = area->x1; x <= area->x2; x++) {
      /* LVGL颜色到ARGB8888 */
      uint32_t offset = (uint32_t)y * scr->stride + (uint32_t)x * 4;
      ((uint32_t*)scr->framebuffer)[offset / 4] = color_p->full;
      color_p++;
    }
  }

  lv_disp_flush_ready(disp_drv);
}

/* ========== LVGL输入驱动回调 ========== */

static bool g_lvgl_touch_pressed = false;
static int16_t g_lvgl_touch_x = 0;
static int16_t g_lvgl_touch_y = 0;

static bool lvgl_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data)
{
  (void)drv;
  data->point.x = g_lvgl_touch_x;
  data->point.y = g_lvgl_touch_y;
  data->state = g_lvgl_touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  return false;
}

/* ========== 控件创建 ========== */

static lv_obj_t* create_label(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, wd->data.label.text);
  lv_obj_set_style_text_color(label, lv_color_hex(wd->data.label.text_color & 0xFFFFFF), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
  return label;
}

static lv_obj_t* create_button(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* btn = lv_btn_create(parent);
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
  /* LVGL v7 gauge (使用lmeter作为替代) */
  lv_obj_t* meter = lv_meter_create(parent);
  lv_obj_set_size(meter, wd->w, wd->h);

  /* 添加刻度盘 */
  lv_meter_scale_t* scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_range(meter, scale,
                            wd->data.gauge.min_val, wd->data.gauge.max_val,
                            270, 135);

  /* 添加指示器 */
  lv_meter_indicator_t* indic = lv_meter_add_needle_lines(meter, scale,
    lv_color_hex(wd->data.gauge.needle_color & 0xFFFFFF), 4, -10);
  lv_meter_set_indicator_value(meter, indic, wd->data.gauge.cur_val);

  return meter;
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

static lv_obj_t* create_progress_bar(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* bar = lv_bar_create(parent);
  lv_obj_set_size(bar, wd->w, wd->h);
  lv_bar_set_range(bar, 0, 100);
  lv_bar_set_value(bar, wd->data.progress_bar.value, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar, lv_color_hex(wd->data.progress_bar.bg_color & 0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_color_hex(wd->data.progress_bar.fill_color & 0xFFFFFF), LV_PART_INDICATOR);
  return bar;
}

static lv_obj_t* create_value_display(lv_obj_t* parent, PlcHmiWidget* wd)
{
  lv_obj_t* label = lv_label_create(parent);
  char buf[32];
  snprintf(buf, sizeof(buf), wd->data.value_display.format, wd->data.value_display.value);
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
  lv_chart_set_range(chart, LV_CHART_PRIMARY_AXIS,
                      wd->data.trend_chart.min_val, wd->data.trend_chart.max_val);

  lv_obj_set_style_bg_color(chart, lv_color_hex(0x000000), 0);
  lv_obj_set_style_line_color(chart, lv_color_hex(wd->data.trend_chart.line_color & 0xFFFFFF), LV_PART_MAIN);

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

/* ========== 显示驱动接口 ========== */

static int lvgl_drv_init(uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)bpp;
  lv_init();

  /* 分配显示缓冲区 */
  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t buf1[480 * 80]; /* 行缓冲 */
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 480 * 80);

  /* 注册显示驱动 */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = w;
  disp_drv.ver_res = h;
  disp_drv.flush_cb = lvgl_flush_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* 注册输入驱动 */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_read_cb;
  lv_indev_drv_register(&indev_drv);

  return 0;
}

static void lvgl_drv_deinit(void)
{
  lv_deinit();
}

static void lvgl_drv_flush(const void* fb, uint16_t w, uint16_t h, uint8_t bpp)
{
  (void)fb; (void)w; (void)h; (void)bpp;
  /* LVGL通过回调直接写入framebuffer */
}

static uint16_t lvgl_drv_get_width(void)  { return lv_disp_get_hor_res(NULL); }
static uint16_t lvgl_drv_get_height(void) { return lv_disp_get_ver_res(NULL); }

static const PlcHmiDriver g_lvgl_driver = {
  "lvgl",
  lvgl_drv_init,
  lvgl_drv_deinit,
  lvgl_drv_flush,
  lvgl_drv_get_width,
  lvgl_drv_get_height
};

/* ========== 公共接口 ========== */

int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                       void* buf1, void* buf2)
{
  (void)buf1; (void)buf2;
  plc_hmi_driver_register(PLC_HMI_DRV_LVGL, &g_lvgl_driver);
  memset(g_lv_widgets, 0, sizeof(g_lv_widgets));
  return plc_hmi_driver_init(PLC_HMI_DRV_LVGL, display_w, display_h, 32);
}

void plc_hmi_lvgl_update(void)
{
  lv_task_handler();
}

void plc_hmi_lvgl_touch_input(int16_t x, int16_t y, bool pressed)
{
  g_lvgl_touch_x = x;
  g_lvgl_touch_y = y;
  g_lvgl_touch_pressed = pressed;
}

/**
 * 同步LVGL控件状态到HMI控件
 * 从LVGL读取交互结果（开关状态、滑块值等）
 */
void plc_hmi_lvgl_sync_from_widgets(void)
{
  const PlcHmiScreen* scr = plc_hmi_get_screen();
  for (uint16_t i = 0; i < PLC_HMI_MAX_WIDGETS; i++) {
    if (!g_lv_widgets[i]) continue;
    PlcHmiWidget* wd = plc_hmi_widget_get(i);
    if (!wd) continue;

    switch (wd->type) {
      case PLC_HMI_WIDGET_SWITCH:
        wd->data.switch_data.state = lv_obj_has_state(g_lv_widgets[i], LV_STATE_CHECKED);
        break;
      case PLC_HMI_WIDGET_SLIDER:
        wd->data.slider.cur_val = lv_slider_get_value(g_lv_widgets[i]);
        break;
      case PLC_HMI_WIDGET_BAR:
        wd->data.bar.cur_val = lv_bar_get_value(g_lv_widgets[i]);
        break;
      default:
        break;
    }
  }
}

#endif /* PLC_USE_LVGL */
