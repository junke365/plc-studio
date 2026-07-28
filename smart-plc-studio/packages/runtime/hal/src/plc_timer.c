/**
 * plc_timer.c - 定时器硬件抽象层实现
 *
 * 使用静态定时器数组管理多个实例
 * 跟踪计数值、溢出次数和回调触发
 * PWM 模式存储占空比
 */

#include "plc_timer.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** 定时器实例运行状态 */
typedef struct {
  PlcTimerConfig   config;
  bool             initialized;
  PlcTimerState    state;
  uint32_t         count;
  uint32_t         overflow_count;
  uint32_t         start_tick;       /* 启动时的毫秒时间戳 */
  float            duty_percent;     /* PWM 占空比 */
  uint32_t         capture_period;   /* 输入捕获周期 (微秒) */
} TimerInstance;

/* ========== 静态变量 ========== */

static TimerInstance s_timer[PLC_TIMER_MAX_INSTANCES];

/* ========== 模拟底层驱动 ========== */

static int timer_hw_init(uint8_t timer_id, const PlcTimerConfig* cfg) {
  (void)timer_id;
  (void)cfg;
  return 0;
}

static int timer_hw_start(uint8_t timer_id) {
  (void)timer_id;
  return 0;
}

static int timer_hw_stop(uint8_t timer_id) {
  (void)timer_id;
  return 0;
}

static int timer_hw_set_period(uint8_t timer_id, uint32_t period_us) {
  (void)timer_id;
  (void)period_us;
  return 0;
}

static uint32_t timer_hw_get_count(uint8_t timer_id) {
  (void)timer_id;
  return 0;
}

static int timer_hw_pwm_set_duty(uint8_t timer_id, float duty) {
  (void)timer_id;
  (void)duty;
  return 0;
}

static int timer_hw_input_capture_get_freq(uint8_t timer_id, uint32_t* freq_hz) {
  (void)timer_id;
  *freq_hz = 0;
  return 0;
}

/* ========== 公共接口实现 ========== */

int plc_timer_init(uint8_t timer_id, const PlcTimerConfig* config) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES || config == NULL) {
    return -1;
  }
  memset(&s_timer[timer_id], 0, sizeof(TimerInstance));
  s_timer[timer_id].config = *config;
  s_timer[timer_id].initialized = true;
  s_timer[timer_id].state = PLC_TIMER_STOPPED;
  s_timer[timer_id].duty_percent = 0.0f;
  return timer_hw_init(timer_id, config);
}

int plc_timer_start(uint8_t timer_id) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES) {
    return -1;
  }
  if (!s_timer[timer_id].initialized) {
    return -2;
  }
  int ret = timer_hw_start(timer_id);
  if (ret == 0) {
    s_timer[timer_id].state = PLC_TIMER_RUNNING;
    s_timer[timer_id].start_tick = plc_platform_tick_ms();
    s_timer[timer_id].count = 0;
    s_timer[timer_id].overflow_count = 0;
  }
  return ret;
}

int plc_timer_stop(uint8_t timer_id) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES) {
    return -1;
  }
  int ret = timer_hw_stop(timer_id);
  if (ret == 0) {
    s_timer[timer_id].state = PLC_TIMER_STOPPED;
  }
  return ret;
}

int plc_timer_set_period(uint8_t timer_id, uint32_t period_us) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES) {
    return -1;
  }
  if (!s_timer[timer_id].initialized) {
    return -2;
  }
  s_timer[timer_id].config.period = period_us;
  return timer_hw_set_period(timer_id, period_us);
}

int plc_timer_get_count(uint8_t timer_id, uint32_t* count) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES || count == NULL) {
    return -1;
  }
  /* 从硬件读取当前计数值 */
  s_timer[timer_id].count = timer_hw_get_count(timer_id);
  *count = s_timer[timer_id].count;
  return 0;
}

int plc_timer_get_elapsed_us(uint8_t timer_id, uint64_t* elapsed_us) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES || elapsed_us == NULL) {
    return -1;
  }
  if (s_timer[timer_id].state != PLC_TIMER_RUNNING) {
    *elapsed_us = 0;
    return 0;
  }

  uint32_t now_tick = plc_platform_tick_ms();
  uint32_t elapsed_ms = now_tick - s_timer[timer_id].start_tick;
  /* 加上溢出计数对应的时间 */
  uint32_t period_us = s_timer[timer_id].config.period;
  uint64_t overflow_us = (uint64_t)s_timer[timer_id].overflow_count * period_us;
  *elapsed_us = (uint64_t)elapsed_ms * 1000 + overflow_us;
  return 0;
}

int plc_timer_pwm_set_duty(uint8_t timer_id, float duty_percent) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES) {
    return -1;
  }
  if (!s_timer[timer_id].initialized) {
    return -2;
  }
  if (s_timer[timer_id].config.mode != PLC_TIMER_PWM) {
    return -3;
  }
  /* 限幅: 0.0 - 100.0 */
  if (duty_percent < 0.0f) duty_percent = 0.0f;
  if (duty_percent > 100.0f) duty_percent = 100.0f;

  s_timer[timer_id].duty_percent = duty_percent;
  return timer_hw_pwm_set_duty(timer_id, duty_percent);
}

int plc_timer_input_capture_get_freq(uint8_t timer_id, uint32_t* freq_hz) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES || freq_hz == NULL) {
    return -1;
  }
  if (s_timer[timer_id].config.mode != PLC_TIMER_INPUT_CAPTURE) {
    return -2;
  }
  return timer_hw_input_capture_get_freq(timer_id, freq_hz);
}

int plc_timer_get_info(uint8_t timer_id, PlcTimerInfo* info) {
  if (timer_id >= PLC_TIMER_MAX_INSTANCES || info == NULL) {
    return -1;
  }
  TimerInstance* ti = &s_timer[timer_id];
  info->initialized = ti->initialized;
  info->state = ti->state;
  info->count = ti->count;
  info->overflow_count = ti->overflow_count;
  info->duty_percent = ti->duty_percent;
  return 0;
}
