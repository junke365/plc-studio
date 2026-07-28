/**
 * plc_uart.c - UART 硬件抽象层实现
 *
 * 使用静态通道数组管理多个 UART 实例
 * 内含环形缓冲区用于 TX/RX 数据缓存
 */

#include "plc_uart.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** 环形缓冲区 */
typedef struct {
  uint8_t*  buf;
  uint32_t  size;
  uint32_t  head;
  uint32_t  tail;
  uint32_t  count;
} RingBuf;

/** UART 实例运行状态 */
typedef struct {
  PlcUartConfig     config;
  bool              initialized;
  bool              open;
  uint32_t          rx_count;
  uint32_t          tx_count;
  uint32_t          rx_errors;
  PlcUartRxCallback rx_callback;
  /* 静态缓冲区 (内联分配) */
  uint8_t           rx_buf_data[PLC_UART_DEFAULT_BUF_SIZE];
  uint8_t           tx_buf_data[PLC_UART_DEFAULT_BUF_SIZE];
  RingBuf           rx_ring;
  RingBuf           tx_ring;
} UartInstance;

/* ========== 静态变量 ========== */

static UartInstance s_uart[PLC_UART_MAX_INSTANCES];

/* ========== 环形缓冲区辅助函数 ========== */

static void ring_init(RingBuf* ring, uint8_t* buf, uint32_t size) {
  ring->buf = buf;
  ring->size = size;
  ring->head = 0;
  ring->tail = 0;
  ring->count = 0;
}

static int ring_put(RingBuf* ring, uint8_t byte) {
  if (ring->count >= ring->size) {
    return -1; /* 满 */
  }
  ring->buf[ring->head] = byte;
  ring->head = (ring->head + 1) % ring->size;
  ring->count++;
  return 0;
}

static int ring_get(RingBuf* ring, uint8_t* byte) {
  if (ring->count == 0) {
    return -1; /* 空 */
  }
  *byte = ring->buf[ring->tail];
  ring->tail = (ring->tail + 1) % ring->size;
  ring->count--;
  return 0;
}

static uint32_t ring_available(const RingBuf* ring) {
  return ring->count;
}

static void ring_flush(RingBuf* ring) {
  ring->head = 0;
  ring->tail = 0;
  ring->count = 0;
}

/* ========== 模拟底层驱动 ========== */

static int uart_hw_send(uint8_t instance, uint8_t byte) {
  (void)instance;
  (void)byte;
  return 0;
}

static int uart_hw_recv(uint8_t instance, uint8_t* byte) {
  (void)instance;
  (void)byte;
  return -1; /* 默认无数据 */
}

/* ========== 公共接口实现 ========== */

int plc_uart_init(void) {
  memset(s_uart, 0, sizeof(s_uart));
  for (uint8_t i = 0; i < PLC_UART_MAX_INSTANCES; i++) {
    ring_init(&s_uart[i].rx_ring, s_uart[i].rx_buf_data,
              PLC_UART_DEFAULT_BUF_SIZE);
    ring_init(&s_uart[i].tx_ring, s_uart[i].tx_buf_data,
              PLC_UART_DEFAULT_BUF_SIZE);
  }
  return 0;
}

int plc_uart_open(uint8_t instance, const PlcUartConfig* config) {
  if (instance >= PLC_UART_MAX_INSTANCES || config == NULL) {
    return -1;
  }
  UartInstance* u = &s_uart[instance];

  /* 保存配置 */
  u->config = *config;

  /* 根据配置分配缓冲区大小 (使用内联缓冲区) */
  ring_init(&u->rx_ring, u->rx_buf_data,
            config->rx_buf_size ? config->rx_buf_size : PLC_UART_DEFAULT_BUF_SIZE);
  ring_init(&u->tx_ring, u->tx_buf_data,
            config->tx_buf_size ? config->tx_buf_size : PLC_UART_DEFAULT_BUF_SIZE);

  u->initialized = true;
  u->open = true;
  u->rx_count = 0;
  u->tx_count = 0;
  u->rx_errors = 0;
  return 0;
}

int plc_uart_close(uint8_t instance) {
  if (instance >= PLC_UART_MAX_INSTANCES) {
    return -1;
  }
  s_uart[instance].open = false;
  return 0;
}

int plc_uart_send(uint8_t instance, const uint8_t* data,
                  uint32_t len, uint32_t timeout_ms) {
  if (instance >= PLC_UART_MAX_INSTANCES || data == NULL) {
    return -1;
  }
  UartInstance* u = &s_uart[instance];
  if (!u->open) {
    return -2;
  }
  (void)timeout_ms;

  int sent = 0;
  for (uint32_t i = 0; i < len; i++) {
    if (ring_put(&u->tx_ring, data[i]) != 0) {
      break;
    }
    /* 尝试底层发送 */
    uart_hw_send(instance, data[i]);
    sent++;
  }
  u->tx_count += (uint32_t)sent;
  return sent;
}

int plc_uart_recv(uint8_t instance, uint8_t* buf,
                  uint32_t len, uint32_t timeout_ms) {
  if (instance >= PLC_UART_MAX_INSTANCES || buf == NULL) {
    return -1;
  }
  UartInstance* u = &s_uart[instance];
  if (!u->open) {
    return -2;
  }
  (void)timeout_ms;

  int received = 0;
  for (uint32_t i = 0; i < len; i++) {
    if (ring_get(&u->rx_ring, &buf[i]) != 0) {
      /* 尝试从底层读取 */
      if (uart_hw_recv(instance, &buf[i]) != 0) {
        break;
      }
    }
    received++;
  }
  u->rx_count += (uint32_t)received;
  return received;
}

int plc_uart_flush_rx(uint8_t instance) {
  if (instance >= PLC_UART_MAX_INSTANCES) {
    return -1;
  }
  ring_flush(&s_uart[instance].rx_ring);
  return 0;
}

int plc_uart_flush_tx(uint8_t instance) {
  if (instance >= PLC_UART_MAX_INSTANCES) {
    return -1;
  }
  ring_flush(&s_uart[instance].tx_ring);
  return 0;
}

int plc_uart_rx_available(uint8_t instance, uint32_t* count) {
  if (instance >= PLC_UART_MAX_INSTANCES || count == NULL) {
    return -1;
  }
  *count = ring_available(&s_uart[instance].rx_ring);
  return 0;
}

int plc_uart_set_rx_callback(uint8_t instance, PlcUartRxCallback callback) {
  if (instance >= PLC_UART_MAX_INSTANCES) {
    return -1;
  }
  s_uart[instance].rx_callback = callback;
  return 0;
}

int plc_uart_get_state(uint8_t instance, PlcUartState* state) {
  if (instance >= PLC_UART_MAX_INSTANCES || state == NULL) {
    return -1;
  }
  UartInstance* u = &s_uart[instance];
  state->initialized = u->initialized;
  state->open = u->open;
  state->rx_count = u->rx_count;
  state->tx_count = u->tx_count;
  state->rx_errors = u->rx_errors;
  return 0;
}
