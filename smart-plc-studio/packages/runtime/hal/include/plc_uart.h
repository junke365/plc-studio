/**
 * plc_uart.h - UART 硬件抽象层接口
 *
 * 提供统一的串口通信接口
 * 支持多种波特率、数据位、停止位、校验和流控配置
 */

#ifndef PLC_UART_H
#define PLC_UART_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_UART_MAX_INSTANCES
  #ifdef PLATFORM_STM32
    #define PLC_UART_MAX_INSTANCES  6
  #elif defined(PLATFORM_ESP32)
    #define PLC_UART_MAX_INSTANCES  3
  #else
    #define PLC_UART_MAX_INSTANCES  16
  #endif
#endif

#ifndef PLC_UART_DEFAULT_BUF_SIZE
  #define PLC_UART_DEFAULT_BUF_SIZE  256
#endif

/* ========== 枚举定义 ========== */

/** UART 校验位 */
typedef enum {
  PLC_UART_PARITY_NONE  = 0,  /**< 无校验 */
  PLC_UART_PARITY_ODD   = 1,  /**< 奇校验 */
  PLC_UART_PARITY_EVEN  = 2   /**< 偶校验 */
} PlcUartParity;

/** UART 流控方式 */
typedef enum {
  PLC_UART_FLOW_NONE    = 0,  /**< 无流控 */
  PLC_UART_FLOW_RTSCTS  = 1,  /**< 硬件流控 (RTS/CTS) */
  PLC_UART_FLOW_XONXOFF = 2   /**< 软件流控 (XON/XOFF) */
} PlcUartFlowControl;

/* ========== 结构体定义 ========== */

/** UART 配置参数 */
typedef struct {
  uint32_t             baud_rate;      /**< 波特率 (如 9600, 115200) */
  uint8_t              data_bits;      /**< 数据位 (5, 6, 7, 8) */
  uint8_t              stop_bits;      /**< 停止位 (1, 2) */
  PlcUartParity        parity;         /**< 校验方式 */
  PlcUartFlowControl   flow_control;   /**< 流控方式 */
  uint32_t             rx_buf_size;    /**< 接收缓冲区大小 (0=使用默认值) */
  uint32_t             tx_buf_size;    /**< 发送缓冲区大小 (0=使用默认值) */
} PlcUartConfig;

/** UART 运行状态 */
typedef struct {
  bool     initialized;   /**< 是否已初始化 */
  bool     open;          /**< 是否已打开 */
  uint32_t rx_count;      /**< 接收字节计数 */
  uint32_t tx_count;      /**< 发送字节计数 */
  uint32_t rx_errors;     /**< 接收错误计数 */
} PlcUartState;

/** UART 接收回调函数原型 */
typedef void (*PlcUartRxCallback)(uint8_t instance, uint8_t byte);

/* ========== 函数声明 ========== */

/**
 * 初始化 UART 子系统
 * @return 0 成功, 负值为错误码
 */
int plc_uart_init(void);

/**
 * 打开并配置指定 UART 实例
 * @param instance UART 实例号 (0-based)
 * @param config   配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_uart_open(uint8_t instance, const PlcUartConfig* config);

/**
 * 关闭指定 UART 实例
 * @param instance UART 实例号
 * @return 0 成功, 负值为错误码
 */
int plc_uart_close(uint8_t instance);

/**
 * 发送数据（阻塞）
 * @param instance UART 实例号
 * @param data     发送数据指针
 * @param len      发送数据长度
 * @param timeout_ms 超时时间（毫秒），0=永久等待
 * @return 实际发送的字节数, 负值为错误码
 */
int plc_uart_send(uint8_t instance, const uint8_t* data,
                  uint32_t len, uint32_t timeout_ms);

/**
 * 接收数据（阻塞）
 * @param instance UART 实例号
 * @param buf      接收缓冲区
 * @param len      期望接收长度
 * @param timeout_ms 超时时间（毫秒），0=永久等待
 * @return 实际接收的字节数, 负值为错误码
 */
int plc_uart_recv(uint8_t instance, uint8_t* buf,
                  uint32_t len, uint32_t timeout_ms);

/**
 * 刷新接收缓冲区（丢弃未读数据）
 * @param instance UART 实例号
 * @return 0 成功, 负值为错误码
 */
int plc_uart_flush_rx(uint8_t instance);

/**
 * 刷新发送缓冲区（等待发送完成）
 * @param instance UART 实例号
 * @return 0 成功, 负值为错误码
 */
int plc_uart_flush_tx(uint8_t instance);

/**
 * 查询接收缓冲区中的可用字节数
 * @param instance UART 实例号
 * @param count    输出参数: 可用字节数
 * @return 0 成功, 负值为错误码
 */
int plc_uart_rx_available(uint8_t instance, uint32_t* count);

/**
 * 注册 UART 接收回调函数（每收到一个字节触发）
 * @param instance UART 实例号
 * @param callback 回调函数（传 NULL 取消注册）
 * @return 0 成功, 负值为错误码
 */
int plc_uart_set_rx_callback(uint8_t instance, PlcUartRxCallback callback);

/**
 * 获取 UART 实例运行状态
 * @param instance UART 实例号
 * @param state    输出参数: 运行状态
 * @return 0 成功, 负值为错误码
 */
int plc_uart_get_state(uint8_t instance, PlcUartState* state);

#ifdef __cplusplus
}
#endif

#endif /* PLC_UART_H */
