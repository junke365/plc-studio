/**
 * plc_hmi_demo.c - HMI演示界面
 *
 * 创建一个完整的工业HMI演示画面：
 * - 标题栏
 * - 电机启停按钮
 * - 转速仪表盘
 * - 温度数值显示
 * - 状态指示LED
 * - 进度条
 * - 趋势图
 */

#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include <string.h>
#include <math.h>

/* 模拟变量索引 */
#define VAR_MOTOR_RUN     0
#define VAR_MOTOR_SPEED   1
#define VAR_TEMPERATURE   2
#define VAR_PROGRESS      3
#define VAR_TREND_VAL     4
#define VAR_STATUS_LED1   5
#define VAR_STATUS_LED2   6

/* 控件ID */
#define WID_TITLE      0
#define WID_BTN_START  1
#define WID_BTN_STOP   2
#define WID_GAUGE      3
#define WID_TEMP_TITLE 4
#define WID_TEMP_VALUE 5
#define WID_LED1       6
#define WID_LED2       7
#define WID_PROGRESS   8
#define WID_TREND      9
#define WID_SLIDER     10
#define WID_SLIDER_VAL 11

/* 计数器 */
static uint32_t g_demo_tick = 0;

/**
 * 初始化演示HMI界面
 */
void plc_hmi_demo_init(void)
{
  plc_hmi_widget_init();

  /* ---- 标题栏 ---- */
  uint16_t id;
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 0, 0, 800, 40);
  if (id != PLC_HMI_WIDGET_ID_INVALID) {
    plc_hmi_widget_set_prop(id, "text", "Smart PLC HMI");
    plc_hmi_widget_set_prop(id, "font_size", "2");
    plc_hmi_widget_set_prop(id, "color", "00FFFF");
    /* 标题栏背景通过矩形模拟 */
  }

  /* 标题栏背景矩形 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 0, 0, 800, 40);
  if (id != PLC_HMI_WIDGET_ID_INVALID) {
    plc_hmi_widget_set_prop(id, "bg_color", "1A1A2E");
    plc_hmi_widget_set_prop(id, "color", "00FFFF");
  }
  plc_hmi_widget_set_visible(id, false); /* 先隐藏文字，后面改 */

  /* ---- 电机控制区 ---- */
  /* 区域背景 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 20, 60, 200, 200);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  /* 区域标题 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 30, 65, 180, 20);
  plc_hmi_widget_set_prop(id, "text", "Motor Control");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  /* 启动按钮 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_BUTTON, 35, 100, 170, 50);
  plc_hmi_widget_set_prop(id, "text", "START");
  plc_hmi_widget_set_prop(id, "bg_color", "228B22");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  /* 停止按钮 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_BUTTON, 35, 160, 170, 50);
  plc_hmi_widget_set_prop(id, "text", "STOP");
  plc_hmi_widget_set_prop(id, "bg_color", "CC3333");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  /* 运行状态开关 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_SWITCH, 50, 225, 80, 30);
  plc_hmi_widget_set_prop(id, "value", "0");

  /* ---- 仪表盘区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 240, 60, 240, 200);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 250, 65, 220, 20);
  plc_hmi_widget_set_prop(id, "text", "Speed (RPM)");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_GAUGE, 260, 90, 200, 150);
  plc_hmi_widget_set_prop(id, "min", "0");
  plc_hmi_widget_set_prop(id, "max", "3000");
  plc_hmi_widget_set_prop(id, "value", "0");

  /* ---- 温度显示区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 500, 60, 140, 120);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 510, 65, 120, 20);
  plc_hmi_widget_set_prop(id, "text", "Temperature");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_VALUE_DISPLAY, 510, 95, 120, 50);
  plc_hmi_widget_set_prop(id, "format", "%.1f");
  plc_hmi_widget_set_prop(id, "unit", "C");
  plc_hmi_widget_set_prop(id, "font_size", "3");
  plc_hmi_widget_set_prop(id, "color", "FF6600");

  /* ---- 状态LED区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 660, 60, 120, 120);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 670, 65, 100, 20);
  plc_hmi_widget_set_prop(id, "text", "Status");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  /* LED1 - 圆形 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_CIRCLE, 680, 95, 30, 30);
  plc_hmi_widget_set_prop(id, "bg_color", "333333");
  plc_hmi_widget_set_prop(id, "color", "FF0000");

  /* LED2 - 圆形 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_CIRCLE, 730, 95, 30, 30);
  plc_hmi_widget_set_prop(id, "bg_color", "333333");
  plc_hmi_widget_set_prop(id, "color", "00FF00");

  /* LED标签 */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 675, 130, 40, 15);
  plc_hmi_widget_set_prop(id, "text", "PWR");
  plc_hmi_widget_set_prop(id, "color", "999999");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 725, 130, 40, 15);
  plc_hmi_widget_set_prop(id, "text", "RUN");
  plc_hmi_widget_set_prop(id, "color", "999999");

  /* ---- 进度条区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 20, 280, 460, 60);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 30, 285, 120, 20);
  plc_hmi_widget_set_prop(id, "text", "Progress");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_PROGRESS_BAR, 30, 310, 440, 20);
  plc_hmi_widget_set_prop(id, "bg_color", "333333");
  plc_hmi_widget_set_prop(id, "value", "0");

  /* ---- 趋势图区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 20, 350, 460, 120);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 30, 355, 120, 20);
  plc_hmi_widget_set_prop(id, "text", "Trend");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_TREND_CHART, 30, 380, 440, 85);
  plc_hmi_widget_set_prop(id, "min", "0");
  plc_hmi_widget_set_prop(id, "max", "100");

  /* ---- 滑块控制区 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 500, 200, 280, 80);
  plc_hmi_widget_set_prop(id, "bg_color", "16213E");
  plc_hmi_widget_set_prop(id, "color", "444444");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 510, 205, 120, 20);
  plc_hmi_widget_set_prop(id, "text", "Setpoint");
  plc_hmi_widget_set_prop(id, "color", "FFFFFF");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_SLIDER, 510, 235, 250, 20);
  plc_hmi_widget_set_prop(id, "min", "0");
  plc_hmi_widget_set_prop(id, "max", "100");
  plc_hmi_widget_set_prop(id, "value", "50");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_VALUE_DISPLAY, 620, 260, 80, 15);
  plc_hmi_widget_set_prop(id, "format", "%d%%");
  plc_hmi_widget_set_prop(id, "font_size", "1");

  /* ---- 底部信息栏 ---- */
  id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 0, 464, 800, 16);
  plc_hmi_widget_set_prop(id, "bg_color", "1A1A2E");
  plc_hmi_widget_set_prop(id, "color", "333333");

  id = plc_hmi_widget_create(PLC_HMI_WIDGET_LABEL, 10, 466, 400, 14);
  plc_hmi_widget_set_prop(id, "text", "Smart PLC Studio v1.0");
  plc_hmi_widget_set_prop(id, "color", "666666");

  plc_platform_log(PLC_LOG_INFO, "演示界面初始化完成: %d个控件",
                   plc_hmi_widget_get_count());
}

/**
 * 更新演示界面 - 模拟PLC变量变化
 * @param var_table PLC变量表
 * @param var_table_size 变量表大小
 */
void plc_hmi_demo_update(void* var_table, uint32_t var_table_size)
{
  if (!var_table) return;
  int32_t* vars = (int32_t*)var_table;
  g_demo_tick++;

  /* 模拟电机转速：正弦波 */
  if (vars[VAR_MOTOR_RUN]) {
    double rad = (double)g_demo_tick * 0.05;
    vars[VAR_MOTOR_SPEED] = (int32_t)(1500.0 + 500.0 * sin(rad));
  } else {
    /* 减速停止 */
    if (vars[VAR_MOTOR_SPEED] > 10) {
      vars[VAR_MOTOR_SPEED] -= vars[VAR_MOTOR_SPEED] / 20 + 1;
    } else {
      vars[VAR_MOTOR_SPEED] = 0;
    }
  }

  /* 模拟温度：慢速正弦 */
  {
    double rad = (double)g_demo_tick * 0.01;
    vars[VAR_TEMPERATURE] = (int32_t)(300.0 + 50.0 * sin(rad));
  }

  /* 进度：循环 0-100 */
  vars[VAR_PROGRESS] = (int32_t)(g_demo_tick % 101);

  /* 趋势值：带噪声的正弦 */
  {
    double rad = (double)g_demo_tick * 0.08;
    int32_t noise = (int32_t)(g_demo_tick * 7 % 20) - 10;
    vars[VAR_TREND_VAL] = (int32_t)(50.0 + 30.0 * sin(rad)) + noise;
  }

  /* LED闪烁 */
  vars[VAR_STATUS_LED1] = (g_demo_tick / 30) % 2;
  vars[VAR_STATUS_LED2] = (g_demo_tick / 50) % 2;

  /* ---- 更新控件 ---- */

  /* 仪表盘 */
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", vars[VAR_MOTOR_SPEED]);
    plc_hmi_widget_set_prop(WID_GAUGE, "value", buf);
  }

  /* 温度显示 (值x10 → 实际温度) */
  {
    char buf[16];
    int32_t temp = vars[VAR_TEMPERATURE];
    int32_t whole = temp / 10;
    int32_t frac = temp % 10;
    if (frac < 0) frac = -frac;
    snprintf(buf, sizeof(buf), "%d.%d", whole, frac);
    plc_hmi_widget_set_prop(WID_TEMP_VALUE, "value", buf);
  }

  /* LED颜色 */
  {
    uint32_t led1_color = vars[VAR_STATUS_LED1] ? 0x00FF00 : 0x333333;
    uint32_t led2_color = vars[VAR_STATUS_LED2] ? 0x00FF00 : 0x333333;
    char c1[8], c2[8];
    snprintf(c1, sizeof(c1), "%06X", led1_color);
    snprintf(c2, sizeof(c2), "%06X", led2_color);
    plc_hmi_widget_set_prop(WID_LED1, "bg_color", c1);
    plc_hmi_widget_set_prop(WID_LED2, "bg_color", c2);
  }

  /* 进度条 */
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", vars[VAR_PROGRESS]);
    plc_hmi_widget_set_prop(WID_PROGRESS, "value", buf);
  }

  /* 趋势图 */
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", vars[VAR_TREND_VAL]);
    plc_hmi_widget_set_prop(WID_TREND, "value", buf);
  }

  /* 滑块值同步到显示 */
  {
    PlcHmiWidget* slider = plc_hmi_widget_get(WID_SLIDER);
    PlcHmiWidget* display = plc_hmi_widget_get(WID_SLIDER_VAL);
    if (slider && display) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d%%", slider->data.slider.cur_val);
      plc_hmi_widget_set_prop(WID_SLIDER_VAL, "value", buf);
    }
  }
}
