/**
 * plc_spi.h - SPI 硬件抽象层接口
 *
 * 提供统一的 SPI 通信接口
 * 支持全双工传输、CS 控制、总线扫描
 */

#ifndef PLC_SPI_H
#define PLC_SPI_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_SPI_MAX_INSTANCES
  #ifdef PLATFORM_STM32
    #define PLC_SPI_MAX_INSTANCES  3
  #elif defined(PLATFORM_ESP32)
    #define PLC_SPI_MAX_INSTANCES  2
  #else
    #define PLC_SPI_MAX_INSTANCES  8
  #endif
#endif

/* ========== 枚举定义 ========== */

/** SPI 时钟极性/相位模式 */
typedef enum {
  PLC_SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0 */
  PLC_SPI_MODE_1 = 1,  /**< CPOL=0, CPHA=1 */
  PLC_SPI_MODE_2 = 2,  /**< CPOL=1, CPHA=0 */
  PLC_SPI_MODE_3 = 3   /**< CPOL=1, CPHA=1 */
} PlcSpiMode;

/** SPI 位序 */
typedef enum {
  PLC_SPI_MSB_FIRST = 0,  /**< 高位在先 */
  PLC_SPI_LSB_FIRST = 1   /**< 低位在先 */
} PlcSpiBitOrder;

/* ========== 结构体定义 ========== */

/** SPI 配置参数 */
typedef struct {
  PlcSpiMode     mode;       /**< SPI 模式 (0-3) */
  PlcSpiBitOrder bit_order;  /**< 位序 */
  uint32_t       clock_hz;   /**< 时钟频率 (Hz) */
  uint8_t        cs_port;    /**< 片选引脚端口号 (0xFF 表示手动控制) */
  uint8_t        cs_pin;     /**< 片选引脚引脚号 */
} PlcSpiConfig;

/** SPI 运行状态 */
typedef struct {
  bool     initialized;  /**< 是否已初始化 */
  uint32_t tx_bytes;     /**< 发送字节计数 */
  uint32_t rx_bytes;     /**< 接收字节计数 */
} PlcSpiState;

/* ========== 函数声明 ========== */

/**
 * 初始化 SPI 子系统
 * @return 0 成功, 负值为错误码
 */
int plc_spi_init(void);

/**
 * 配置并打开 SPI 实例
 * @param instance SPI 实例号 (0-based)
 * @param config   配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_spi_open(uint8_t instance, const PlcSpiConfig* config);

/**
 * 关闭 SPI 实例
 * @param instance SPI 实例号
 * @return 0 成功, 负值为错误码
 */
int plc_spi_close(uint8_t instance);

/**
 * 全双工传输单字节
 * @param instance SPI 实例号
 * @param tx_data  发送字节
 * @param rx_data  输出参数: 接收字节
 * @return 0 成功, 负值为错误码
 */
int plc_spi_transfer(uint8_t instance, uint8_t tx_data, uint8_t* rx_data);

/**
 * 全双工传输缓冲区
 * @param instance SPI 实例号
 * @param tx_buf   发送缓冲区 (可为 NULL, 则发送 0xFF)
 * @param rx_buf   接收缓冲区 (可为 NULL, 则丢弃接收数据)
 * @param len      传输字节数
 * @return 0 成功, 负值为错误码
 */
int plc_spi_transfer_buf(uint8_t instance, const uint8_t* tx_buf,
                         uint8_t* rx_buf, uint32_t len);

/**
 * 先发送再接收（半双工模式）
 * @param instance SPI 实例号
 * @param tx_buf   发送数据
 * @param tx_len   发送长度
 * @param rx_buf   接收缓冲区
 * @param rx_len   期望接收长度
 * @return 0 成功, 负值为错误码
 */
int plc_spi_write_read(uint8_t instance, const uint8_t* tx_buf,
                       uint32_t tx_len, uint8_t* rx_buf, uint32_t rx_len);

/**
 * 控制片选信号
 * @param instance SPI 实例号
 * @param assert   true=拉低CS（选中）, false=拉高CS（释放）
 * @return 0 成功, 负值为错误码
 */
int plc_spi_cs_control(uint8_t instance, bool assert);

/**
 * SPI 总线扫描（探测所有可能的从设备地址）
 * @param instance  SPI 实例号
 * @param found     输出参数: 发现的从设备数量
 * @param addresses 输出参数: 发现的从设备地址数组 (至少 128 个元素)
 * @return 0 成功, 负值为错误码
 */
int plc_spi_bus_scan(uint8_t instance, uint8_t* found,
                     uint8_t* addresses);

/**
 * 获取 SPI 实例运行状态
 * @param instance SPI 实例号
 * @param state    输出参数: 运行状态
 * @return 0 成功, 负值为错误码
 */
int plc_spi_get_state(uint8_t instance, PlcSpiState* state);

#ifdef __cplusplus
}
#endif

#endif /* PLC_SPI_H */
