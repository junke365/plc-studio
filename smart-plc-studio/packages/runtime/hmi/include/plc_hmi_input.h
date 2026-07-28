/**
 * plc_hmi_input.h - 输入抽象层
 *
 * 统一触摸屏、鼠标、键盘等输入设备的事件处理
 * 采用环形缓冲区存储输入事件
 */

#ifndef PLC_HMI_INPUT_H
#define PLC_HMI_INPUT_H

#include <stdint.h>
#include <stdbool.h>

/* ========== 常量 ========== */

#define PLC_HMI_INPUT_MAX_EVENTS  16
#define PLC_HMI_INPUT_KEY_NONE    0

/* ========== 输入类型 ========== */

typedef enum {
  PLC_HMI_INPUT_TOUCH = 0,   /* 触摸屏 */
  PLC_HMI_INPUT_MOUSE,       /* 鼠标 */
  PLC_HMI_INPUT_KEYBOARD,    /* 键盘 */
  PLC_HMI_INPUT_TYPE_COUNT
} PlcHmiInputType;

/* ========== 键码定义 ========== */

#define PLC_HMI_KEY_0         0x30
#define PLC_HMI_KEY_9         0x39
#define PLC_HMI_KEY_A         0x41
#define PLC_HMI_KEY_Z         0x5A
#define PLC_HMI_KEY_ENTER     0x0D
#define PLC_HMI_KEY_ESCAPE    0x1B
#define PLC_HMI_KEY_BACKSPACE 0x08
#define PLC_HMI_KEY_TAB       0x09
#define PLC_HMI_KEY_SPACE     0x20
#define PLC_HMI_KEY_UP        0x100
#define PLC_HMI_KEY_DOWN      0x101
#define PLC_HMI_KEY_LEFT      0x102
#define PLC_HMI_KEY_RIGHT     0x103
#define PLC_HMI_KEY_F1        0x110
#define PLC_HMI_KEY_F12       0x11B

/* ========== 输入事件结构 ========== */

/**
 * 输入事件
 */
typedef struct {
  PlcHmiInputType type;     /* 事件类型 */
  int16_t         x;        /* 坐标X (触摸/鼠标) */
  int16_t         y;        /* 坐标Y (触摸/鼠标) */
  uint16_t        key;      /* 键码 (键盘) */
  bool            pressed;  /* 按下/释放 (触摸/键盘) */
} PlcHmiInputEvent;

/* ========== 输入驱动接口 ========== */

/**
 * 输入设备驱动
 */
typedef struct {
  const char* name;
  int  (*init)(void);
  void (*poll_event)(PlcHmiInputEvent* event, bool* has_event);
  void (*deinit)(void);
} PlcHmiInputDriver;

/* ========== 接口函数 ========== */

/**
 * 初始化输入子系统
 * @return 0成功, 负数失败
 */
int plc_hmi_input_init(void);

/**
 * 轮询所有输入驱动，收集事件到缓冲区
 * @return 本次新增的事件数
 */
int plc_hmi_input_poll(void);

/**
 * 获取一个输入事件
 * @param event 输出事件
 * @return true有事件, false缓冲区空
 */
bool plc_hmi_input_get_event(PlcHmiInputEvent* event);

/**
 * 注册输入驱动
 * @param type 输入类型
 * @param driver 驱动实现
 * @return 0成功, 负数失败
 */
int plc_hmi_input_register_driver(PlcHmiInputType type,
                                   const PlcHmiInputDriver* driver);

/**
 * 直接注入一个输入事件（用于测试/模拟）
 * @param event 事件
 */
void plc_hmi_input_inject(const PlcHmiInputEvent* event);

/**
 * 清空事件缓冲区
 */
void plc_hmi_input_flush(void);

#endif /* PLC_HMI_INPUT_H */
