/**
 * plc_hmi_widget.h - 控件系统
 *
 * 定义HMI控件的类型、状态、创建/销毁/渲染接口
 * 支持：标签、按钮、开关、滑块、仪表、数值显示、柱状条、进度条、趋势图、矩形、圆、组
 */

#ifndef PLC_HMI_WIDGET_H
#define PLC_HMI_WIDGET_H

#include <stdint.h>
#include <stdbool.h>
#include "plc_hmi.h"

/* ========== 常量定义 ========== */

#define PLC_HMI_MAX_WIDGETS   256
#define PLC_HMI_MAX_BINDINGS  128
#define PLC_HMI_MAX_SCREENS   16
#define PLC_HMI_MAX_STR_LEN   64
#define PLC_HMI_WIDGET_ID_INVALID  0xFFFF

/* ========== 控件类型枚举 ========== */

typedef enum {
  PLC_HMI_WIDGET_LABEL = 0,     /* 文本标签 */
  PLC_HMI_WIDGET_BUTTON,        /* 按钮 */
  PLC_HMI_WIDGET_SWITCH,        /* 开关 */
  PLC_HMI_WIDGET_SLIDER,        /* 滑块 */
  PLC_HMI_WIDGET_GAUGE,         /* 仪表盘 */
  PLC_HMI_WIDGET_VALUE_DISPLAY, /* 数值显示 */
  PLC_HMI_WIDGET_BAR,           /* 柱状条 */
  PLC_HMI_WIDGET_PROGRESS_BAR,  /* 进度条 */
  PLC_HMI_WIDGET_TREND_CHART,   /* 趋势图 */
  PLC_HMI_WIDGET_RECTANGLE,     /* 矩形 */
  PLC_HMI_WIDGET_CIRCLE,        /* 圆形 */
  PLC_HMI_WIDGET_GROUP,         /* 容器组 */
  PLC_HMI_WIDGET_COUNT
} PlcHmiWidgetType;

/* ========== 控件状态枚举 ========== */

typedef enum {
  PLC_HMI_STATE_NORMAL = 0,
  PLC_HMI_STATE_PRESSED,
  PLC_HMI_STATE_DISABLED,
  PLC_HMI_STATE_COUNT
} PlcHmiWidgetState;

/* ========== 属性名称 ========== */

#define PLC_HMI_PROP_VISIBLE   "visible"
#define PLC_HMI_PROP_ENABLED   "enabled"
#define PLC_HMI_PROP_X         "x"
#define PLC_HMI_PROP_Y         "y"
#define PLC_HMI_PROP_W         "w"
#define PLC_HMI_PROP_H         "h"
#define PLC_HMI_PROP_TEXT      "text"
#define PLC_HMI_PROP_VALUE     "value"
#define PLC_HMI_PROP_MIN       "min"
#define PLC_HMI_PROP_MAX       "max"
#define PLC_HMI_PROP_COLOR     "color"
#define PLC_HMI_PROP_BG_COLOR  "bg_color"
#define PLC_HMI_PROP_FONT_SIZE "font_size"
#define PLC_HMI_PROP_FORMAT    "format"
#define PLC_HMI_PROP_UNIT      "unit"
#define PLC_HMI_PROP_DECIMALS  "decimals"
#define PLC_HMI_PROP_BORDER_COLOR "border_color"
#define PLC_HMI_PROP_BORDER_WIDTH "border_width"

/* ========== 回调类型 ========== */

struct PlcHmiWidget;

/** 点击回调 */
typedef void (*PlcHmiClickCallback)(struct PlcHmiWidget* widget);
/** 值变化回调 */
typedef void (*PlcHmiChangeCallback)(struct PlcHmiWidget* widget, int32_t new_val);

/* ========== 控件数据结构 ========== */

/** 标签数据 */
typedef struct {
  char     text[PLC_HMI_MAX_STR_LEN];
  uint8_t  font_size;   /* 缩放倍数: 1=8x16, 2=16x32... */
  uint32_t text_color;
} PlcHmiLabelData;

/** 按钮数据 */
typedef struct {
  char     text[PLC_HMI_MAX_STR_LEN];
  uint32_t normal_color;
  uint32_t pressed_color;
  uint32_t text_color;
  uint8_t  font_size;
} PlcHmiButtonData;

/** 开关数据 */
typedef struct {
  bool     state;       /* true=开, false=关 */
  uint32_t on_color;
  uint32_t off_color;
} PlcHmiSwitchData;

/** 滑块数据 */
typedef struct {
  int32_t  min_val;
  int32_t  max_val;
  int32_t  cur_val;
  uint32_t track_color;
  uint32_t fill_color;
} PlcHmiSliderData;

/** 仪表数据 */
typedef struct {
  int32_t  min_val;
  int32_t  max_val;
  int32_t  cur_val;
  int32_t  redzone_start;  /* 红区起始值 */
  uint32_t needle_color;
  uint32_t scale_color;
} PlcHmiGaugeData;

/** 数值显示数据 */
typedef struct {
  int32_t  value;
  char     format[16];    /* printf格式如 "%d", "%.2f" */
  uint8_t  decimals;
  char     unit[8];
  uint32_t value_color;
  uint32_t unit_color;
  uint8_t  font_size;
} PlcHmiValueDisplayData;

/** 柱状条数据 */
typedef struct {
  int32_t  min_val;
  int32_t  max_val;
  int32_t  cur_val;
  uint32_t bar_color;
  uint32_t bg_color;
  bool     vertical;     /* true=垂直, false=水平 */
} PlcHmiBarData;

/** 进度条数据 */
typedef struct {
  int32_t  value;        /* 0-100百分比 */
  uint32_t fill_color;
  uint32_t bg_color;
  char     text[PLC_HMI_MAX_STR_LEN];
} PlcHmiProgressBarData;

/** 趋势图数据 */
typedef struct {
  int32_t  values[64];   /* 64点历史数据 */
  uint8_t  head;         /* 环形缓冲区头 */
  uint8_t  count;        /* 有效数据点数 */
  int32_t  min_val;
  int32_t  max_val;
  uint32_t line_color;
  uint32_t grid_color;
} PlcHmiTrendChartData;

/** 矩形数据 */
typedef struct {
  uint32_t fill_color;
  uint32_t border_color;
  uint8_t  border_width;
  bool     filled;
} PlcHmiRectangleData;

/** 圆形数据 */
typedef struct {
  uint32_t fill_color;
  uint32_t border_color;
  uint8_t  border_width;
  bool     filled;
} PlcHmiCircleData;

/** 组数据 */
typedef struct {
  uint16_t children[32];  /* 子控件ID列表 */
  uint8_t  child_count;
} PlcHmiGroupData;

/* ========== 控件结构 ========== */

/**
 * 控件描述结构
 * 所有控件类型共用此结构，通过union存放各类型特有数据
 */
typedef struct PlcHmiWidget {
  uint16_t           id;         /* 唯一标识 */
  PlcHmiWidgetType   type;       /* 控件类型 */
  int16_t            x;          /* X坐标 */
  int16_t            y;          /* Y坐标 */
  uint16_t           w;          /* 宽度 */
  uint16_t           h;          /* 高度 */
  bool               visible;    /* 是否可见 */
  bool               enabled;    /* 是否启用 */
  PlcHmiWidgetState  state;      /* 控件状态 */
  uint16_t           parent_id;  /* 父控件ID (0xFFFF=无父) */

  PlcHmiClickCallback  on_click;
  PlcHmiChangeCallback on_change;

  union {
    PlcHmiLabelData         label;
    PlcHmiButtonData        button;
    PlcHmiSwitchData        switch_data;
    PlcHmiSliderData        slider;
    PlcHmiGaugeData         gauge;
    PlcHmiValueDisplayData  value_display;
    PlcHmiBarData           bar;
    PlcHmiProgressBarData   progress_bar;
    PlcHmiTrendChartData    trend_chart;
    PlcHmiRectangleData     rectangle;
    PlcHmiCircleData        circle;
    PlcHmiGroupData         group;
  } data;
} PlcHmiWidget;

/* ========== 绑定结构 ========== */

/** 绑定方向 */
typedef enum {
  PLC_HMI_BIND_READ = 0,     /* PLC→HMI (只读) */
  PLC_HMI_BIND_WRITE,        /* HMI→PLC (只写) */
  PLC_HMI_BIND_BIDIR         /* 双向 */
} PlcHmiBindDirection;

/**
 * 控件属性绑定
 * 将PLC变量绑定到控件的某个属性
 */
typedef struct {
  uint16_t              widget_id;       /* 控件ID */
  char                  property[16];    /* 属性名 */
  char                  var_name[32];    /* PLC变量名 */
  PlcHmiBindDirection   direction;       /* 绑定方向 */
  bool                  active;          /* 是否有效 */
} PlcHmiBinding;

/* ========== 控件接口函数 ========== */

/**
 * 初始化控件系统
 */
void plc_hmi_widget_init(void);

/**
 * 创建控件
 * @param type 控件类型
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @return 控件ID, 失败返回 PLC_HMI_WIDGET_ID_INVALID
 */
uint16_t plc_hmi_widget_create(PlcHmiWidgetType type,
                                int16_t x, int16_t y,
                                uint16_t w, uint16_t h);

/**
 * 销毁控件
 * @param id 控件ID
 */
void plc_hmi_widget_destroy(uint16_t id);

/**
 * 设置控件属性
 * @param id 控件ID
 * @param prop 属性名
 * @param value 属性值（字符串）
 * @return 0成功, 负数失败
 */
int plc_hmi_widget_set_prop(uint16_t id, const char* prop, const char* value);

/**
 * 获取控件属性
 * @param id 控件ID
 * @param prop 属性名
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 0成功, 负数失败
 */
int plc_hmi_widget_get_prop(uint16_t id, const char* prop,
                            char* buf, uint32_t buf_size);

/**
 * 设置控件可见性
 */
void plc_hmi_widget_set_visible(uint16_t id, bool visible);

/**
 * 设置控件启用状态
 */
void plc_hmi_widget_set_enabled(uint16_t id, bool enabled);

/**
 * 获取控件指针
 * @param id 控件ID
 * @return 控件指针, 无效ID返回NULL
 */
PlcHmiWidget* plc_hmi_widget_get(uint16_t id);

/**
 * 处理输入事件（触摸/鼠标）
 * @param x 触点X
 * @param y 触点Y
 * @param pressed 是否按下
 * @return 被触发的控件ID, 无触发返回 INVALID
 */
uint16_t plc_hmi_widget_handle_input(int16_t x, int16_t y, bool pressed);

/**
 * 渲染所有控件到帧缓冲区
 * @param screen 屏幕信息
 */
void plc_hmi_widget_render(const PlcHmiScreen* screen);

/* ========== 绑定接口函数 ========== */

/**
 * 添加绑定
 * @param widget_id 控件ID
 * @param prop 属性名
 * @param var_name PLC变量名
 * @param direction 绑定方向
 * @return 0成功, 负数失败
 */
int plc_hmi_binding_add(uint16_t widget_id, const char* prop,
                        const char* var_name, PlcHmiBindDirection direction);

/**
 * 更新所有绑定
 * 从PLC变量表读取值并更新控件属性
 * @param var_table PLC变量表基址
 * @param var_table_size 变量表大小（字节）
 */
void plc_hmi_binding_update_all(void* var_table, uint32_t var_table_size);

/**
 * 移除指定控件的所有绑定
 */
void plc_hmi_binding_remove_by_widget(uint16_t widget_id);

/**
 * 获取当前控件总数
 */
uint16_t plc_hmi_widget_get_count(void);

/**
 * 获取当前绑定总数
 */
uint16_t plc_hmi_binding_get_count(void);

#endif /* PLC_HMI_WIDGET_H */
