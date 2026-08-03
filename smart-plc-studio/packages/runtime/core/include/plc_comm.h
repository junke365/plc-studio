/**
 * plc_comm.h - 通信管理模块
 *
 * 支持多种通信协议：
 * - Modbus TCP/RTU
 * - OPC UA
 * - MQTT
 * - WebSocket（调试用）
 * - 串口（RS232/RS485）
 */

#ifndef PLC_COMM_H
#define PLC_COMM_H

#include "plc_platform.h"
#include "plc_var.h"

/* ========== 通信协议 ========== */

typedef enum {
  COMM_PROTO_MODBUS_TCP,
  COMM_PROTO_MODBUS_RTU,
  COMM_PROTO_OPC_UA,
  COMM_PROTO_MQTT,
  COMM_PROTO_WEBSOCKET,
  COMM_PROTO_SERIAL,
  COMM_PROTO_CUSTOM,
} CommProtocol;

/* 连接状态 */
typedef enum {
  COMM_STATE_DISCONNECTED,
  COMM_STATE_CONNECTING,
  COMM_STATE_CONNECTED,
  COMM_STATE_ERROR,
} CommState;

/* 通信接口 */
typedef struct {
  uint8_t        id;
  CommProtocol   protocol;
  CommState      state;
  const char*    name;
  const char*    host;
  uint16_t       port;
  uint32_t       baud_rate;    /* 串口波特率 */
  uint8_t        data_bits;
  uint8_t        stop_bits;
  uint8_t        parity;
  uint32_t       timeout_ms;
  uint32_t       rx_bytes;
  uint32_t       tx_bytes;
  uint32_t       rx_errors;
  uint32_t       tx_errors;
  void*          platform_data; /* 平台相关数据 */
} PlcCommChannel;

/* Modbus 寄存器映射 */
typedef struct {
  uint16_t       address;
  uint16_t       count;
  const char*    var_name;
  PlcVariable*   var;
  bool           writable;
} ModbusMapping;

/* Modbus 寄存器映射数量（STM32 嵌入式平台使用紧凑配置） */
#ifndef PLC_MAX_MODBUS_MAPPINGS
  #ifdef PLATFORM_STM32
    #define PLC_MAX_MODBUS_MAPPINGS 16
  #else
    #define PLC_MAX_MODBUS_MAPPINGS 128
  #endif
#endif

/* Modbus 配置 */
typedef struct {
  ModbusMapping  holdings[PLC_MAX_MODBUS_MAPPINGS];  /* 保持寄存器 */
  uint16_t       holding_count;
  ModbusMapping  inputs[PLC_MAX_MODBUS_MAPPINGS];    /* 输入寄存器 */
  uint16_t       input_count;
  uint16_t       coils[PLC_MAX_MODBUS_MAPPINGS];     /* 线圈 */
  uint16_t       coil_count;
  uint8_t        device_id;      /* 从站地址 */
} ModbusConfig;

/* ========== 接口函数 ========== */

/**
 * 初始化通信系统
 */
void plc_comm_init(void);

/**
 * 创建通信通道
 * @return 通道ID，-1=失败
 */
int plc_comm_create(CommProtocol protocol, const char* name,
                    const char* host, uint16_t port);

/**
 * 打开通信通道
 */
int plc_comm_open(uint8_t channel_id);

/**
 * 关闭通信通道
 */
void plc_comm_close(uint8_t channel_id);

/**
 * 发送数据
 */
int plc_comm_send(uint8_t channel_id, const uint8_t* data, uint32_t len);

/**
 * 接收数据
 */
int plc_comm_recv(uint8_t channel_id, uint8_t* data, uint32_t max_len, uint32_t timeout_ms);

/**
 * 配置 Modbus 映射
 */
void plc_comm_modbus_configure(uint8_t channel_id, const ModbusConfig* config);

/**
 * 处理 Modbus 请求（在通信任务中调用）
 */
void plc_comm_modbus_poll(uint8_t channel_id, PlcVarTable* var_table);

/**
 * 获取通道统计
 */
void plc_comm_get_stats(uint8_t channel_id, uint32_t* rx, uint32_t* tx,
                        uint32_t* rx_err, uint32_t* tx_err);

/* ========== 平台层实现 ========== */

/**
 * 平台层：建立 TCP 连接
 */
int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms);

/**
 * 平台层：关闭 TCP 连接
 */
void plc_hal_tcp_close(int fd);

/**
 * 平台层：TCP 发送
 */
int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len);

/**
 * 平台层：TCP 接收
 */
int plc_hal_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms);

/**
 * 平台层：打开串口
 */
int plc_hal_serial_open(const char* port_name, uint32_t baud_rate,
                        uint8_t data_bits, uint8_t stop_bits, uint8_t parity);

/**
 * 平台层：关闭串口
 */
void plc_hal_serial_close(int fd);

/**
 * 平台层：串口发送
 */
int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len);

/**
 * 平台层：串口接收
 */
int plc_hal_serial_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms);

#endif /* PLC_COMM_H */
