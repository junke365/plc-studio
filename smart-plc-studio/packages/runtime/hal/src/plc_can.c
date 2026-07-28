/**
 * plc_can.c - CAN 总线硬件抽象层实现
 *
 * 使用静态 CAN 接口数组管理多个实例
 * 内含 RX 环形缓冲区、过滤器匹配和统计追踪
 */

#include "plc_can.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** CAN 接收环形缓冲区 */
typedef struct {
  PlcCanMsg  buf[PLC_CAN_MAX_MSG];
  uint8_t    head;
  uint8_t    tail;
  uint8_t    count;
} CanRingBuf;

/** CAN 接口运行状态 */
typedef struct {
  PlcCanConfig   config;
  bool           initialized;
  bool           running;
  PlcCanState    state;
  PlcCanRxCallback rx_callback;
  CanRingBuf     rx_ring;
  /* 统计信息 */
  uint32_t       tx_count;
  uint32_t       rx_count;
  uint32_t       error_count;
  uint32_t       overrun_count;
} CanInstance;

/* ========== 静态变量 ========== */

#ifndef PLC_CAN_MAX_INSTANCES
  #define PLC_CAN_MAX_INSTANCES  4
#endif

static CanInstance s_canif[PLC_CAN_MAX_INSTANCES];

/* ========== 内部辅助函数 ========== */

/** 检查消息是否匹配过滤器 */
static bool can_filter_match(const PlcCanFilter* filter, const PlcCanMsg* msg) {
  if (!filter->enabled) {
    return true; /* 过滤器未启用则放行 */
  }
  if (filter->frame != msg->frame) {
    return false; /* 帧类型不匹配 */
  }
  return (msg->id & filter->mask) == (filter->id & filter->mask);
}

/** 将消息放入接收缓冲区 */
static void can_ring_put(CanRingBuf* ring, const PlcCanMsg* msg) {
  if (ring->count >= PLC_CAN_MAX_MSG) {
    return; /* 满, 丢弃 */
  }
  ring->buf[ring->head] = *msg;
  ring->head = (ring->head + 1) % PLC_CAN_MAX_MSG;
  ring->count++;
}

/** 从接收缓冲区取出消息 */
static bool can_ring_get(CanRingBuf* ring, PlcCanMsg* msg) {
  if (ring->count == 0) {
    return false;
  }
  *msg = ring->buf[ring->tail];
  ring->tail = (ring->tail + 1) % PLC_CAN_MAX_MSG;
  ring->count--;
  return true;
}

/* ========== 模拟底层驱动 ========== */

static int can_hw_send(uint8_t can_id, const PlcCanMsg* msg) {
  (void)can_id;
  (void)msg;
  return 0;
}

static int can_hw_start(uint8_t can_id) {
  (void)can_id;
  return 0;
}

static int can_hw_stop(uint8_t can_id) {
  (void)can_id;
  return 0;
}

/* ========== 公共接口实现 ========== */

int plc_can_init(uint8_t can_id, const PlcCanConfig* config) {
  if (can_id >= PLC_CAN_MAX_INSTANCES || config == NULL) {
    return -1;
  }
  memset(&s_canif[can_id], 0, sizeof(CanInstance));
  s_canif[can_id].config = *config;
  s_canif[can_id].initialized = true;
  s_canif[can_id].running = false;
  s_canif[can_id].state = PLC_CAN_STATE_STOPPED;
  return 0;
}

int plc_can_send(uint8_t can_id, const PlcCanMsg* msg) {
  if (can_id >= PLC_CAN_MAX_INSTANCES || msg == NULL) {
    return -1;
  }
  CanInstance* ci = &s_canif[can_id];
  if (!ci->running) {
    return -2;
  }

  int ret = can_hw_send(can_id, msg);
  if (ret == 0) {
    ci->tx_count++;
  } else {
    ci->error_count++;
  }
  return ret;
}

int plc_can_recv(uint8_t can_id, PlcCanMsg* msg, uint32_t timeout_ms) {
  if (can_id >= PLC_CAN_MAX_INSTANCES || msg == NULL) {
    return -1;
  }
  CanInstance* ci = &s_canif[can_id];
  if (!ci->running) {
    return -2;
  }
  (void)timeout_ms;

  /* 从缓冲区取消息 */
  if (can_ring_get(&ci->rx_ring, msg)) {
    ci->rx_count++;
    return 1;
  }
  return 0; /* 超时 */
}

int plc_can_start(uint8_t can_id) {
  if (can_id >= PLC_CAN_MAX_INSTANCES) {
    return -1;
  }
  int ret = can_hw_start(can_id);
  if (ret == 0) {
    s_canif[can_id].running = true;
    s_canif[can_id].state = PLC_CAN_STATE_ACTIVE;
  }
  return ret;
}

int plc_can_stop(uint8_t can_id) {
  if (can_id >= PLC_CAN_MAX_INSTANCES) {
    return -1;
  }
  int ret = can_hw_stop(can_id);
  if (ret == 0) {
    s_canif[can_id].running = false;
    s_canif[can_id].state = PLC_CAN_STATE_STOPPED;
  }
  return ret;
}

int plc_can_set_filter(uint8_t can_id, uint8_t filter_idx,
                        const PlcCanFilter* filter) {
  if (can_id >= PLC_CAN_MAX_INSTANCES || filter == NULL) {
    return -1;
  }
  if (filter_idx >= PLC_CAN_MAX_FILTERS) {
    return -2;
  }
  s_canif[can_id].config.filters[filter_idx] = *filter;
  return 0;
}

int plc_can_set_rx_callback(uint8_t can_id, PlcCanRxCallback callback) {
  if (can_id >= PLC_CAN_MAX_INSTANCES) {
    return -1;
  }
  s_canif[can_id].rx_callback = callback;
  return 0;
}

int plc_can_get_stats(uint8_t can_id, PlcCanStats* stats) {
  if (can_id >= PLC_CAN_MAX_INSTANCES || stats == NULL) {
    return -1;
  }
  CanInstance* ci = &s_canif[can_id];
  stats->tx_count = ci->tx_count;
  stats->rx_count = ci->rx_count;
  stats->error_count = ci->error_count;
  stats->overrun_count = ci->overrun_count;
  stats->state = ci->state;
  return 0;
}

int plc_can_clear_stats(uint8_t can_id) {
  if (can_id >= PLC_CAN_MAX_INSTANCES) {
    return -1;
  }
  CanInstance* ci = &s_canif[can_id];
  ci->tx_count = 0;
  ci->rx_count = 0;
  ci->error_count = 0;
  ci->overrun_count = 0;
  return 0;
}
