/**
 * plc_eth.h - 以太网硬件抽象层接口
 *
 * 提供以太网初始化、收发帧等统一接口
 * 支持原始以太网帧和 IP/TCP/UDP 协议栈
 */

#ifndef PLC_ETH_H
#define PLC_ETH_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_ETH_MAC_LEN
  #define PLC_ETH_MAC_LEN         6
#endif

#ifndef PLC_ETH_IP_LEN
  #define PLC_ETH_IP_LEN          4
#endif

#ifndef PLC_ETH_MAX_SOCKETS
  #define PLC_ETH_MAX_SOCKETS     8
#endif

#ifndef PLC_ETH_MTU
  #define PLC_ETH_MTU             1500
#endif

/* ========== 枚举定义 ========== */

/** 以太网链路状态 */
typedef enum {
  PLC_ETH_LINK_DOWN  = 0,
  PLC_ETH_LINK_UP    = 1,
  PLC_ETH_LINK_HALF  = 2,
  PLC_ETH_LINK_FULL  = 3,
} PlcEthLink;

/** 套接字类型 */
typedef enum {
  PLC_ETH_SOCK_TCP   = 0,
  PLC_ETH_SOCK_UDP   = 1,
  PLC_ETH_SOCK_RAW  = 2,
} PlcEthSockType;

/** 套接字状态 */
typedef enum {
  PLC_ETH_SOCK_FREE     = 0,
  PLC_ETH_SOCK_BOUND    = 1,
  PLC_ETH_SOCK_LISTEN   = 2,
  PLC_ETH_SOCK_CONNECT  = 3,
  PLC_ETH_SOCK_ESTABLISHED = 4,
  PLC_ETH_SOCK_CLOSED   = 5,
} PlcEthSockState;

/* ========== 结构体定义 ========== */

/** 以太网配置 */
typedef struct {
  uint8_t      mac[PLC_ETH_MAC_LEN];   /* MAC 地址 */
  uint8_t      ip[PLC_ETH_IP_LEN];     /* IP 地址 */
  uint8_t      subnet[PLC_ETH_IP_LEN]; /* 子网掩码 */
  uint8_t      gateway[PLC_ETH_IP_LEN];/* 网关 */
  uint8_t      dns[PLC_ETH_IP_LEN];    /* DNS 服务器 */
  bool         dhcp;                    /* 是否使用 DHCP */
  uint16_t     mtu;                     /* MTU 大小 */
} PlcEthConfig;

/** 以太网帧 */
typedef struct {
  uint8_t      dst_mac[PLC_ETH_MAC_LEN];
  uint8_t      src_mac[PLC_ETH_MAC_LEN];
  uint16_t     ether_type;
  uint8_t      payload[PLC_ETH_MTU];
  uint16_t     length;
} PlcEthFrame;

/** 以太网统计信息 */
typedef struct {
  uint32_t     rx_frames;
  uint32_t     tx_frames;
  uint32_t     rx_bytes;
  uint32_t     tx_bytes;
  uint32_t     rx_errors;
  uint32_t     tx_errors;
  uint32_t     rx_dropped;
  uint32_t     collisions;
  PlcEthLink   link;
} PlcEthStats;

/** 以太网原始帧接收回调 */
typedef void (*PlcEthRawRxCallback)(uint8_t eth_id, const PlcEthFrame* frame);

/* ========== 函数声明 ========== */

/**
 * 初始化以太网接口
 * @param eth_id 以太网接口编号
 * @param config 配置参数
 * @return 0 成功, 负值错误码
 */
int plc_eth_init(uint8_t eth_id, const PlcEthConfig* config);

/**
 * 启动以太网接口
 */
int plc_eth_start(uint8_t eth_id);

/**
 * 停止以太网接口
 */
int plc_eth_stop(uint8_t eth_id);

/**
 * 发送原始以太网帧
 */
int plc_eth_send_raw(uint8_t eth_id, const PlcEthFrame* frame);

/**
 * 接收原始以太网帧
 */
int plc_eth_recv_raw(uint8_t eth_id, PlcEthFrame* frame, uint32_t timeout_ms);

/**
 * 注册原始帧接收回调
 */
int plc_eth_set_raw_rx_callback(uint8_t eth_id, PlcEthRawRxCallback callback);

/**
 * 获取链路状态
 */
PlcEthLink plc_eth_get_link(uint8_t eth_id);

/**
 * 获取 MAC 地址
 */
int plc_eth_get_mac(uint8_t eth_id, uint8_t mac[PLC_ETH_MAC_LEN]);

/**
 * 获取 IP 地址
 */
int plc_eth_get_ip(uint8_t eth_id, uint8_t ip[PLC_ETH_IP_LEN]);

/**
 * 设置 IP 地址
 */
int plc_eth_set_ip(uint8_t eth_id, const uint8_t ip[PLC_ETH_IP_LEN],
                   const uint8_t subnet[PLC_ETH_IP_LEN],
                   const uint8_t gateway[PLC_ETH_IP_LEN]);

/**
 * 获取统计信息
 */
int plc_eth_get_stats(uint8_t eth_id, PlcEthStats* stats);

/**
 * 清除统计信息
 */
int plc_eth_clear_stats(uint8_t eth_id);

#ifdef __cplusplus
}
#endif

#endif /* PLC_ETH_H */
