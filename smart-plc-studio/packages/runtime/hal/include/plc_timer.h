/**
 * plc_timer.h - 定时器硬件抽象层接口
 *
 * 提供单次触发、周期定时、PWM 输出、输入捕获等统一接口
 */

#ifndef PLC_TIMER_H
#define PLC_TIMER_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_TIMER_MAX_INSTANCES
  #ifdef PLATFORM_STM32
    #define PLC_TIMER_MAX_INSTANCES  12
  #elif defined(PLATFORM_ESP32)
    #define PLC_TIMER_MAX_INSTANCES  4
  #else
    #define PLC_TIMER_MAX_INSTANCES  16
  #endif
#endif

/* ========== 枚举定义 ========== */

/** 定时器工作模式 */
typedef enum {
  PLC_TIMER_ONE_SHOT     = 0,  /**< 单次触发 */
  PLC_TIMER_PERIODIC     = 1,  /**< 周期定时 */
  PLC_TIMER_PWM          = 2,  /**< PWM 输出 */
  PLC_TIMER_INPUT_CAPTURE = 3  /**< 输入捕获 */
} PlcTimerMode;

/** 定时器运行状态 */
typedef enum {
  PLC_TIMER_STOPPED = 0,
  PLC_TIMER_RUNNING = 1,
} PlcTimerState;

/* ========== 结构体定义 ========== */

/** 定时器回调函数原型 */
typedef void (*PlcTimerCallback)(uint8_t timer_id);

/** 定时器配置 */
typedef struct {
  uint8_t            channel;    /**< 定时器通道号 (硬件相关) */
  uint32_t           prescaler;  /**< 预分频值 (0 表示不分频) */
  uint32_t           period;     /**< 周期值 (计数器单位) */
  PlcTimerMode       mode;       /**< 工作模式 */
  PlcTimerCallback   callback;   /**< 回调函数 (可选) */
} PlcTimerConfig;

/** 定时器运行信息 */
typedef struct {
  bool         initialized;  /**< 是否已初始化 */
  PlcTimerState state;       /**< 运行状态 */
  uint32_t     count;        /**< 当前计数值 */
  uint32_t     overflow_count; /**< 溢出次数 */
  float        duty_percent; /**< PWM 占空比 (仅 PWM 模式) */
} PlcTimerInfo;

/* ========== 函数声明 ========== */

/**
 * 初始化定时器
 * @param timer_id 定时器编号 (0-based)
 * @param config   配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_timer_init(uint8_t timer_id, const PlcTimerConfig* config);

/**
 * 启动定时器
 * @param timer_id 定时器编号
 * @return 0 成功, 负值为错误码
 */
int plc_timer_start(uint8_t timer_id);

/**
 * 停止定时器
 * @param timer_id 定时器编号
 * @return 0 成功, 负值为错误码
 */
int plc_timer_stop(uint8_t timer_id);

/**
 * 动态设置定时器周期
 * @param timer_id  定时器编号
 * @param period_us 周期 (微秒)
 * @return 0 成功, 负值为错误码
 */
int plc_timer_set_period(uint8_t timer_id, uint32_t period_us);

/**
 * 获取当前计数值
 * @param timer_id 定时器编号
 * @param count    输出参数: 当前计数值
 * @return 0 成功, 负值为错误码
 */
int plc_timer_get_count(uint8_t timer_id, uint32_t* count);

/**
 * 获取自启动以来经过的时间 (微秒)
 * @param timer_id  定时器编号
 * @param elapsed_us 输出参数: 经过时间
 * @return 0 成功, 负值为错误码
 */
int plc_timer_get_elapsed_us(uint8_t timer_id, uint64_t* elapsed_us);

/**
 * 设置 PWM 占空比
 * @param timer_id    定时器编号
 * @param duty_percent 占空比 (0.0 - 100.0)
 * @return 0 成功, 负值为错误码
 */
int plc_timer_pwm_set_duty(uint8_t timer_id, float duty_percent);

/**
 * 获取输入捕获频率
 * @param timer_id 定时器编号
 * @param freq_hz  输出参数: 频率 (Hz)
 * @return 0 成功, 负值为错误码
 */
int plc_timer_input_capture_get_freq(uint8_t timer_id, uint32_t* freq_hz);

/**
 * 获取定时器运行信息
 * @param timer_id 定时器编号
 * @param info     输出参数: 运行信息
 * @return 0 成功, 负值为错误码
 */
int plc_timer_get_info(uint8_t timer_id, PlcTimerInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* PLC_TIMER_H */
