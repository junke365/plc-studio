/**
 * plc_tcp.h - TCP 通信协议接口
 *
 * 提供 TCP 客户端/服务器通信能力
 * 支持多连接管理、心跳保活、接收回调
 */

#ifndef PLC_TCP_H
#define PLC_TCP_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_TCP_MAX_CONNECTIONS
  #ifdef PLATFORM_STM32
    #define PLC_TCP_MAX_CONNECTIONS    4
  #elif defined(PLATFORM_ESP32)
    #define PLC_TCP_MAX_CONNECTIONS    4
  #else
    #define PLC_TCP_MAX_CONNECTIONS    16
  #endif
#endif

#ifndef PLC_TCP_MAX_SEND_BUF
  #define PLC_TCP_MAX_SEND_BUF       1024
#endif

#ifndef PLC_TCP_MAX_RECV_BUF
  #define PLC_TCP_MAX_RECV_BUF       2048
#endif

#ifndef PLC_TCP_DEFAULT_KEEPALIVE
  #define PLC_TCP_DEFAULT_KEEPALIVE  30000
#endif

/* ========== 枚举定义 ========== */

/** TCP 角色 */
typedef enum {
  PLC_TCP_ROLE_CLIENT = 0,
  PLC_TCP_ROLE_SERVER = 1
} PlcTcpRole;

/** TCP 连接状态 */
typedef enum {
  PLC_TCP_CONN_DISCONNECTED = 0,
  PLC_TCP_CONN_CONNECTING   = 1,
  PLC_TCP_CONN_CONNECTED    = 2,
  PLC_TCP_CONN_ERROR        = 3
} PlcTcpConnState;

/* ========== 结构体定义 ========== */

/** TCP 配置参数 */
typedef struct {
  PlcTcpRole   role;                  /* 客户端/服务器 */
  uint16_t     local_port;            /* 本地端口 */
  char         remote_host[64];       /* 远程主机地址（客户端模式） */
  uint16_t     remote_port;           /* 远程端口（客户端模式） */
  uint8_t      max_connections;       /* 最大连接数（服务器模式） */
  uint32_t     keepalive_ms;          /* 保活超时（毫秒，0=禁用） */
} PlcTcpConfig;

/** TCP 连接信息 */
typedef struct {
  uint8_t        id;                  /* 连接 ID */
  int            fd;                  /* 套接字文件描述符 */
  PlcTcpConnState state;             /* 连接状态 */
  char           remote_addr[46];     /* 远程地址（IPv4/IPv6） */
  uint16_t       remote_port;         /* 远程端口 */
  uint32_t       last_activity_ms;    /* 最后活动时间戳 */
} PlcTcpConnection;

/** TCP 接收回调函数原型 */
typedef void (*PlcTcpRxCallback)(uint8_t conn_id, const uint8_t* data,
                                  uint32_t len);

/* ========== 函数声明 ========== */

/**
 * 初始化 TCP 协议栈
 * @param config 配置参数
 * @return 0 成功, 负值错误码
 */
int plc_tcp_init(const PlcTcpConfig* config);

/**
 * 启动 TCP 服务（服务器开始监听，客户端连接）
 * @return 0 成功, 负值错误码
 */
int plc_tcp_start(void);

/**
 * 停止 TCP 服务
 * @return 0 成功
 */
int plc_tcp_stop(void);

/**
 * 服务器端接受新连接
 * @return 新连接 ID, 负值错误码（无新连接时返回 -1）
 */
int plc_tcp_accept(void);

/**
 * 客户端发起连接
 * @param host 远程主机地址
 * @param port 远程端口
 * @return 连接 ID, 负值错误码
 */
int plc_tcp_connect(const char* host, uint16_t port);

/**
 * 发送数据
 * @param conn_id 连接 ID
 * @param data    数据指针
 * @param len     数据长度
 * @return 实际发送的字节数, 负值错误码
 */
int plc_tcp_send(uint8_t conn_id, const uint8_t* data, uint32_t len);

/**
 * 接收数据
 * @param conn_id   连接 ID
 * @param data      接收缓冲区
 * @param max_len   缓冲区最大长度
 * @param timeout_ms 超时（毫秒）
 * @return 实际接收的字节数, 0=超时, 负值错误码
 */
int plc_tcp_recv(uint8_t conn_id, uint8_t* data, uint32_t max_len,
                  uint32_t timeout_ms);

/**
 * 断开连接
 * @param conn_id 连接 ID
 * @return 0 成功, 负值错误码
 */
int plc_tcp_disconnect(uint8_t conn_id);

/**
 * 向所有已连接客户端广播数据（仅服务器模式）
 * @param data 数据指针
 * @param len  数据长度
 * @return 实际发送成功的连接数, 负值错误码
 */
int plc_tcp_send_all(const uint8_t* data, uint32_t len);

/**
 * 注册接收回调函数
 * @param callback 回调函数（NULL 取消注册）
 */
void plc_tcp_set_rx_callback(PlcTcpRxCallback callback);

/**
 * 获取当前活跃连接数
 * @return 活跃连接数
 */
uint8_t plc_tcp_get_connection_count(void);

#ifdef __cplusplus
}
#endif

#endif /* PLC_TCP_H */
