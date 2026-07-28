/**
 * plc_spi.c - SPI 硬件抽象层实现
 *
 * 使用静态总线数组管理多个 SPI 实例
 * 跟踪 CS 状态，支持字节级和缓冲区级传输
 */

#include "plc_spi.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** SPI 实例运行状态 */
typedef struct {
  PlcSpiConfig  config;
  bool          initialized;
  bool          cs_asserted;    /* CS 是否已拉低 (选中) */
  uint32_t      tx_bytes;
  uint32_t      rx_bytes;
} SpiInstance;

/* ========== 静态变量 ========== */

static SpiInstance s_spi[PLC_SPI_MAX_INSTANCES];

/* ========== 模拟底层驱动 ========== */

/** 模拟 SPI 硬件: 全双工单字节传输 */
static int spi_hw_transfer(uint8_t instance, uint8_t tx, uint8_t* rx) {
  (void)instance;
  /* 模拟传输: 发 0xFF 则收 0x00 */
  *rx = (tx == 0xFF) ? 0x00 : 0xFF;
  return 0;
}

/** 控制 CS 引脚 */
static int spi_hw_cs(uint8_t instance, bool assert) {
  (void)instance;
  (void)assert;
  return 0;
}

/* ========== 公共接口实现 ========== */

int plc_spi_init(void) {
  memset(s_spi, 0, sizeof(s_spi));
  return 0;
}

int plc_spi_open(uint8_t instance, const PlcSpiConfig* config) {
  if (instance >= PLC_SPI_MAX_INSTANCES || config == NULL) {
    return -1;
  }
  s_spi[instance].config = *config;
  s_spi[instance].initialized = true;
  s_spi[instance].cs_asserted = false;
  s_spi[instance].tx_bytes = 0;
  s_spi[instance].rx_bytes = 0;
  return 0;
}

int plc_spi_close(uint8_t instance) {
  if (instance >= PLC_SPI_MAX_INSTANCES) {
    return -1;
  }
  /* 关闭前先释放 CS */
  if (s_spi[instance].cs_asserted) {
    plc_spi_cs_control(instance, false);
  }
  s_spi[instance].initialized = false;
  return 0;
}

int plc_spi_transfer(uint8_t instance, uint8_t tx_data, uint8_t* rx_data) {
  if (instance >= PLC_SPI_MAX_INSTANCES || rx_data == NULL) {
    return -1;
  }
  if (!s_spi[instance].initialized) {
    return -2;
  }

  int ret = spi_hw_transfer(instance, tx_data, rx_data);
  if (ret == 0) {
    s_spi[instance].tx_bytes++;
    s_spi[instance].rx_bytes++;
  }
  return ret;
}

int plc_spi_transfer_buf(uint8_t instance, const uint8_t* tx_buf,
                          uint8_t* rx_buf, uint32_t len) {
  if (instance >= PLC_SPI_MAX_INSTANCES) {
    return -1;
  }
  if (!s_spi[instance].initialized) {
    return -2;
  }
  if (len == 0) {
    return 0;
  }

  for (uint32_t i = 0; i < len; i++) {
    uint8_t tx = tx_buf ? tx_buf[i] : 0xFF;
    uint8_t rx = 0;
    int ret = spi_hw_transfer(instance, tx, &rx);
    if (ret != 0) {
      return ret;
    }
    if (rx_buf) {
      rx_buf[i] = rx;
    }
  }
  s_spi[instance].tx_bytes += len;
  s_spi[instance].rx_bytes += len;
  return 0;
}

int plc_spi_write_read(uint8_t instance, const uint8_t* tx_buf,
                        uint32_t tx_len, uint8_t* rx_buf, uint32_t rx_len) {
  if (instance >= PLC_SPI_MAX_INSTANCES) {
    return -1;
  }
  if (!s_spi[instance].initialized) {
    return -2;
  }

  /* 先发送 */
  for (uint32_t i = 0; i < tx_len; i++) {
    uint8_t dummy;
    int ret = spi_hw_transfer(instance, tx_buf ? tx_buf[i] : 0xFF, &dummy);
    if (ret != 0) return ret;
  }

  /* 再接收 (发送 0xFF 产生时钟) */
  for (uint32_t i = 0; i < rx_len; i++) {
    uint8_t rx = 0;
    int ret = spi_hw_transfer(instance, 0xFF, &rx);
    if (ret != 0) return ret;
    if (rx_buf) {
      rx_buf[i] = rx;
    }
  }

  s_spi[instance].tx_bytes += tx_len;
  s_spi[instance].rx_bytes += rx_len;
  return 0;
}

int plc_spi_cs_control(uint8_t instance, bool assert) {
  if (instance >= PLC_SPI_MAX_INSTANCES) {
    return -1;
  }
  int ret = spi_hw_cs(instance, assert);
  if (ret == 0) {
    s_spi[instance].cs_asserted = assert;
  }
  return ret;
}

int plc_spi_bus_scan(uint8_t instance, uint8_t* found, uint8_t* addresses) {
  if (instance >= PLC_SPI_MAX_INSTANCES || found == NULL || addresses == NULL) {
    return -1;
  }
  *found = 0;

  /* SPI 总线扫描: 逐个拉低 CS 并尝试读取 ID */
  for (uint8_t addr = 0; addr < 8; addr++) {
    plc_spi_cs_control(instance, true);
    uint8_t rx = 0;
    spi_hw_transfer(instance, 0x9F, &rx); /* 发送 JEDEC ID 命令 */
    spi_hw_transfer(instance, 0xFF, &rx); /* 读取 ID 字节 */
    plc_spi_cs_control(instance, false);

    if (rx != 0x00 && rx != 0xFF) {
      addresses[*found] = addr;
      (*found)++;
    }
  }
  return 0;
}

int plc_spi_get_state(uint8_t instance, PlcSpiState* state) {
  if (instance >= PLC_SPI_MAX_INSTANCES || state == NULL) {
    return -1;
  }
  state->initialized = s_spi[instance].initialized;
  state->tx_bytes = s_spi[instance].tx_bytes;
  state->rx_bytes = s_spi[instance].rx_bytes;
  return 0;
}
