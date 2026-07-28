/**
 * plc_i2c.h - I2C 硬件抽象层接口
 *
 * 提供统一的 I2C 主从通信接口
 * 支持标准模式、快速模式、快速模式+
 */

#ifndef PLC_I2C_H
#define PLC_I2C_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_I2C_MAX_BUSSES
  #ifdef PLATFORM_STM32
    #define PLC_I2C_MAX_BUSSES  3
  #elif defined(PLATFORM_ESP32)
    #define PLC_I2C_MAX_BUSSES  2
  #else
    #define PLC_I2C_MAX_BUSSES  8
  #endif
#endif

#ifndef PLC_I2C_XFER_MAX
  #define PLC_I2C_XFER_MAX   256
#endif

/* ========== 枚举定义 ========== */

/** I2C 速度模式 */
typedef enum {
  PLC_I2C_SPEED_STANDARD  = 100000,  /**< 标准模式 100 kHz */
  PLC_I2C_SPEED_FAST      = 400000,  /**< 快速模式 400 kHz */
  PLC_I2C_SPEED_FAST_PLUS = 1000000  /**< 快速模式+ 1 MHz */
} PlcI2cSpeed;

/** I2C 传输结果 */
typedef enum {
  PLC_I2C_ACK  = 0,  /**< 应答 */
  PLC_I2C_NACK = 1   /**< 无应答 */
} PlcI2cAck;

/* ========== 结构体定义 ========== */

/** I2C 总线配置 */
typedef struct {
  PlcI2cSpeed  clock_speed_hz;  /**< 时钟速率 */
  uint16_t     own_address;     /**< 本机地址 (从机模式, 7-bit) */
  bool         enable_pullup;   /**< 是否启用内部上拉 */
} PlcI2cConfig;

/** I2C 运行状态 */
typedef struct {
  bool     initialized;  /**< 是否已初始化 */
  uint32_t tx_count;     /**< 发送字节计数 */
  uint32_t rx_count;     /**< 接收字节计数 */
  uint32_t error_count;  /**< 错误计数 */
} PlcI2cState;

/* ========== 函数声明 ========== */

/**
 * 初始化 I2C 总线
 * @param bus_id 总线编号 (0-based)
 * @param config 配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_init(uint8_t bus_id, const PlcI2cConfig* config);

/**
 * 写入数据到指定从设备
 * @param bus_id    总线编号
 * @param addr      从设备 7-bit 地址
 * @param data      发送数据
 * @param len       数据长度
 * @param timeout_ms 超时毫秒
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_write(uint8_t bus_id, uint8_t addr, const uint8_t* data,
                  uint32_t len, uint32_t timeout_ms);

/**
 * 从指定从设备读取数据
 * @param bus_id    总线编号
 * @param addr      从设备 7-bit 地址
 * @param data      接收缓冲区
 * @param len       期望读取长度
 * @param timeout_ms 超时毫秒
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_read(uint8_t bus_id, uint8_t addr, uint8_t* data,
                 uint32_t len, uint32_t timeout_ms);

/**
 * 写入 8-bit 寄存器 (先发寄存器地址再发数据)
 * @param bus_id    总线编号
 * @param dev_addr  从设备地址
 * @param reg_addr  寄存器地址 (8-bit)
 * @param value     写入值
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_write_reg8(uint8_t bus_id, uint8_t dev_addr,
                       uint8_t reg_addr, uint8_t value);

/**
 * 读取 8-bit 寄存器
 * @param bus_id    总线编号
 * @param dev_addr  从设备地址
 * @param reg_addr  寄存器地址 (8-bit)
 * @return 寄存器值 (>=0), 负值为错误码
 */
int plc_i2c_read_reg8(uint8_t bus_id, uint8_t dev_addr, uint8_t reg_addr);

/**
 * 写入 16-bit 寄存器
 * @param bus_id    总线编号
 * @param dev_addr  从设备地址
 * @param reg_addr  寄存器地址 (16-bit, 大端先发高字节)
 * @param value     写入值 (16-bit)
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_write_reg16(uint8_t bus_id, uint8_t dev_addr,
                        uint16_t reg_addr, uint16_t value);

/**
 * 读取 16-bit 寄存器
 * @param bus_id    总线编号
 * @param dev_addr  从设备地址
 * @param reg_addr  寄存器地址 (16-bit)
 * @return 寄存器值 (>=0), 负值为错误码
 */
int plc_i2c_read_reg16(uint8_t bus_id, uint8_t dev_addr, uint16_t reg_addr);

/**
 * 探测 I2C 地址上的设备 (发送地址看是否有 ACK)
 * @param bus_id  总线编号
 * @param addr    要探测的 7-bit 地址
 * @return PLC_I2C_ACK 或 PLC_I2C_NACK, 负值为错误码
 */
int plc_i2c_probe(uint8_t bus_id, uint8_t addr);

/**
 * 获取 I2C 总线运行状态
 * @param bus_id 总线编号
 * @param state  输出参数: 运行状态
 * @return 0 成功, 负值为错误码
 */
int plc_i2c_get_state(uint8_t bus_id, PlcI2cState* state);

#ifdef __cplusplus
}
#endif

#endif /* PLC_I2C_H */
