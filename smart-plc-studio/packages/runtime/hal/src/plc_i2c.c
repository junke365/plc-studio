/**
 * plc_i2c.c - I2C 硬件抽象层实现
 *
 * 使用静态总线数组管理多个 I2C 实例
 * 支持 ACK/NACK 探测、8/16 位寄存器读写
 */

#include "plc_i2c.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** I2C 总线运行状态 */
typedef struct {
  PlcI2cConfig  config;
  bool          initialized;
  uint32_t      tx_count;
  uint32_t      rx_count;
  uint32_t      error_count;
} I2cBus;

/* ========== 静态变量 ========== */

static I2cBus s_bus[PLC_I2C_MAX_BUSSES];

/* ========== 模拟底层驱动 ========== */

/** 模拟 I2C: 发送起始位 + 地址 + 写数据 */
static int i2c_hw_write(uint8_t bus_id, uint8_t addr, const uint8_t* data,
                         uint32_t len) {
  (void)bus_id;
  (void)addr;
  (void)data;
  (void)len;
  return 0;
}

/** 模拟 I2C: 发送起始位 + 地址 + 读数据 */
static int i2c_hw_read(uint8_t bus_id, uint8_t addr, uint8_t* data,
                        uint32_t len) {
  (void)bus_id;
  (void)addr;
  memset(data, 0, len);
  return 0;
}

/** 模拟 I2C: 发送起始位 + 地址，检测 ACK/NACK */
static int i2c_hw_probe(uint8_t bus_id, uint8_t addr) {
  (void)bus_id;
  (void)addr;
  return 0; /* 返回 0 表示 ACK */
}

/* ========== 公共接口实现 ========== */

int plc_i2c_init(uint8_t bus_id, const PlcI2cConfig* config) {
  if (bus_id >= PLC_I2C_MAX_BUSSES || config == NULL) {
    return -1;
  }
  s_bus[bus_id].config = *config;
  s_bus[bus_id].initialized = true;
  s_bus[bus_id].tx_count = 0;
  s_bus[bus_id].rx_count = 0;
  s_bus[bus_id].error_count = 0;
  return 0;
}

int plc_i2c_write(uint8_t bus_id, uint8_t addr, const uint8_t* data,
                  uint32_t len, uint32_t timeout_ms) {
  if (bus_id >= PLC_I2C_MAX_BUSSES || data == NULL) {
    return -1;
  }
  if (!s_bus[bus_id].initialized) {
    return -2;
  }
  if (len > PLC_I2C_XFER_MAX) {
    return -3;
  }
  (void)timeout_ms;

  int ret = i2c_hw_write(bus_id, addr, data, len);
  if (ret == 0) {
    s_bus[bus_id].tx_count += len;
  } else {
    s_bus[bus_id].error_count++;
  }
  return ret;
}

int plc_i2c_read(uint8_t bus_id, uint8_t addr, uint8_t* data,
                 uint32_t len, uint32_t timeout_ms) {
  if (bus_id >= PLC_I2C_MAX_BUSSES || data == NULL) {
    return -1;
  }
  if (!s_bus[bus_id].initialized) {
    return -2;
  }
  if (len > PLC_I2C_XFER_MAX) {
    return -3;
  }
  (void)timeout_ms;

  int ret = i2c_hw_read(bus_id, addr, data, len);
  if (ret == 0) {
    s_bus[bus_id].rx_count += len;
  } else {
    s_bus[bus_id].error_count++;
  }
  return ret;
}

int plc_i2c_write_reg8(uint8_t bus_id, uint8_t dev_addr,
                       uint8_t reg_addr, uint8_t value) {
  if (bus_id >= PLC_I2C_MAX_BUSSES) {
    return -1;
  }
  /* 组装: [寄存器地址, 数据] */
  uint8_t buf[2];
  buf[0] = reg_addr;
  buf[1] = value;
  return plc_i2c_write(bus_id, dev_addr, buf, 2, 100);
}

int plc_i2c_read_reg8(uint8_t bus_id, uint8_t dev_addr, uint8_t reg_addr) {
  if (bus_id >= PLC_I2C_MAX_BUSSES) {
    return -1;
  }
  /* 先写寄存器地址，再读 1 字节 */
  int ret = plc_i2c_write(bus_id, dev_addr, &reg_addr, 1, 100);
  if (ret != 0) {
    return ret;
  }
  uint8_t val = 0;
  ret = plc_i2c_read(bus_id, dev_addr, &val, 1, 100);
  if (ret != 0) {
    return ret;
  }
  return (int)val;
}

int plc_i2c_write_reg16(uint8_t bus_id, uint8_t dev_addr,
                        uint16_t reg_addr, uint16_t value) {
  if (bus_id >= PLC_I2C_MAX_BUSSES) {
    return -1;
  }
  /* 组装: [寄存器高字节, 寄存器低字节, 数据高字节, 数据低字节] */
  uint8_t buf[4];
  buf[0] = (uint8_t)(reg_addr >> 8);
  buf[1] = (uint8_t)(reg_addr & 0xFF);
  buf[2] = (uint8_t)(value >> 8);
  buf[3] = (uint8_t)(value & 0xFF);
  return plc_i2c_write(bus_id, dev_addr, buf, 4, 100);
}

int plc_i2c_read_reg16(uint8_t bus_id, uint8_t dev_addr, uint16_t reg_addr) {
  if (bus_id >= PLC_I2C_MAX_BUSSES) {
    return -1;
  }
  /* 先发 16 位寄存器地址 (大端) */
  uint8_t addr_buf[2];
  addr_buf[0] = (uint8_t)(reg_addr >> 8);
  addr_buf[1] = (uint8_t)(reg_addr & 0xFF);
  int ret = plc_i2c_write(bus_id, dev_addr, addr_buf, 2, 100);
  if (ret != 0) {
    return ret;
  }
  /* 读取 2 字节 */
  uint8_t val_buf[2] = {0, 0};
  ret = plc_i2c_read(bus_id, dev_addr, val_buf, 2, 100);
  if (ret != 0) {
    return ret;
  }
  return (int)((uint16_t)val_buf[0] << 8 | val_buf[1]);
}

int plc_i2c_probe(uint8_t bus_id, uint8_t addr) {
  if (bus_id >= PLC_I2C_MAX_BUSSES) {
    return -1;
  }
  int ret = i2c_hw_probe(bus_id, addr);
  return ret == 0 ? PLC_I2C_ACK : PLC_I2C_NACK;
}

int plc_i2c_get_state(uint8_t bus_id, PlcI2cState* state) {
  if (bus_id >= PLC_I2C_MAX_BUSSES || state == NULL) {
    return -1;
  }
  state->initialized = s_bus[bus_id].initialized;
  state->tx_count = s_bus[bus_id].tx_count;
  state->rx_count = s_bus[bus_id].rx_count;
  state->error_count = s_bus[bus_id].error_count;
  return 0;
}
