/**
 * plc_eth.c - 以太网硬件抽象层实现
 *
 * 使用静态接口数组管理多个以太网实例
 * 跟踪 MAC/IP/链路状态和 DHCP 状态
 */

#include "plc_eth.h"
#include <string.h>

/* ========== 常量定义 ========== */

#ifndef PLC_ETH_MAX_INSTANCES
  #define PLC_ETH_MAX_INSTANCES  4
#endif

/* ========== 内部数据结构 ========== */

/** DHCP 状态 */
typedef enum {
  DHCP_IDLE = 0,
  DHCP_DISCOVER,
  DHCP_REQUEST,
  DHCP_BOUND,
} DhcpState;

/** 以太网接口运行状态 */
typedef struct {
  PlcEthConfig          config;
  bool                  initialized;
  bool                  running;
  PlcEthLink            link;
  DhcpState             dhcp_state;
  PlcEthRawRxCallback   raw_rx_callback;
  /* 统计信息 */
  uint32_t              rx_frames;
  uint32_t              tx_frames;
  uint32_t              rx_bytes;
  uint32_t              tx_bytes;
  uint32_t              rx_errors;
  uint32_t              tx_errors;
  uint32_t              rx_dropped;
  uint32_t              collisions;
} EthInstance;

/* ========== 静态变量 ========== */

static EthInstance s_eth[PLC_ETH_MAX_INSTANCES];

/* ========== 模拟底层驱动 ========== */

static int eth_hw_start(uint8_t eth_id) {
  (void)eth_id;
  return 0;
}

static int eth_hw_stop(uint8_t eth_id) {
  (void)eth_id;
  return 0;
}

static int eth_hw_send(uint8_t eth_id, const PlcEthFrame* frame) {
  (void)eth_id;
  (void)frame;
  return 0;
}

static int eth_hw_recv(uint8_t eth_id, PlcEthFrame* frame) {
  (void)eth_id;
  (void)frame;
  return -1; /* 无数据 */
}

/* ========== 公共接口实现 ========== */

int plc_eth_init(uint8_t eth_id, const PlcEthConfig* config) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || config == NULL) {
    return -1;
  }
  memset(&s_eth[eth_id], 0, sizeof(EthInstance));
  s_eth[eth_id].config = *config;
  s_eth[eth_id].initialized = true;
  s_eth[eth_id].link = PLC_ETH_LINK_DOWN;
  /* 如果启用 DHCP 则进入 DISCOVER 状态 */
  s_eth[eth_id].dhcp_state = config->dhcp ? DHCP_DISCOVER : DHCP_BOUND;
  return 0;
}

int plc_eth_start(uint8_t eth_id) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES) {
    return -1;
  }
  int ret = eth_hw_start(eth_id);
  if (ret == 0) {
    s_eth[eth_id].running = true;
    s_eth[eth_id].link = PLC_ETH_LINK_UP;
    /* 如果使用 DHCP，模拟进入绑定状态 */
    if (s_eth[eth_id].dhcp_state == DHCP_DISCOVER) {
      s_eth[eth_id].dhcp_state = DHCP_BOUND;
    }
  }
  return ret;
}

int plc_eth_stop(uint8_t eth_id) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES) {
    return -1;
  }
  int ret = eth_hw_stop(eth_id);
  if (ret == 0) {
    s_eth[eth_id].running = false;
    s_eth[eth_id].link = PLC_ETH_LINK_DOWN;
  }
  return ret;
}

int plc_eth_send_raw(uint8_t eth_id, const PlcEthFrame* frame) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || frame == NULL) {
    return -1;
  }
  if (!s_eth[eth_id].running) {
    return -2;
  }

  int ret = eth_hw_send(eth_id, frame);
  if (ret == 0) {
    s_eth[eth_id].tx_frames++;
    s_eth[eth_id].tx_bytes += frame->length;
  } else {
    s_eth[eth_id].tx_errors++;
  }
  return ret;
}

int plc_eth_recv_raw(uint8_t eth_id, PlcEthFrame* frame, uint32_t timeout_ms) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || frame == NULL) {
    return -1;
  }
  if (!s_eth[eth_id].running) {
    return -2;
  }
  (void)timeout_ms;

  int ret = eth_hw_recv(eth_id, frame);
  if (ret == 0) {
    s_eth[eth_id].rx_frames++;
    s_eth[eth_id].rx_bytes += frame->length;
    return 1;
  }
  return 0; /* 无数据 */
}

int plc_eth_set_raw_rx_callback(uint8_t eth_id, PlcEthRawRxCallback callback) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES) {
    return -1;
  }
  s_eth[eth_id].raw_rx_callback = callback;
  return 0;
}

PlcEthLink plc_eth_get_link(uint8_t eth_id) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES) {
    return PLC_ETH_LINK_DOWN;
  }
  return s_eth[eth_id].link;
}

int plc_eth_get_mac(uint8_t eth_id, uint8_t mac[PLC_ETH_MAC_LEN]) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || mac == NULL) {
    return -1;
  }
  memcpy(mac, s_eth[eth_id].config.mac, PLC_ETH_MAC_LEN);
  return 0;
}

int plc_eth_get_ip(uint8_t eth_id, uint8_t ip[PLC_ETH_IP_LEN]) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || ip == NULL) {
    return -1;
  }
  memcpy(ip, s_eth[eth_id].config.ip, PLC_ETH_IP_LEN);
  return 0;
}

int plc_eth_set_ip(uint8_t eth_id, const uint8_t ip[PLC_ETH_IP_LEN],
                    const uint8_t subnet[PLC_ETH_IP_LEN],
                    const uint8_t gateway[PLC_ETH_IP_LEN]) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || ip == NULL ||
      subnet == NULL || gateway == NULL) {
    return -1;
  }
  memcpy(s_eth[eth_id].config.ip, ip, PLC_ETH_IP_LEN);
  memcpy(s_eth[eth_id].config.subnet, subnet, PLC_ETH_IP_LEN);
  memcpy(s_eth[eth_id].config.gateway, gateway, PLC_ETH_IP_LEN);
  return 0;
}

int plc_eth_get_stats(uint8_t eth_id, PlcEthStats* stats) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES || stats == NULL) {
    return -1;
  }
  EthInstance* ei = &s_eth[eth_id];
  stats->rx_frames = ei->rx_frames;
  stats->tx_frames = ei->tx_frames;
  stats->rx_bytes = ei->rx_bytes;
  stats->tx_bytes = ei->tx_bytes;
  stats->rx_errors = ei->rx_errors;
  stats->tx_errors = ei->tx_errors;
  stats->rx_dropped = ei->rx_dropped;
  stats->collisions = ei->collisions;
  stats->link = ei->link;
  return 0;
}

int plc_eth_clear_stats(uint8_t eth_id) {
  if (eth_id >= PLC_ETH_MAX_INSTANCES) {
    return -1;
  }
  EthInstance* ei = &s_eth[eth_id];
  ei->rx_frames = 0;
  ei->tx_frames = 0;
  ei->rx_bytes = 0;
  ei->tx_bytes = 0;
  ei->rx_errors = 0;
  ei->tx_errors = 0;
  ei->rx_dropped = 0;
  ei->collisions = 0;
  return 0;
}
