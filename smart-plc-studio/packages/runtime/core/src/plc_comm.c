/**
 * plc_comm.c - 通信管理模块实现
 *
 * 实现通道管理和 Modbus TCP 基本功能
 * 支持功能码：0x03（读保持寄存器）、0x06（写单个寄存器）、0x10（写多个寄存器）
 */

#include "plc_comm.h"
#include <string.h>

/* ========== Modbus 常量 ========== */

#define MODBUS_FC_READ_HOLDINGS   0x03
#define MODBUS_FC_READ_INPUTS     0x04
#define MODBUS_FC_WRITE_SINGLE    0x06
#define MODBUS_FC_WRITE_MULTIPLE  0x10

#define MODBUS_MAX_PDU_SIZE       253
#define MODBUS_MBAP_HEADER_SIZE   7

/* Modbus 异常码 */
#define MODBUS_EX_NONE            0x00
#define MODBUS_EX_ILLEGAL_FUNC    0x01
#define MODBUS_EX_ILLEGAL_ADDR    0x02
#define MODBUS_EX_ILLEGAL_VALUE   0x03

/* ========== 通道管理 ========== */

/* 通信通道数组 */
static PlcCommChannel g_channels[PLC_MAX_TASKS]; /* 复用最大任务数作为最大通道数 */
static uint8_t g_channel_count = 0;

/* Modbus 配置数组（每个通道一个） */
static ModbusConfig g_modbus_configs[PLC_MAX_TASKS];
static bool g_modbus_configured[PLC_MAX_TASKS];

void plc_comm_init(void) {
  memset(g_channels, 0, sizeof(g_channels));
  memset(g_modbus_configs, 0, sizeof(g_modbus_configs));
  memset(g_modbus_configured, 0, sizeof(g_modbus_configured));
  g_channel_count = 0;
}

int plc_comm_create(CommProtocol protocol, const char* name,
                    const char* host, uint16_t port) {
  if (name == NULL || g_channel_count >= PLC_MAX_TASKS) return -1;

  uint8_t id = g_channel_count;
  PlcCommChannel* ch = &g_channels[id];

  ch->id = id;
  ch->protocol = protocol;
  ch->state = COMM_STATE_DISCONNECTED;
  ch->name = name;
  ch->host = host;
  ch->port = port;
  ch->baud_rate = 0;
  ch->data_bits = 8;
  ch->stop_bits = 1;
  ch->parity = 0;
  ch->timeout_ms = 3000;
  ch->rx_bytes = 0;
  ch->tx_bytes = 0;
  ch->rx_errors = 0;
  ch->tx_errors = 0;
  ch->platform_data = NULL;

  g_channel_count++;

  return (int)id;
}

int plc_comm_open(uint8_t channel_id) {
  if (channel_id >= g_channel_count) return -1;

  PlcCommChannel* ch = &g_channels[channel_id];
  ch->state = COMM_STATE_CONNECTING;

  switch (ch->protocol) {
    case COMM_PROTO_MODBUS_TCP:
    case COMM_PROTO_WEBSOCKET: {
      /* 建立 TCP 连接 */
      int fd = plc_hal_tcp_connect(ch->host, ch->port, ch->timeout_ms);
      if (fd < 0) {
        ch->state = COMM_STATE_ERROR;
        ch->tx_errors++;
        return -1;
      }
      ch->platform_data = (void*)(intptr_t)fd;
      ch->state = COMM_STATE_CONNECTED;
      break;
    }
    case COMM_PROTO_SERIAL:
    case COMM_PROTO_MODBUS_RTU: {
      /* 打开串口 */
      int fd = plc_hal_serial_open(ch->name, ch->baud_rate,
                                    ch->data_bits, ch->stop_bits, ch->parity);
      if (fd < 0) {
        ch->state = COMM_STATE_ERROR;
        ch->tx_errors++;
        return -1;
      }
      ch->platform_data = (void*)(intptr_t)fd;
      ch->state = COMM_STATE_CONNECTED;
      break;
    }
    default:
      ch->state = COMM_STATE_ERROR;
      return -1;
  }

  return 0;
}

void plc_comm_close(uint8_t channel_id) {
  if (channel_id >= g_channel_count) return;

  PlcCommChannel* ch = &g_channels[channel_id];
  if (ch->state != COMM_STATE_CONNECTED) return;

  int fd = (int)(intptr_t)ch->platform_data;

  switch (ch->protocol) {
    case COMM_PROTO_MODBUS_TCP:
    case COMM_PROTO_WEBSOCKET:
      plc_hal_tcp_close(fd);
      break;
    case COMM_PROTO_SERIAL:
    case COMM_PROTO_MODBUS_RTU:
      plc_hal_serial_close(fd);
      break;
    default:
      break;
  }

  ch->platform_data = NULL;
  ch->state = COMM_STATE_DISCONNECTED;
}

int plc_comm_send(uint8_t channel_id, const uint8_t* data, uint32_t len) {
  if (channel_id >= g_channel_count || data == NULL) return -1;

  PlcCommChannel* ch = &g_channels[channel_id];
  if (ch->state != COMM_STATE_CONNECTED) return -1;

  int fd = (int)(intptr_t)ch->platform_data;
  int sent = 0;

  switch (ch->protocol) {
    case COMM_PROTO_MODBUS_TCP:
    case COMM_PROTO_WEBSOCKET:
      sent = plc_hal_tcp_send(fd, data, len);
      break;
    case COMM_PROTO_SERIAL:
    case COMM_PROTO_MODBUS_RTU:
      sent = plc_hal_serial_send(fd, data, len);
      break;
    default:
      return -1;
  }

  if (sent < 0) {
    ch->tx_errors++;
    return -1;
  }

  ch->tx_bytes += (uint32_t)sent;
  return sent;
}

int plc_comm_recv(uint8_t channel_id, uint8_t* data, uint32_t max_len, uint32_t timeout_ms) {
  if (channel_id >= g_channel_count || data == NULL) return -1;

  PlcCommChannel* ch = &g_channels[channel_id];
  if (ch->state != COMM_STATE_CONNECTED) return -1;

  int fd = (int)(intptr_t)ch->platform_data;
  int received = 0;

  switch (ch->protocol) {
    case COMM_PROTO_MODBUS_TCP:
    case COMM_PROTO_WEBSOCKET:
      received = plc_hal_tcp_recv(fd, data, max_len, timeout_ms);
      break;
    case COMM_PROTO_SERIAL:
    case COMM_PROTO_MODBUS_RTU:
      received = plc_hal_serial_recv(fd, data, max_len, timeout_ms);
      break;
    default:
      return -1;
  }

  if (received < 0) {
    ch->rx_errors++;
    return -1;
  }

  ch->rx_bytes += (uint32_t)received;
  return received;
}

/* ========== Modbus TCP 实现 ========== */

/* CRC16 计算（Modbus RTU 用） */
static uint16_t modbus_crc16(const uint8_t* data, uint32_t len) {
  uint16_t crc = 0xFFFF;
  for (uint32_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void plc_comm_modbus_configure(uint8_t channel_id, const ModbusConfig* config) {
  if (channel_id >= g_channel_count || config == NULL) return;

  memcpy(&g_modbus_configs[channel_id], config, sizeof(ModbusConfig));
  g_modbus_configured[channel_id] = true;
}

/* 构建 Modbus 异常响应 */
static uint32_t modbus_build_exception(uint8_t* buf, uint8_t transaction_id,
                                        uint8_t function_code, uint8_t exception) {
  /* MBAP 头部 */
  buf[0] = transaction_id >> 8;
  buf[1] = transaction_id & 0xFF;
  buf[2] = 0;  /* 协议标识（Modbus = 0） */
  buf[3] = 0;
  buf[4] = 0;  /* 长度（后面字节数） */
  buf[5] = 3;
  buf[6] = 0;  /* 单元标识（从站地址） */
  /* PDU */
  buf[7] = function_code | 0x80;  /* 异常响应 = 功能码 + 0x80 */
  buf[8] = exception;

  return 9;  /* MBAP(7) + 异常码(2) */
}

/* 处理功能码 0x03：读保持寄存器 */
static uint32_t modbus_handle_read_holdings(const uint8_t* request, uint32_t req_len,
                                             uint8_t* response, uint8_t transaction_id,
                                             ModbusConfig* config, PlcVarTable* var_table) {
  if (req_len < 11) {  /* MBAP(7) + FC(1) + Addr(2) + Count(2) = 12, 最少需要11字节索引 */
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_READ_HOLDINGS, MODBUS_EX_ILLEGAL_VALUE);
  }

  uint16_t start_addr = (request[8] << 8) | request[9];
  uint16_t reg_count = (request[10] << 8) | request[11];

  /* 检查寄存器数量 */
  if (reg_count < 1 || reg_count > 125) {
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_READ_HOLDINGS, MODBUS_EX_ILLEGAL_VALUE);
  }

  /* 构建响应 */
  response[0] = transaction_id >> 8;
  response[1] = transaction_id & 0xFF;
  response[2] = 0;  /* 协议标识 */
  response[3] = 0;
  response[4] = 0;  /* 长度（占位） */
  response[5] = 0;
  response[6] = 0;  /* 单元标识 */
  response[7] = MODBUS_FC_READ_HOLDINGS;

  uint8_t byte_count = (uint8_t)(reg_count * 2);
  response[8] = byte_count;

  uint32_t offset = 9;
  uint16_t regs_read = 0;

  for (uint16_t i = 0; i < reg_count; i++) {
    uint16_t addr = start_addr + i;

    /* 查找该地址对应的变量 */
    bool found = false;
    for (uint16_t m = 0; m < config->holding_count; m++) {
      if (config->holdings[m].address == addr) {
        /* 从变量读取数据 */
        PlcVariable* var = config->holdings[m].var;
        if (var == NULL && var_table != NULL) {
          var = plc_var_find(var_table, config->holdings[m].var_name);
          config->holdings[m].var = var;
        }

        if (var != NULL && var->data != NULL) {
          /* 读取 16 位寄存器值 */
          uint16_t val = 0;
          if (var->size >= 2) {
            val = (uint16_t)(*(uint16_t*)var->data);
          } else if (var->size == 1) {
            val = (uint16_t)(*(uint8_t*)var->data);
          }
          response[offset] = val >> 8;
          response[offset + 1] = val & 0xFF;
        } else {
          response[offset] = 0;
          response[offset + 1] = 0;
        }

        found = true;
        offset += 2;
        regs_read++;
        break;
      }
    }

    if (!found) {
      /* 地址未映射，返回 0 */
      response[offset] = 0;
      response[offset + 1] = 0;
      offset += 2;
      regs_read++;
    }
  }

  /* 更新长度字段 */
  uint16_t pdu_len = 2 + regs_read * 2;  /* FC(1) + byte_count(1) + data */
  response[4] = (uint8_t)(pdu_len >> 8);
  response[5] = (uint8_t)(pdu_len & 0xFF);

  return (uint32_t)(offset);
}

/* 处理功能码 0x06：写单个保持寄存器 */
static uint32_t modbus_handle_write_single(const uint8_t* request, uint32_t req_len,
                                            uint8_t* response, uint8_t transaction_id,
                                            ModbusConfig* config, PlcVarTable* var_table) {
  if (req_len < 12) {
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_WRITE_SINGLE, MODBUS_EX_ILLEGAL_VALUE);
  }

  uint16_t addr = (request[8] << 8) | request[9];
  uint16_t value = (request[10] << 8) | request[11];

  /* 查找对应变量并写入 */
  bool found = false;
  for (uint16_t m = 0; m < config->holding_count; m++) {
    if (config->holdings[m].address == addr && config->holdings[m].writable) {
      PlcVariable* var = config->holdings[m].var;
      if (var == NULL && var_table != NULL) {
        var = plc_var_find(var_table, config->holdings[m].var_name);
        config->holdings[m].var = var;
      }

      if (var != NULL && var->data != NULL) {
        if (var->size >= 2) {
          *(uint16_t*)var->data = value;
        } else if (var->size == 1) {
          *(uint8_t*)var->data = (uint8_t)(value & 0xFF);
        }
      }

      found = true;
      break;
    }
  }

  if (!found) {
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_WRITE_SINGLE, MODBUS_EX_ILLEGAL_ADDR);
  }

  /* 正常响应：回显请求 */
  memcpy(response, request, 12);
  return 12;
}

/* 处理功能码 0x10：写多个保持寄存器 */
static uint32_t modbus_handle_write_multiple(const uint8_t* request, uint32_t req_len,
                                              uint8_t* response, uint8_t transaction_id,
                                              ModbusConfig* config, PlcVarTable* var_table) {
  if (req_len < 15) {  /* MBAP(7) + FC(1) + Addr(2) + Count(2) + ByteCount(1) + 至少1个值(2) */
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_WRITE_MULTIPLE, MODBUS_EX_ILLEGAL_VALUE);
  }

  uint16_t start_addr = (request[8] << 8) | request[9];
  uint16_t reg_count = (request[10] << 8) | request[11];
  uint8_t byte_count = request[12];

  /* 验证数据长度 */
  if (reg_count < 1 || reg_count > 123 || byte_count != reg_count * 2) {
    return modbus_build_exception(response, transaction_id,
                                  MODBUS_FC_WRITE_MULTIPLE, MODBUS_EX_ILLEGAL_VALUE);
  }

  /* 逐个寄存器写入 */
  for (uint16_t i = 0; i < reg_count; i++) {
    uint16_t addr = start_addr + i;
    uint16_t value = (request[13 + i * 2] << 8) | request[14 + i * 2];

    for (uint16_t m = 0; m < config->holding_count; m++) {
      if (config->holdings[m].address == addr && config->holdings[m].writable) {
        PlcVariable* var = config->holdings[m].var;
        if (var == NULL && var_table != NULL) {
          var = plc_var_find(var_table, config->holdings[m].var_name);
          config->holdings[m].var = var;
        }

        if (var != NULL && var->data != NULL) {
          if (var->size >= 2) {
            *(uint16_t*)var->data = value;
          } else if (var->size == 1) {
            *(uint8_t*)var->data = (uint8_t)(value & 0xFF);
          }
        }
        break;
      }
    }
  }

  /* 正常响应 */
  response[0] = transaction_id >> 8;
  response[1] = transaction_id & 0xFF;
  response[2] = 0;
  response[3] = 0;
  response[4] = 0;
  response[5] = 6;  /* 长度 */
  response[6] = 0;
  response[7] = MODBUS_FC_WRITE_MULTIPLE;
  response[8] = start_addr >> 8;
  response[9] = start_addr & 0xFF;
  response[10] = reg_count >> 8;
  response[11] = reg_count & 0xFF;

  return 12;
}

void plc_comm_modbus_poll(uint8_t channel_id, PlcVarTable* var_table) {
  if (channel_id >= g_channel_count) return;
  if (!g_modbus_configured[channel_id]) return;

  PlcCommChannel* ch = &g_channels[channel_id];
  if (ch->state != COMM_STATE_CONNECTED) return;

  ModbusConfig* config = &g_modbus_configs[channel_id];

  /* 接收请求 */
  uint8_t request[MODBUS_MAX_PDU_SIZE + MODBUS_MBAP_HEADER_SIZE + 1];
  uint8_t response[MODBUS_MAX_PDU_SIZE + MODBUS_MBAP_HEADER_SIZE + 1];

  int received = plc_comm_recv(channel_id, request, sizeof(request), ch->timeout_ms);
  if (received <= 0) return;

  /* 至少需要 MBAP 头部(7) + FC(1) */
  if (received < 8) return;

  /* 解析 MBAP 头部 */
  uint16_t transaction_id = (request[0] << 8) | request[1];
  uint16_t protocol_id = (request[2] << 8) | request[3];
  /* uint16_t length = (request[4] << 8) | request[5]; */
  uint8_t unit_id = request[6];

  /* 检查协议标识（必须为 0 = Modbus） */
  if (protocol_id != 0) return;

  /* 检查从站地址 */
  if (config->device_id != 0 && unit_id != config->device_id) return;

  /* 根据功能码分发处理 */
  uint8_t function_code = request[7];
  uint32_t resp_len = 0;

  switch (function_code) {
    case MODBUS_FC_READ_HOLDINGS:
      resp_len = modbus_handle_read_holdings(request, (uint32_t)received,
                                              response, transaction_id, config, var_table);
      break;
    case MODBUS_FC_WRITE_SINGLE:
      resp_len = modbus_handle_write_single(request, (uint32_t)received,
                                             response, transaction_id, config, var_table);
      break;
    case MODBUS_FC_WRITE_MULTIPLE:
      resp_len = modbus_handle_write_multiple(request, (uint32_t)received,
                                               response, transaction_id, config, var_table);
      break;
    default:
      /* 不支持的功能码，返回异常 */
      resp_len = modbus_build_exception(response, transaction_id,
                                         function_code, MODBUS_EX_ILLEGAL_FUNC);
      break;
  }

  /* 发送响应 */
  if (resp_len > 0) {
    plc_comm_send(channel_id, response, (uint32_t)resp_len);
  }
}

void plc_comm_get_stats(uint8_t channel_id, uint32_t* rx, uint32_t* tx,
                        uint32_t* rx_err, uint32_t* tx_err) {
  if (channel_id >= g_channel_count) return;

  PlcCommChannel* ch = &g_channels[channel_id];
  if (rx != NULL) *rx = ch->rx_bytes;
  if (tx != NULL) *tx = ch->tx_bytes;
  if (rx_err != NULL) *rx_err = ch->rx_errors;
  if (tx_err != NULL) *tx_err = ch->tx_errors;
}
