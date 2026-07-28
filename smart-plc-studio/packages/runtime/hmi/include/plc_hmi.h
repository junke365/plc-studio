/**
 * plc_hmi.h - HMI主接口
 *
 * 定义HMI运行时的核心类型、颜色系统和主接口函数
 * 支持RGB565(16位)和ARGB8888(32位)两种颜色模式
 */

#ifndef PLC_HMI_H
#define PLC_HMI_H

#include <stdint.h>
#include <stdbool.h>
#include "plc_platform.h"

/* ========== 颜色类型定义 ========== */

/**
 * 颜色类型 - 根据bpp选择
 * 16bpp: RGB565格式
 * 32bpp: ARGB8888格式
 */
typedef uint16_t PlcHmiColor16;
typedef uint32_t PlcHmiColor32;

/* 颜色模式枚举 */
typedef enum {
  PLC_HMI_BPP_16 = 16,    /* RGB565 */
  PLC_HMI_BPP_32 = 32     /* ARGB8888 */
} PlcHmiBpp;

/* ========== 颜色宏定义 ========== */

/* RGB565颜色宏 */
#define PLC_HMI_RGB565(r, g, b) \
  ((PlcHmiColor16)(((r) & 0xF8) << 8 | ((g) & 0xFC) << 3 | ((b) & 0xF8) >> 3))

/* ARGB8888颜色宏 */
#define PLC_HMI_ARGB(a, r, g, b) \
  ((PlcHmiColor32)(((a) & 0xFF) << 24 | ((r) & 0xFF) << 16 | \
                   ((g) & 0xFF) << 8 | ((b) & 0xFF)))

/* 通用颜色宏（根据bpp选择） */
#define PLC_HMI_COLOR(r, g, b)  PLC_HMI_ARGB(0xFF, r, g, b)

/* 预定义颜色 - ARGB8888 */
#define PLC_HMI_COLOR_BLACK     PLC_HMI_ARGB(0xFF, 0, 0, 0)
#define PLC_HMI_COLOR_WHITE     PLC_HMI_ARGB(0xFF, 255, 255, 255)
#define PLC_HMI_COLOR_RED       PLC_HMI_ARGB(0xFF, 255, 0, 0)
#define PLC_HMI_COLOR_GREEN     PLC_HMI_ARGB(0xFF, 0, 255, 0)
#define PLC_HMI_COLOR_BLUE      PLC_HMI_ARGB(0xFF, 0, 0, 255)
#define PLC_HMI_COLOR_YELLOW    PLC_HMI_ARGB(0xFF, 255, 255, 0)
#define PLC_HMI_COLOR_CYAN      PLC_HMI_ARGB(0xFF, 0, 255, 255)
#define PLC_HMI_COLOR_MAGENTA   PLC_HMI_ARGB(0xFF, 255, 0, 255)
#define PLC_HMI_COLOR_ORANGE    PLC_HMI_ARGB(0xFF, 255, 165, 0)
#define PLC_HMI_COLOR_GRAY      PLC_HMI_ARGB(0xFF, 128, 128, 128)
#define PLC_HMI_COLOR_DARK_GRAY PLC_HMI_ARGB(0xFF, 64, 64, 64)
#define PLC_HMI_COLOR_LIGHT_GRAY PLC_HMI_ARGB(0xFF, 192, 192, 192)

/* ========== 屏幕结构 ========== */

/**
 * 屏幕信息结构
 * 描述一个显示屏幕的帧缓冲区
 */
typedef struct {
  uint16_t width;       /* 屏幕宽度(像素) */
  uint16_t height;      /* 屏幕高度(像素) */
  uint8_t  bpp;         /* 位深度: 16或32 */
  void*    framebuffer; /* 帧缓冲区指针 */
  uint32_t stride;      /* 每行字节数 */
} PlcHmiScreen;

/* ========== 配置结构 ========== */

/**
 * HMI配置结构
 * 用于初始化HMI系统
 */
typedef struct {
  uint16_t screen_width;   /* 屏幕宽度 */
  uint16_t screen_height;  /* 屏幕高度 */
  uint8_t  bpp;            /* 位深度 */
  uint8_t  fps_target;     /* 目标帧率(1-120) */
  void*    var_table;      /* PLC变量表指针 */
  uint32_t var_table_size; /* 变量表大小(字节) */
} PlcHmiConfig;

/* ========== 主接口函数 ========== */

/**
 * 初始化HMI系统
 * @param config 配置参数
 * @return 0成功, 负数失败
 */
int plc_hmi_init(const PlcHmiConfig* config);

/**
 * 启动HMI运行
 * 创建显示任务、开始渲染循环
 * @return 0成功, 负数失败
 */
int plc_hmi_start(void);

/**
 * 停止HMI运行
 */
void plc_hmi_stop(void);

/**
 * HMI更新函数
 * 应在主循环中调用，处理输入、更新绑定、渲染画面
 * 内部会按照目标帧率控制更新频率
 */
void plc_hmi_update(void);

/**
 * 获取当前屏幕信息
 * @return 屏幕信息指针
 */
const PlcHmiScreen* plc_hmi_get_screen(void);

/**
 * 设置目标帧率
 * @param fps 目标帧率(1-120)
 */
void plc_hmi_set_fps(uint8_t fps);

/**
 * 绑定PLC变量到控件属性
 * @param screen_id 屏幕ID
 * @param widget_id 控件ID
 * @param var_name PLC变量名
 * @param direction 绑定方向: 0=只读, 1=只写, 2=双向
 * @return 0成功, 负数失败
 */
int plc_hmi_bind_variable(uint8_t screen_id, uint16_t widget_id,
                         const char* var_name, uint8_t direction);

/**
 * 保存截图
 * @param filename 文件名
 * @return 0成功, 负数失败
 */
int plc_hmi_screenshot(const char* filename);

/**
 * 导航到指定屏幕
 * @param screen_name 屏幕名称
 * @return 0成功, 负数失败
 */
int plc_hmi_navigate(const char* screen_name);

/**
 * 获取当前帧率
 * @return 实际帧率
 */
uint8_t plc_hmi_get_fps(void);

/**
 * 获取HMI运行状态
 * @return true运行中, false已停止
 */
bool plc_hmi_is_running(void);

#endif /* PLC_HMI_H */
