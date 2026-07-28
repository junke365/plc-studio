/**
 * plc_gpio.h - GPIO 硬件抽象层接口
 *
 * 提供统一的 GPIO 输入/输出、中断回调等接口
 * 支持 STM32、ESP32、Linux 等平台
 */

#ifndef PLC_GPIO_H
#define PLC_GPIO_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_GPIO_MAX_PINS
  #ifdef PLATFORM_STM32
    #define PLC_GPIO_MAX_PINS   16
  #elif defined(PLATFORM_ESP32)
    #define PLC_GPIO_MAX_PINS   40
  #else
    #define PLC_GPIO_MAX_PINS   64
  #endif
#endif

#ifndef PLC_GPIO_MAX_PORTS
  #ifdef PLATFORM_STM32
    #define PLC_GPIO_MAX_PORTS  8
  #else
    #define PLC_GPIO_MAX_PORTS  4
  #endif
#endif

/* ========== 枚举定义 ========== */

/** GPIO 工作模式 */
typedef enum {
  PLC_GPIO_MODE_INPUT        = 0,  /**< 浮空输入 */
  PLC_GPIO_MODE_OUTPUT       = 1,  /**< 推挽输出 */
  PLC_GPIO_MODE_INPUT_PULLUP = 2,  /**< 上拉输入 */
  PLC_GPIO_MODE_INPUT_PULLDOWN = 3, /**< 下拉输入 */
  PLC_GPIO_MODE_AF           = 4   /**< 复用功能 */
} PlcGpioMode;

/** GPIO 边沿触发类型 */
typedef enum {
  PLC_GPIO_EDGE_NONE    = 0,  /**< 无中断 */
  PLC_GPIO_EDGE_RISING  = 1,  /**< 上升沿 */
  PLC_GPIO_EDGE_FALLING = 2,  /**< 下降沿 */
  PLC_GPIO_EDGE_BOTH    = 3   /**< 双边沿 */
} PlcGpioEdge;

/** GPIO 速度等级 */
typedef enum {
  PLC_GPIO_SPEED_LOW    = 0,  /**< 低速 (2 MHz) */
  PLC_GPIO_SPEED_MEDIUM = 1,  /**< 中速 (25 MHz) */
  PLC_GPIO_SPEED_HIGH   = 2,  /**< 高速 (50 MHz) */
  PLC_GPIO_SPEED_VHIGH  = 3   /**< 最高速 (100 MHz) */
} PlcGpioSpeed;

/* ========== 结构体定义 ========== */

/** GPIO 引脚配置描述 */
typedef struct {
  uint8_t       port;      /**< 端口号 (0-based) */
  uint8_t       pin;       /**< 引脚号 (0-based) */
  PlcGpioMode   mode;      /**< 工作模式 */
  PlcGpioSpeed  speed;     /**< 输出速度 */
  uint8_t       af_num;    /**< 复用功能编号 (AF 模式有效, 0-15) */
  const char*   label;     /**< 引脚标签 (可选, 用于调试) */
} PlcGpioPin;

/** GPIO 中断回调函数原型 */
typedef void (*PlcGpioIsrCallback)(uint8_t port, uint8_t pin, PlcGpioEdge edge);

/** GPIO 初始化状态信息 */
typedef struct {
  uint8_t  configured_pins;  /**< 已配置的引脚数 */
  uint8_t  port_count;       /**< 可用端口数 */
  uint8_t  port_pins[PLC_GPIO_MAX_PORTS]; /**< 每个端口的引脚数 */
} PlcGpioStatus;

/* ========== 函数声明 ========== */

/**
 * 初始化 GPIO 子系统
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_init(void);

/**
 * 配置指定引脚的工作模式
 * @param port 端口号
 * @param pin  引脚号
 * @param mode 工作模式
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_set_mode(uint8_t port, uint8_t pin, PlcGpioMode mode);

/**
 * 设置引脚输出电平
 * @param port  端口号
 * @param pin   引脚号
 * @param value true=高电平, false=低电平
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_write(uint8_t port, uint8_t pin, bool value);

/**
 * 读取引脚输入电平
 * @param port  端口号
 * @param pin   引脚号
 * @param value 输出参数: 引脚电平值
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_read(uint8_t port, uint8_t pin, bool* value);

/**
 * 翻转引脚输出电平
 * @param port 端口号
 * @param pin  引脚号
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_toggle(uint8_t port, uint8_t pin);

/**
 * 读取端口全部引脚的电平（位掩码）
 * @param port    端口号
 * @param values  输出参数: 位掩码，bit 0 对应 pin 0
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_read_port(uint8_t port, uint32_t* values);

/**
 * 批量写入端口全部引脚的电平（位掩码）
 * @param port    端口号
 * @param mask    要写入的引脚掩码
 * @param values  要写入的值（bit 0 对应 pin 0）
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_write_port(uint8_t port, uint32_t mask, uint32_t values);

/**
 * 注册 GPIO 中断回调函数
 * @param port     端口号
 * @param pin      引脚号
 * @param edge     触发边沿类型
 * @param callback 回调函数（传 NULL 取消中断）
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_set_isr_callback(uint8_t port, uint8_t pin,
                              PlcGpioEdge edge,
                              PlcGpioIsrCallback callback);

/**
 * 获取可用端口数量
 * @return 端口数
 */
uint8_t plc_gpio_get_port_count(void);

/**
 * 获取指定端口的引脚数量
 * @param port 端口号
 * @return 引脚数
 */
uint8_t plc_gpio_get_pin_count(uint8_t port);

/**
 * 获取 GPIO 子系统状态
 * @param status 输出参数: 状态信息
 * @return 0 成功, 负值为错误码
 */
int plc_gpio_get_status(PlcGpioStatus* status);

#ifdef __cplusplus
}
#endif

#endif /* PLC_GPIO_H */
