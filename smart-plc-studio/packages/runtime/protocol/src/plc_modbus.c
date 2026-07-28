/**
 * plc_modbus.c - Modbus RTU/TCP 协议实现
 *
 * 实现 Modbus 主站/从站通信，支持 RTU（串口）和 TCP（以太网）传输
 * 包含 CRC16 查表算法、帧构建/解析、异常处理
 */

#include "plc_modbus.h"
#include "plc_platform.h"
#include "plc_uart.h"
#include "plc_eth.h"
#include <string.h>

/* ========== CRC16 查表 ========== */

/** Modbus CRC16 高字节查找表 */
static const uint16_t crc16_table[256] = {
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
  0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
  0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
  0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
  0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
  0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
  0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
  0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFB01, 0x3BC0, 0x3A80, 0xFA41,
  0xFE01, 0x3EC0, 0x3F80, 0xFF41, 0x3D00, 0xFDC1, 0xFC81, 0x3C40,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
  0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
  0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
  0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
  0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAB01, 0x6BC0, 0x6A80, 0xAA41,
  0xAE01, 0x6EC0, 0x6F80, 0xAF41, 0x6D00, 0xADC1, 0xAC81, 0x6C40,
  0x6400, 0xA4C1, 0xA581, 0x6540, 0xA701, 0x67C0, 0x6680, 0xA641,
  0xA201, 0x62C0, 0x6380, 0xA341, 0x6100, 0xA1C1, 0xA081, 0x6040,
  0x4000, 0x80C1, 0x8181, 0x4140, 0x8301, 0x43C0, 0x4280, 0x8241,
  0x8601, 0x46C0, 0x4780, 0x8741, 0x4500, 0x85C1, 0x8481, 0x4440,
  0x8C01, 0x4CC0, 0x4D80, 0x8D41, 0x4F00, 0x8FC1, 0x8E81, 0x4E40,
  0x4A00, 0x8AC1, 0x8B81, 0x4B40, 0x8901, 0x49C0, 0x4880, 0x8841,
  0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
  0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x8401, 0x44C0, 0x4580, 0x8541, 0x4700, 0x87C1, 0x8681, 0x4640,
  0x4200, 0x82C1, 0x8381, 0x4340, 0x8101, 0x41C0, 0x4080, 0x8041,
  0x8001, 0x40C0, 0x4180, 0x8141, 0x4300, 0x83C1, 0x8281, 0x4240,
  0x4600, 0x86C1, 0x8781, 0x4740, 0x8501, 0x45C0, 0x4480, 0x8441,
  0x8C01, 0x4CC0, 0x4D80, 0x8D41, 0x4F00, 0x8FC1, 0x8E81, 0x4E40,
};

/* ========== 内部状态 ========== */

/** 模块初始化状态 */
static plc_bool s_initialized = false;
static PlcModbusConfig s_config;

/** 映射表 */
static PlcModbusMapping s_hold_mappings[PLC_MODBUS_MAX_MAPPINGS];
static uint8_t s_hold_count = 0;
static PlcModbusMapping s_input_mappings[PLC_MODBUS_MAX_MAPPINGS];
static uint8_t s_input_count = 0;
static PlcModbusMapping s_coil_mappings[PLC_MODBUS_MAX_MAPPINGS];
static uint8_t s_coil_count = 0;
static PlcModbusMapping s_discrete_mappings[PLC_MODBUS_MAX_MAPPINGS];
static uint8_t s_discrete_count = 0;

/** 统计信息 */
static PlcModbusStats s_stats;

/** 缓冲区 */
static uint8_t s_tx_buf[PLC_MODBUS_MAX_FRAME];
static uint8_t s_rx_buf[PLC_MODBUS_MAX_FRAME];

/* ========== 内部函数 ========== */

/** 高字节在前写入大端序 */
static void write_uint16_be(uint8_t* buf, uint16_t value)
{
  buf[0] = (uint8_t)(value >> 8);
  buf[1] = (uint8_t)(value & 0xFF);
}

/** 从大端序读取 uint16 */
static uint16_t read_uint16_be(const uint8_t* buf)
{
  return (uint16_t)((uint16_t)buf[0] << 8 | buf[1]);
}

/** 通过 UART 发送 RTU 帧 */
static int rtu_send(const uint8_t* data, uint16_t len)
{
  if (plc_uart_send(s_config.uart_id, data, len, 100) < 0) {
    return -1;
  }
  s_stats.tx_count++;
  return 0;
}

/** 通过 UART 接收 RTU 帧 */
static int rtu_recv(uint8_t* data, uint16_t max_len, uint32_t timeout_ms)
{
  int ret = plc_uart_recv(s_config.uart_id, data, max_len, timeout_ms);
  if (ret < 0) {
    s_stats.timeout_count++;
    return -1;
  }
  s_stats.rx_count++;
  return ret;
}

/** 发送 TCP 帧 */
static int tcp_send(const uint8_t* data, uint16_t len)
{
  /* TCP 模式使用以太网 HAL（需底层 TCP 连接实现） */
  (void)data;
  (void)len;
  s_stats.tx_count++;
  return 0;
}

/** 接收 TCP 帧 */
static int tcp_recv(uint8_t* data, uint16_t max_len, uint32_t timeout_ms)
{
  (void)data;
  (void)max_len;
  (void)timeout_ms;
  s_stats.rx_count++;
  return 0;
}

/** 发送帧（自动选择传输方式） */
static int send_frame(const uint8_t* data, uint16_t len)
{
  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    return rtu_send(data, len);
  }
  return tcp_send(data, len);
}

/** 接收帧 */
static int recv_frame(uint8_t* data, uint16_t max_len, uint32_t timeout_ms)
{
  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    return rtu_recv(data, max_len, timeout_ms);
  }
  return tcp_recv(data, max_len, timeout_ms);
}

/** 构建异常响应帧 */
static uint16_t build_exception_frame(uint8_t* buf, uint8_t slave_id,
                                       uint8_t func_code, uint8_t exception)
{
  uint16_t pos = 0;

  if (s_config.transport == PLC_MODBUS_TRANSPORT_TCP) {
    /* TCP: [trans_id(2)][proto_id(2)][length(2)][unit_id][func_code|0x80][exception] */
    write_uint16_be(buf + pos, 0); /* 传输 ID */
    pos += 2;
    write_uint16_be(buf + pos, 0); /* 协议 ID */
    pos += 2;
    write_uint16_be(buf + pos, 3); /* 数据长度 */
    pos += 2;
    buf[pos++] = slave_id;
  } else {
    /* RTU: [slave_id][func_code|0x80][exception][CRC] */
    buf[pos++] = slave_id;
  }

  buf[pos++] = func_code | 0x80;
  buf[pos++] = exception;

  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    uint16_t crc = plc_modbus_crc16(buf, pos);
    buf[pos++] = (uint8_t)(crc & 0xFF);       /* CRC 低字节在前 */
    buf[pos++] = (uint8_t)(crc >> 8);
  }

  return pos;
}

/* ========== 寄存器/线圈数据存储 ========== */

static uint16_t s_hold_regs[PLC_MODBUS_MAX_REGISTERS];
static uint16_t s_input_regs[PLC_MODBUS_MAX_REGISTERS];
static uint8_t s_coils[PLC_MODBUS_MAX_COILS / 8];
static uint8_t s_discretes[PLC_MODBUS_MAX_COILS / 8];

/** 获取线圈位值 */
static plc_bool get_coil_bit(uint16_t addr)
{
  if (addr >= PLC_MODBUS_MAX_COILS) return false;
  return (s_coils[addr / 8] >> (addr % 8)) & 0x01;
}

/** 设置线圈位值 */
static void set_coil_bit(uint16_t addr, plc_bool value)
{
  if (addr >= PLC_MODBUS_MAX_COILS) return;
  if (value) {
    s_coils[addr / 8] |= (uint8_t)(1u << (addr % 8));
  } else {
    s_coils[addr / 8] &= (uint8_t)~(1u << (addr % 8));
  }
}

/** 获取离散输入位值 */
static plc_bool get_discrete_bit(uint16_t addr)
{
  if (addr >= PLC_MODBUS_MAX_COILS) return false;
  return (s_discretes[addr / 8] >> (addr % 8)) & 0x01;
}

/* ========== CRC16 ========== */

uint16_t plc_modbus_crc16(const uint8_t* data, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  uint16_t i;

  for (i = 0; i < len; i++) {
    crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
  }

  return crc;
}

/* ========== 公开接口实现 ========== */

int plc_modbus_init(const PlcModbusConfig* config)
{
  if (!config) return -1;

  memcpy(&s_config, config, sizeof(PlcModbusConfig));
  memset(&s_stats, 0, sizeof(PlcModbusStats));
  memset(s_hold_regs, 0, sizeof(s_hold_regs));
  memset(s_input_regs, 0, sizeof(s_input_regs));
  memset(s_coils, 0, sizeof(s_coils));
  memset(s_discretes, 0, sizeof(s_discretes));

  s_hold_count = 0;
  s_input_count = 0;
  s_coil_count = 0;
  s_discrete_count = 0;

  /* RTU 模式初始化 UART */
  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    PlcUartConfig uart_cfg = {
      .baud_rate = 9600,
      .data_bits = 8,
      .stop_bits = 1,
      .parity = PLC_UART_PARITY_NONE,
      .flow_control = PLC_UART_FLOW_NONE,
      .rx_buf_size = 256,
      .tx_buf_size = 256
    };
    if (plc_uart_open(s_config.uart_id, &uart_cfg) < 0) {
      return -2;
    }
  }

  s_initialized = true;
  plc_platform_log(PLC_LOG_INFO, "Modbus 初始化完成 (角色=%s, 传输=%s)",
                   s_config.role == PLC_MODBUS_ROLE_MASTER ? "主站" : "从站",
                   s_config.transport == PLC_MODBUS_TRANSPORT_RTU ? "RTU" : "TCP");
  return 0;
}

int plc_modbus_deinit(void)
{
  if (!s_initialized) return -1;

  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    plc_uart_close(s_config.uart_id);
  }

  s_initialized = false;
  return 0;
}

int plc_modbus_configure_mappings(PlcModbusMapType type,
                                   const PlcModbusMapping* mappings,
                                   uint8_t count)
{
  if (!s_initialized || !mappings) return -1;
  if (count > PLC_MODBUS_MAX_MAPPINGS) return -2;

  switch (type) {
    case PLC_MODBUS_MAP_HOLDING:
      memcpy(s_hold_mappings, mappings, count * sizeof(PlcModbusMapping));
      s_hold_count = count;
      break;
    case PLC_MODBUS_MAP_INPUT:
      memcpy(s_input_mappings, mappings, count * sizeof(PlcModbusMapping));
      s_input_count = count;
      break;
    case PLC_MODBUS_MAP_COILS:
      memcpy(s_coil_mappings, mappings, count * sizeof(PlcModbusMapping));
      s_coil_count = count;
      break;
    case PLC_MODBUS_MAP_DISCRETE:
      memcpy(s_discrete_mappings, mappings, count * sizeof(PlcModbusMapping));
      s_discrete_count = count;
      break;
    default:
      return -3;
  }

  return 0;
}

/* ========== 主站功能码实现 ========== */

/** 发送请求并等待响应（通用） */
static int master_request_response(uint8_t slave, uint8_t func_code,
                                    const uint8_t* req_data, uint16_t req_len,
                                    uint8_t* resp_data, uint16_t* resp_len)
{
  uint16_t frame_len = 0;
  uint16_t tx_len = 0;
  int recv_len;
  uint16_t expected_resp_len;

  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    /* 构建 RTU 请求帧 */
    s_tx_buf[tx_len++] = slave;
    s_tx_buf[tx_len++] = func_code;
    if (req_data && req_len > 0) {
      memcpy(s_tx_buf + tx_len, req_data, req_len);
      tx_len += req_len;
    }
    uint16_t crc = plc_modbus_crc16(s_tx_buf, tx_len);
    s_tx_buf[tx_len++] = (uint8_t)(crc & 0xFF);
    s_tx_buf[tx_len++] = (uint8_t)(crc >> 8);
    frame_len = tx_len;
  } else {
    /* 构建 TCP 请求帧 */
    tx_len = 6; /* 跳过 TCP 头部（传输ID + 协议ID + 长度） */
    s_tx_buf[tx_len++] = slave;
    s_tx_buf[tx_len++] = func_code;
    if (req_data && req_len > 0) {
      memcpy(s_tx_buf + tx_len, req_data, req_len);
      tx_len += req_len;
    }
    /* 填充 TCP 头部 */
    write_uint16_be(s_tx_buf, 0); /* 传输 ID */
    write_uint16_be(s_tx_buf + 2, 0); /* 协议 ID */
    write_uint16_be(s_tx_buf + 4, (uint16_t)(tx_len - 6)); /* 数据长度 */
    frame_len = tx_len;
  }

  /* 发送 */
  if (send_frame(s_tx_buf, frame_len) < 0) {
    return -10;
  }

  /* 接收响应 */
  expected_resp_len = (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) ? 256 : PLC_MODBUS_MAX_FRAME;
  recv_len = recv_frame(s_rx_buf, expected_resp_len, s_config.timeout_ms);
  if (recv_len < 4) {
    s_stats.error_count++;
    return -11;
  }

  /* 校验响应 */
  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    /* 校验 CRC */
    uint16_t recv_crc = (uint16_t)s_rx_buf[recv_len - 2] |
                        ((uint16_t)s_rx_buf[recv_len - 1] << 8);
    uint16_t calc_crc = plc_modbus_crc16(s_rx_buf, (uint16_t)(recv_len - 2));
    if (recv_crc != calc_crc) {
      s_stats.error_count++;
      return -12;
    }
    /* 检查异常码 */
    if (s_rx_buf[1] & 0x80) {
      s_stats.error_count++;
      return -(int)s_rx_buf[2]; /* 返回负异常码 */
    }
    if (resp_data) memcpy(resp_data, s_rx_buf + 2, (uint16_t)(recv_len - 4));
    if (resp_len) *resp_len = (uint16_t)(recv_len - 4);
  } else {
    /* TCP: 检查异常码 */
    if (s_rx_buf[7] & 0x80) {
      s_stats.error_count++;
      return -(int)s_rx_buf[9];
    }
    uint16_t tcp_data_len = read_uint16_be(s_rx_buf + 4);
    if (resp_data) memcpy(resp_data, s_rx_buf + 9, (uint16_t)(tcp_data_len - 2));
    if (resp_len) *resp_len = (uint16_t)(tcp_data_len - 2);
  }

  return 0;
}

int plc_modbus_read_holding(uint8_t slave, uint16_t addr,
                             uint16_t count, uint16_t* values)
{
  uint8_t req_data[4];
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint16_t resp_len = 0;
  int ret;
  uint16_t i;

  if (!values || count == 0) return -1;
  if (count > PLC_MODBUS_MAX_REGISTERS) return -2;

  /* 请求数据: 起始地址(2) + 数量(2) */
  write_uint16_be(req_data, addr);
  write_uint16_be(req_data + 2, count);

  ret = master_request_response(slave, PLC_MODBUS_FC_READ_HOLDING_REGS,
                                 req_data, 4, resp_data, &resp_len);
  if (ret < 0) return ret;

  /* 解析响应: [字节数(1)][数据(2*count)] */
  for (i = 0; i < count && (2 * i + 1) < resp_len; i++) {
    values[i] = read_uint16_be(resp_data + 1 + 2 * i);
  }

  return 0;
}

int plc_modbus_write_register(uint8_t slave, uint16_t addr, uint16_t value)
{
  uint8_t req_data[4];
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint16_t resp_len = 0;

  write_uint16_be(req_data, addr);
  write_uint16_be(req_data + 2, value);

  return master_request_response(slave, PLC_MODBUS_FC_WRITE_SINGLE_REG,
                                  req_data, 4, resp_data, &resp_len);
}

int plc_modbus_write_registers(uint8_t slave, uint16_t addr,
                                uint16_t count, const uint16_t* values)
{
  uint8_t req_data[PLC_MODBUS_MAX_FRAME];
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint16_t resp_len = 0;
  uint16_t i;
  uint16_t pos = 0;

  if (!values || count == 0) return -1;

  /* 请求: 起始地址(2) + 数量(2) + 字节数(1) + 数据 */
  write_uint16_be(req_data + pos, addr); pos += 2;
  write_uint16_be(req_data + pos, count); pos += 2;
  req_data[pos++] = (uint8_t)(count * 2);
  for (i = 0; i < count; i++) {
    write_uint16_be(req_data + pos, values[i]); pos += 2;
  }

  return master_request_response(slave, PLC_MODBUS_FC_WRITE_MULTIPLE_REGS,
                                  req_data, pos, resp_data, &resp_len);
}

int plc_modbus_read_coils(uint8_t slave, uint16_t addr,
                           uint16_t count, plc_bool* values)
{
  uint8_t req_data[4];
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint16_t resp_len = 0;
  int ret;
  uint16_t i;
  uint16_t byte_count;

  if (!values || count == 0) return -1;

  write_uint16_be(req_data, addr);
  write_uint16_be(req_data + 2, count);

  ret = master_request_response(slave, PLC_MODBUS_FC_READ_COILS,
                                 req_data, 4, resp_data, &resp_len);
  if (ret < 0) return ret;

  /* 解析: [字节数(1)][位数据] */
  byte_count = (count + 7) / 8;
  for (i = 0; i < count && (i / 8 + 1) < resp_len; i++) {
    values[i] = (plc_bool)(((resp_data[1 + i / 8] >> (i % 8)) & 0x01) != 0);
  }

  return 0;
}

int plc_modbus_write_coil(uint8_t slave, uint16_t addr, plc_bool value)
{
  uint8_t req_data[4];
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint16_t resp_len = 0;

  write_uint16_be(req_data, addr);
  write_uint16_be(req_data + 2, value ? 0xFF00 : 0x0000);

  return master_request_response(slave, PLC_MODBUS_FC_WRITE_SINGLE_COIL,
                                  req_data, 4, resp_data, &resp_len);
}

/* ========== 从站处理 ========== */

/** 处理读保持寄存器请求 */
static uint16_t slave_read_holding(uint16_t start_addr, uint16_t count,
                                    uint8_t* resp, uint16_t max_resp)
{
  uint16_t pos = 0;
  uint16_t i;
  uint16_t byte_count = count * 2;

  if (byte_count + 1 > max_resp) return 0;

  resp[pos++] = (uint8_t)byte_count;
  for (i = 0; i < count; i++) {
    uint16_t reg_addr = start_addr + i;
    if (reg_addr < PLC_MODBUS_MAX_REGISTERS) {
      write_uint16_be(resp + pos, s_hold_regs[reg_addr]);
    } else {
      write_uint16_be(resp + pos, 0);
    }
    pos += 2;
  }

  return pos;
}

/** 处理写单个寄存器请求 */
static uint16_t slave_write_single_reg(uint16_t addr, uint16_t value,
                                        uint8_t* resp, uint16_t max_resp)
{
  if (max_resp < 4) return 0;

  /* 正常响应回显请求数据 */
  write_uint16_be(resp, addr);
  write_uint16_be(resp + 2, value);

  if (addr < PLC_MODBUS_MAX_REGISTERS) {
    s_hold_regs[addr] = value;
  }

  return 4;
}

/** 处理写多个寄存器请求 */
static uint16_t slave_write_multiple_regs(uint16_t start_addr, uint16_t count,
                                           const uint8_t* data,
                                           uint8_t* resp, uint16_t max_resp)
{
  uint16_t i;
  uint16_t byte_count = count * 2;

  /* 写入数据 */
  for (i = 0; i < count; i++) {
    uint16_t reg_addr = start_addr + i;
    uint16_t value = read_uint16_be(data + i * 2);
    if (reg_addr < PLC_MODBUS_MAX_REGISTERS) {
      s_hold_regs[reg_addr] = value;
    }
  }

  /* 正常响应: 起始地址(2) + 数量(2) */
  if (max_resp < 4) return 0;
  write_uint16_be(resp, start_addr);
  write_uint16_be(resp + 2, count);
  return 4;
}

/** 处理读线圈请求 */
static uint16_t slave_read_coils(uint16_t start_addr, uint16_t count,
                                  uint8_t* resp, uint16_t max_resp)
{
  uint16_t byte_count = (count + 7) / 8;
  uint16_t i;
  uint16_t pos = 0;

  if (byte_count + 1 > max_resp) return 0;

  resp[pos++] = (uint8_t)byte_count;
  for (i = 0; i < count; i++) {
    uint16_t coil_addr = start_addr + i;
    if (i % 8 == 0) {
      resp[pos] = 0;
    }
    if (coil_addr < PLC_MODBUS_MAX_COILS && get_coil_bit(coil_addr)) {
      resp[pos] |= (uint8_t)(1u << (i % 8));
    }
    if (i % 8 == 7) pos++;
  }
  /* 处理最后不足 8 位的情况 */
  if (count % 8 != 0) pos++;

  return pos;
}

/** 处理写单个线圈请求 */
static uint16_t slave_write_single_coil(uint16_t addr, uint16_t value,
                                         uint8_t* resp, uint16_t max_resp)
{
  if (max_resp < 4) return 0;

  write_uint16_be(resp, addr);
  write_uint16_be(resp + 2, value);

  if (addr < PLC_MODBUS_MAX_COILS) {
    set_coil_bit(addr, value != 0x0000);
  }

  return 4;
}

int plc_modbus_slave_poll(void* var_table)
{
  uint16_t recv_len;
  uint8_t slave_id;
  uint8_t func_code;
  uint16_t start_addr;
  uint16_t count;
  uint16_t resp_len = 0;
  uint8_t resp_data[PLC_MODBUS_MAX_FRAME];
  uint8_t frame[PLC_MODBUS_MAX_FRAME];

  (void)var_table;

  if (!s_initialized || s_config.role != PLC_MODBUS_ROLE_SLAVE) return -1;

  /* 接收请求 */
  recv_len = (uint16_t)recv_frame(frame, PLC_MODBUS_MAX_FRAME, 10);
  if (recv_len < 4) return 0; /* 无数据 */

  /* 解析帧头 */
  if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
    slave_id = frame[0];
    func_code = frame[1];

    /* 校验目标地址 */
    if (slave_id != s_config.slave_id && slave_id != 0) return 0;
    if (recv_len < 4) return -1;

    /* 校验 CRC */
    uint16_t recv_crc = (uint16_t)frame[recv_len - 2] |
                        ((uint16_t)frame[recv_len - 1] << 8);
    uint16_t calc_crc = plc_modbus_crc16(frame, (uint16_t)(recv_len - 2));
    if (recv_crc != calc_crc) return -1;
  } else {
    /* TCP 模式 */
    slave_id = frame[6];
    func_code = frame[7];
    if (slave_id != s_config.slave_id && slave_id != 0) return 0;
  }

  /* 处理功能码 */
  switch (func_code) {
    case PLC_MODBUS_FC_READ_HOLDING_REGS:
      start_addr = read_uint16_be(frame + 2);
      count = read_uint16_be(frame + 4);
      resp_len = slave_read_holding(start_addr, count,
                                     resp_data, sizeof(resp_data));
      break;

    case PLC_MODBUS_FC_WRITE_SINGLE_REG:
      start_addr = read_uint16_be(frame + 2);
      count = read_uint16_be(frame + 4);
      resp_len = slave_write_single_reg(start_addr, count,
                                         resp_data, sizeof(resp_data));
      break;

    case PLC_MODBUS_FC_WRITE_MULTIPLE_REGS:
      start_addr = read_uint16_be(frame + 2);
      count = read_uint16_be(frame + 4);
      resp_len = slave_write_multiple_regs(start_addr, count,
                                            frame + 7,
                                            resp_data, sizeof(resp_data));
      break;

    case PLC_MODBUS_FC_READ_COILS:
      start_addr = read_uint16_be(frame + 2);
      count = read_uint16_be(frame + 4);
      resp_len = slave_read_coils(start_addr, count,
                                   resp_data, sizeof(resp_data));
      break;

    case PLC_MODBUS_FC_WRITE_SINGLE_COIL:
      start_addr = read_uint16_be(frame + 2);
      count = read_uint16_be(frame + 4);
      resp_len = slave_write_single_coil(start_addr, count,
                                          resp_data, sizeof(resp_data));
      break;

    default: {
      /* 不支持的功能码 → 异常响应 */
      resp_len = build_exception_frame(resp_data, slave_id, func_code,
                                        PLC_MODBUS_EX_ILLEGAL_FUNCTION);
      send_frame(resp_data, resp_len);
      s_stats.error_count++;
      return 0;
    }
  }

  if (resp_len == 0) {
    /* 数据地址错误异常 */
    resp_len = build_exception_frame(resp_data, slave_id, func_code,
                                      PLC_MODBUS_EX_ILLEGAL_DATA_ADDR);
    send_frame(resp_data, resp_len);
    s_stats.error_count++;
    return 0;
  }

  /* 构建完整响应帧并发送 */
  {
    uint16_t frame_pos = 0;

    if (s_config.transport == PLC_MODBUS_TRANSPORT_RTU) {
      s_tx_buf[frame_pos++] = s_config.slave_id;
      s_tx_buf[frame_pos++] = func_code;
      memcpy(s_tx_buf + frame_pos, resp_data, resp_len);
      frame_pos += resp_len;
      uint16_t crc = plc_modbus_crc16(s_tx_buf, frame_pos);
      s_tx_buf[frame_pos++] = (uint8_t)(crc & 0xFF);
      s_tx_buf[frame_pos++] = (uint8_t)(crc >> 8);
    } else {
      write_uint16_be(s_tx_buf, 0); /* 传输 ID */
      write_uint16_be(s_tx_buf + 2, 0); /* 协议 ID */
      write_uint16_be(s_tx_buf + 4, (uint16_t)(resp_len + 2));
      s_tx_buf[6] = s_config.slave_id;
      s_tx_buf[7] = func_code;
      memcpy(s_tx_buf + 8, resp_data, resp_len);
      frame_pos = 8 + resp_len;
    }

    send_frame(s_tx_buf, frame_pos);
  }

  s_stats.rx_count++;
  return 0;
}

int plc_modbus_get_stats(PlcModbusStats* stats)
{
  if (!stats) return -1;
  memcpy(stats, &s_stats, sizeof(PlcModbusStats));
  return 0;
}
