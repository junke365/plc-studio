/**
 * plc_platform.h - 平台抽象层
 *
 * 统一各平台的类型定义、时间函数、内存操作等基础接口
 * 编译时通过 PLATFORM_* 宏选择目标平台
 */

#ifndef PLC_PLATFORM_H
#define PLC_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ========== 平台检测 ========== */

#if defined(__ARM_LINUX) || defined(__aarch64__)
  #define PLATFORM_LINUX_ARM
#elif defined(__x86_64__) || defined(__i386__)
  #define PLATFORM_LINUX_X86
#elif defined(__XENOMAI__)
  #define PLATFORM_XENOMAI4
#elif defined(STM32F4xx) || defined(STM32F7xx)
  #define PLATFORM_STM32
#elif defined(ESP_PLATFORM) || defined(CONFIG_IDF_TARGET_ESP32)
  #define PLATFORM_ESP32
#elif defined(__riscv) && defined(PLATFORM_LITEX)
  #define PLATFORM_LITEX
#else
  #define PLATFORM_GENERIC
#endif

/* ========== 基本类型 ========== */

typedef int8_t    plc_sint;
typedef uint8_t   plc_usint;
typedef int16_t   plc_int;
typedef uint16_t  plc_uint;
typedef int32_t   plc_dint;
typedef uint32_t  plc_udint;
typedef int64_t   plc_lint;
typedef uint64_t  plc_ulint;
typedef float     plc_real;
typedef double    plc_lreal;
typedef bool      plc_bool;
typedef uint8_t   plc_byte;
typedef uint16_t  plc_word;
typedef uint32_t  plc_dword;
typedef uint64_t  plc_lword;
typedef uint32_t  plc_time;  /* 毫秒 */

typedef struct {
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint16_t ms;
} plc_datetime;

/* ========== 平台配置 ========== */

#ifndef PLC_MAX_TASKS
  #define PLC_MAX_TASKS           16
#endif

#ifndef PLC_MAX_VARIABLES
  #define PLC_MAX_VARIABLES       1024
#endif

#ifndef PLC_MAX_IO_CHANNELS
  #define PLC_MAX_IO_CHANNELS     256
#endif

#ifndef PLC_MAX_POUS
  #define PLC_MAX_POUS            64
#endif

#ifndef PLC_STACK_SIZE
  #ifdef PLATFORM_STM32
    #define PLC_STACK_SIZE         4096
  #elif defined(PLATFORM_LITEX)
    #define PLC_STACK_SIZE         16384  /* LiteX/RISC-V 有更多 RAM */
  #elif defined(PLATFORM_ESP32)
    #define PLC_STACK_SIZE         8192
  #else
    #define PLC_STACK_SIZE         65536
  #endif
#endif

#ifndef PLC_RUNTIME_TICK_MS
  #define PLC_RUNTIME_TICK_MS     1  /* 1ms 基础 tick */
#endif

/* ========== 平台接口（各平台实现） ========== */

/**
 * 初始化平台（硬件、时钟、中断等）
 */
void plc_platform_init(void);

/**
 * 获取当前时间戳（毫秒）
 */
uint32_t plc_platform_tick_ms(void);

/**
 * 获取高精度时间戳（微秒）
 */
uint64_t plc_platform_tick_us(void);

/**
 * 毫秒级延时
 */
void plc_platform_delay_ms(uint32_t ms);

/**
 * 微秒级延时
 */
void plc_platform_delay_us(uint32_t us);

/**
 * 临界区进入（禁用中断/获取互斥锁）
 */
void plc_platform_critical_enter(void);

/**
 * 临界区退出
 */
void plc_platform_critical_exit(void);

/**
 * 动态分配内存
 */
void* plc_platform_malloc(size_t size);

/**
 * 释放内存
 */
void plc_platform_free(void* ptr);

/**
 * 输出日志
 */
void plc_platform_log(uint8_t level, const char* fmt, ...);

/* 日志级别 */
#define PLC_LOG_ERROR   0
#define PLC_LOG_WARN    1
#define PLC_LOG_INFO    2
#define PLC_LOG_DEBUG   3
#define PLC_LOG_TRACE   4

/* ========== 通用 GPIO HAL（各平台实现，供 motion 层调用） ========== */

/**
 * 读取 GPIO 引脚值
 * @param addr 物理地址编码 (端口索引<<4 | 引脚号)
 * @return 0=低电平, 1=高电平
 */
int32_t plc_hal_gpio_read(uint32_t addr);

/**
 * 写入 GPIO 引脚值
 * @param addr 物理地址编码
 * @param value 0=低电平, 非0=高电平
 */
void plc_hal_gpio_write(uint32_t addr, int32_t value);

/**
 * 翻转 GPIO 引脚
 */
void plc_hal_gpio_toggle(uint32_t addr);

/**
 * 步进电机脉冲生成（一个完整脉冲: 拉高→2μs→拉低）
 * @param step_addr STEP 引脚物理地址
 * @param dir_addr  DIR 引脚物理地址
 * @param dir       方向 (0=负, 非0=正)
 */
void plc_hal_step_pulse(uint32_t step_addr, uint32_t dir_addr, int32_t dir);

#endif /* PLC_PLATFORM_H */
