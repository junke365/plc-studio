/**
 * plc_modbus.h - Modbus RTU/TCP 协议接口
 *
 * 支持 Modbus RTU（串口）和 Modbus TCP（以太网）两种传输方式
 * 支持主站（Master）和从站（Slave）模式
 * 实现标准功能码：0x01-0x06, 0x0F, 0x10
 */

#ifndef PLC_MODBUS_H
#define PLC_MODBUS_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_MODBUS_MAX_MAPPINGS
  #define PLC_MODBUS_MAX_MAPPINGS    64
#endif

#ifndef PLC_MODBUS_MAX_REGISTERS
  #define PLC_MODBUS_MAX_REGISTERS   128
#endif

#ifndef PLC_MODBUS_MAX_COILS
  #define PLC_MODBUS_MAX_COILS       2048
#endif

#ifndef PLC_MODBUS_TCP_DEFAULT_PORT
  #define PLC_MODBUS_TCP_DEFAULT_PORT 502
#endif

#ifndef PLC_MODBUS_MAX_FRAME
  #define PLC_MODBUS_MAX_FRAME       256
#endif

/* ========== 功能码定义 ========== */

#define PLC_MODBUS_FC_READ_COILS            0x01
#define PLC_MODBUS_FC_READ_DISCRETE_INPUTS  0x02
#define PLC_MODBUS_FC_READ_HOLDING_REGS     0x03
#define PLC_MODBUS_FC_READ_INPUT_REGS       0x04
#define PLC_MODBUS_FC_WRITE_SINGLE_COIL     0x05
#define PLC_MODBUS_FC_WRITE_SINGLE_REG      0x06
#define PLC_MODBUS_FC_WRITE_MULTIPLE_COILS  0x0F
#define PLC_MODBUS_FC_WRITE_MULTIPLE_REGS   0x10

/* ========== 异常码定义 ========== */

#define PLC_MODBUS_EX_ILLEGAL_FUNCTION   0x01
#define PLC_MODBUS_EX_ILLEGAL_DATA_ADDR  0x02
#define PLC_MODBUS_EX_ILLEGAL_DATA_VALUE 0x03
#define PLC_MODBUS_EX_SLAVE_FAILURE      0x04

/* ========== 枚举定义 ========== */

/** Modbus 角色 */
typedef enum {
  PLC_MODBUS_ROLE_MASTER = 0,
  PLC_MODBUS_ROLE_SLAVE  = 1
} PlcModbusRole;

/** Modbus 传输方式 */
typedef enum {
  PLC_MODBUS_TRANSPORT_RTU = 0,
  PLC_MODBUS_TRANSPORT_TCP = 1
} PlcModbusTransport;

/** 映射数据类型 */
typedef enum {
  PLC_MODBUS_MAP_HOLDING     = 0,  /* 保持寄存器 */
  PLC_MODBUS_MAP_INPUT       = 1,  /* 输入寄存器 */
  PLC_MODBUS_MAP_COILS       = 2,  /* 线圈 */
  PLC_MODBUS_MAP_DISCRETE    = 3   /* 离散输入 */
} PlcModbusMapType;

/* ========== 结构体定义 ========== */

/** Modbus 配置参数 */
typedef struct {
  PlcModbusRole       role;           /* 主站/从站 */
  PlcModbusTransport  transport;      /* RTU/TCP */
  uint8_t             slave_id;       /* 从站地址 (1-247) */
  uint8_t             uart_id;        /* RTU 模式使用的 UART 编号 */
  uint16_t            tcp_port;       /* TCP 模式监听/连接端口 */
  uint32_t            timeout_ms;     /* 响应超时（毫秒） */
} PlcModbusConfig;

/** Modbus 地址映射 */
typedef struct {
  uint16_t      address;       /* Modbus 起始地址 */
  uint16_t      count;         /* 寄存器/线圈数量 */
  const char*   var_name;      /* 绑定的变量名（用于调试） */
  void*         var_ptr;       /* 绑定的变量指针 */
  plc_bool      writable;      /* 是否可写 */
} PlcModbusMapping;

/** Modbus 统计信息 */
typedef struct {
  uint32_t  tx_count;          /* 发送帧计数 */
  uint32_t  rx_count;          /* 接收帧计数 */
  uint32_t  error_count;       /* 错误计数 */
  uint32_t  timeout_count;     /* 超时计数 */
} PlcModbusStats;

/* ========== 函数声明 ========== */

/**
 * 初始化 Modbus 协议栈
 * @param config 配置参数
 * @return 0 成功, 负值错误码
 */
int plc_modbus_init(const PlcModbusConfig* config);

/**
 * 反初始化 Modbus 协议栈
 * @return 0 成功
 */
int plc_modbus_deinit(void);

/**
 * 配置数据映射表
 * @param type     数据类型 (holding/input/coils/discrete)
 * @param mappings 映射数组
 * @param count    映射数量
 * @return 0 成功, 负值错误码
 */
int plc_modbus_configure_mappings(PlcModbusMapType type,
                                   const PlcModbusMapping* mappings,
                                   uint8_t count);

/**
 * 读取保持寄存器（主站）
 * @param slave  从站地址
 * @param addr   起始地址
 * @param count  寄存器数量
 * @param values 输出缓冲区
 * @return 0 成功, 负值错误码
 */
int plc_modbus_read_holding(uint8_t slave, uint16_t addr,
                             uint16_t count, uint16_t* values);

/**
 * 写入单个寄存器（主站）
 * @param slave 从站地址
 * @param addr  寄存器地址
 * @param value 写入值
 * @return 0 成功, 负值错误码
 */
int plc_modbus_write_register(uint8_t slave, uint16_t addr, uint16_t value);

/**
 * 写入多个寄存器（主站）
 * @param slave  从站地址
 * @param addr   起始地址
 * @param count  寄存器数量
 * @param values 写入数据
 * @return 0 成功, 负值错误码
 */
int plc_modbus_write_registers(uint8_t slave, uint16_t addr,
                                uint16_t count, const uint16_t* values);

/**
 * 读取线圈状态（主站）
 * @param slave  从站地址
 * @param addr   起始地址
 * @param count  线圈数量
 * @param values 输出位数组（每个元素一个线圈状态）
 * @return 0 成功, 负值错误码
 */
int plc_modbus_read_coils(uint8_t slave, uint16_t addr,
                           uint16_t count, plc_bool* values);

/**
 * 写入单个线圈（主站）
 * @param slave 从站地址
 * @param addr  线圈地址
 * @param value 线圈状态
 * @return 0 成功, 负值错误码
 */
int plc_modbus_write_coil(uint8_t slave, uint16_t addr, plc_bool value);

/**
 * 从站轮询处理（在主循环中调用）
 * @param var_table 变量表指针（可选，NULL 使用配置的映射）
 * @return 0 成功, 负值错误码
 */
int plc_modbus_slave_poll(void* var_table);

/**
 * 获取统计信息
 * @param stats 输出统计信息
 * @return 0 成功
 */
int plc_modbus_get_stats(PlcModbusStats* stats);

/**
 * 计算 Modbus CRC16
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC16 值
 */
uint16_t plc_modbus_crc16(const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PLC_MODBUS_H */
