/**
 * plc_tcp.c - TCP 通信协议实现
 *
 * 实现 TCP 客户端/服务器通信
 * 包含连接管理、心跳保活、接收回调分发
 */

#include "plc_tcp.h"
#include "plc_platform.h"
#include <string.h>

/* ========== 平台适配 ========== */

/*
 * TCP 操作通过平台 HAL 抽象层实现
 * 具体的 socket 调用由各平台的 plc_platform 提供
 * 这里定义平台无关的 TCP 操作接口
 */

/** 平台层 TCP 操作（由平台实现提供） */
extern int plc_platform_tcp_create_socket(void);
extern int plc_platform_tcp_bind(int fd, uint16_t port);
extern int plc_platform_tcp_listen(int fd, int backlog);
extern int plc_platform_tcp_accept(int fd, char* remote_addr, uint16_t* remote_port);
extern int plc_platform_tcp_connect(int fd, const char* host, uint16_t port, uint32_t timeout_ms);
extern int plc_platform_tcp_send(int fd, const uint8_t* data, uint32_t len);
extern int plc_platform_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms);
extern int plc_platform_tcp_close(int fd);
extern int plc_platform_tcp_set_keepalive(int fd, uint32_t idle_ms);
extern int plc_platform_tcp_set_nonblocking(int fd, plc_bool enable);

/* ========== 内部状态 ========== */

static plc_bool s_initialized = false;
static plc_bool s_running = false;

/** 配置参数 */
static PlcTcpConfig s_config;

/** 连接表 */
static PlcTcpConnection s_connections[PLC_TCP_MAX_CONNECTIONS];
static uint8_t s_conn_count = 0;

/** 服务器监听 socket */
static int s_listen_fd = -1;

/** 接收回调 */
static PlcTcpRxCallback s_rx_callback = NULL;

/* ========== 内部辅助函数 ========== */

/** 查找空闲连接槽位 */
static int find_free_slot(void)
{
  uint8_t i;
  for (i = 0; i < PLC_TCP_MAX_CONNECTIONS; i++) {
    if (s_connections[i].state == PLC_TCP_CONN_DISCONNECTED) {
      return (int)i;
    }
  }
  return -1;
}

/** 通过 ID 查找连接 */
static PlcTcpConnection* find_connection(uint8_t conn_id)
{
  if (conn_id >= PLC_TCP_MAX_CONNECTIONS) return NULL;
  if (s_connections[conn_id].state == PLC_TCP_CONN_DISCONNECTED) return NULL;
  return &s_connections[conn_id];
}

/** 关闭连接并清理槽位 */
static void close_connection(uint8_t conn_id)
{
  PlcTcpConnection* conn = &s_connections[conn_id];
  if (conn->fd >= 0) {
    plc_platform_tcp_close(conn->fd);
    conn->fd = -1;
  }
  conn->state = PLC_TCP_CONN_DISCONNECTED;
  conn->remote_addr[0] = '\0';
  conn->remote_port = 0;
  s_conn_count--;
  plc_platform_log(PLC_LOG_DEBUG, "TCP: 连接 %d 已关闭", conn_id);
}

/* ========== 公开接口实现 ========== */

int plc_tcp_init(const PlcTcpConfig* config)
{
  if (!config) return -1;

  memcpy(&s_config, config, sizeof(PlcTcpConfig));

  /* 设置默认值 */
  if (s_config.max_connections == 0) {
    s_config.max_connections = 8;
  }
  if (s_config.keepalive_ms == 0) {
    s_config.keepalive_ms = PLC_TCP_DEFAULT_KEEPALIVE;
  }

  /* 初始化连接表 */
  memset(s_connections, 0, sizeof(s_connections));
  s_conn_count = 0;
  s_listen_fd = -1;
  s_rx_callback = NULL;

  s_initialized = true;
  plc_platform_log(PLC_LOG_INFO, "TCP 初始化完成 (角色=%s, 端口=%d)",
                   s_config.role == PLC_TCP_ROLE_SERVER ? "服务器" : "客户端",
                   s_config.local_port);
  return 0;
}

int plc_tcp_start(void)
{
  if (!s_initialized) return -1;

  if (s_config.role == PLC_TCP_ROLE_SERVER) {
    /* 服务器模式：创建、绑定、监听 */
    s_listen_fd = plc_platform_tcp_create_socket();
    if (s_listen_fd < 0) {
      plc_platform_log(PLC_LOG_ERROR, "TCP: 创建 socket 失败");
      return -2;
    }

    if (plc_platform_tcp_bind(s_listen_fd, s_config.local_port) < 0) {
      plc_platform_log(PLC_LOG_ERROR, "TCP: 绑定端口 %d 失败", s_config.local_port);
      plc_platform_tcp_close(s_listen_fd);
      s_listen_fd = -1;
      return -3;
    }

    if (plc_platform_tcp_listen(s_listen_fd, s_config.max_connections) < 0) {
      plc_platform_log(PLC_LOG_ERROR, "TCP: 监听失败");
      plc_platform_tcp_close(s_listen_fd);
      s_listen_fd = -1;
      return -4;
    }

    plc_platform_log(PLC_LOG_INFO, "TCP 服务器启动，监听端口 %d", s_config.local_port);
  } else {
    /* 客户端模式：立即连接 */
    int conn_id = plc_tcp_connect(s_config.remote_host, s_config.remote_port);
    if (conn_id < 0) {
      plc_platform_log(PLC_LOG_WARN, "TCP: 连接 %s:%d 失败，将在后台重试",
                       s_config.remote_host, s_config.remote_port);
    }
  }

  s_running = true;
  return 0;
}

int plc_tcp_stop(void)
{
  uint8_t i;

  if (!s_initialized) return -1;

  s_running = false;

  /* 关闭所有连接 */
  for (i = 0; i < PLC_TCP_MAX_CONNECTIONS; i++) {
    if (s_connections[i].state != PLC_TCP_CONN_DISCONNECTED) {
      close_connection(i);
    }
  }

  /* 关闭监听 socket */
  if (s_listen_fd >= 0) {
    plc_platform_tcp_close(s_listen_fd);
    s_listen_fd = -1;
  }

  plc_platform_log(PLC_LOG_INFO, "TCP 停止");
  return 0;
}

int plc_tcp_accept(void)
{
  int slot;
  PlcTcpConnection* conn;
  char remote_addr[46];
  uint16_t remote_port = 0;
  int new_fd;

  if (!s_initialized || !s_running) return -1;
  if (s_config.role != PLC_TCP_ROLE_SERVER) return -2;
  if (s_listen_fd < 0) return -3;

  /* 查找空闲槽位 */
  slot = find_free_slot();
  if (slot < 0) {
    plc_platform_log(PLC_LOG_WARN, "TCP: 连接表已满，拒绝新连接");
    return -4;
  }

  /* 尝试接受连接（非阻塞） */
  plc_platform_tcp_set_nonblocking(s_listen_fd, true);
  new_fd = plc_platform_tcp_accept(s_listen_fd, remote_addr, &remote_port);
  plc_platform_tcp_set_nonblocking(s_listen_fd, false);

  if (new_fd < 0) {
    return -5; /* 无新连接 */
  }

  /* 初始化连接信息 */
  conn = &s_connections[slot];
  conn->id = (uint8_t)slot;
  conn->fd = new_fd;
  conn->state = PLC_TCP_CONN_CONNECTED;
  strncpy(conn->remote_addr, remote_addr, sizeof(conn->remote_addr) - 1);
  conn->remote_addr[sizeof(conn->remote_addr) - 1] = '\0';
  conn->remote_port = remote_port;
  conn->last_activity_ms = plc_platform_tick_ms();

  /* 设置 keepalive */
  if (s_config.keepalive_ms > 0) {
    plc_platform_tcp_set_keepalive(new_fd, s_config.keepalive_ms);
  }

  s_conn_count++;

  plc_platform_log(PLC_LOG_INFO, "TCP: 接受连接 #%d [%s:%d]",
                   slot, remote_addr, remote_port);

  return slot;
}

int plc_tcp_connect(const char* host, uint16_t port)
{
  int slot;
  PlcTcpConnection* conn;
  int fd;

  if (!s_initialized) return -1;
  if (!host) return -2;

  slot = find_free_slot();
  if (slot < 0) return -3;

  fd = plc_platform_tcp_create_socket();
  if (fd < 0) return -4;

  /* 带超时的连接 */
  if (plc_platform_tcp_connect(fd, host, port, s_config.timeout_ms) < 0) {
    plc_platform_tcp_close(fd);
    return -5;
  }

  /* 初始化连接信息 */
  conn = &s_connections[slot];
  conn->id = (uint8_t)slot;
  conn->fd = fd;
  conn->state = PLC_TCP_CONN_CONNECTED;
  strncpy(conn->remote_addr, host, sizeof(conn->remote_addr) - 1);
  conn->remote_addr[sizeof(conn->remote_addr) - 1] = '\0';
  conn->remote_port = port;
  conn->last_activity_ms = plc_platform_tick_ms();

  if (s_config.keepalive_ms > 0) {
    plc_platform_tcp_set_keepalive(fd, s_config.keepalive_ms);
  }

  s_conn_count++;

  plc_platform_log(PLC_LOG_INFO, "TCP: 已连接到 %s:%d (连接 #%d)",
                   host, port, slot);

  return slot;
}

int plc_tcp_send(uint8_t conn_id, const uint8_t* data, uint32_t len)
{
  PlcTcpConnection* conn;

  if (!data || len == 0) return -1;

  conn = find_connection(conn_id);
  if (!conn) return -2;
  if (conn->state != PLC_TCP_CONN_CONNECTED) return -3;

  int sent = plc_platform_tcp_send(conn->fd, data, len);
  if (sent < 0) {
    conn->state = PLC_TCP_CONN_ERROR;
    close_connection(conn_id);
    return -4;
  }

  conn->last_activity_ms = plc_platform_tick_ms();
  return sent;
}

int plc_tcp_recv(uint8_t conn_id, uint8_t* data, uint32_t max_len,
                  uint32_t timeout_ms)
{
  PlcTcpConnection* conn;
  int received;

  if (!data || max_len == 0) return -1;

  conn = find_connection(conn_id);
  if (!conn) return -2;
  if (conn->state != PLC_TCP_CONN_CONNECTED) return -3;

  received = plc_platform_tcp_recv(conn->fd, data, max_len, timeout_ms);
  if (received < 0) {
    conn->state = PLC_TCP_CONN_ERROR;
    close_connection(conn_id);
    return -4;
  }

  if (received == 0) {
    return 0; /* 超时无数据 */
  }

  conn->last_activity_ms = plc_platform_tick_ms();
  return received;
}

int plc_tcp_disconnect(uint8_t conn_id)
{
  PlcTcpConnection* conn = find_connection(conn_id);
  if (!conn) return -1;

  plc_platform_log(PLC_LOG_INFO, "TCP: 主动断开连接 #%d", conn_id);
  close_connection(conn_id);
  return 0;
}

int plc_tcp_send_all(const uint8_t* data, uint32_t len)
{
  uint8_t i;
  int sent_count = 0;

  if (!data || len == 0) return -1;

  for (i = 0; i < PLC_TCP_MAX_CONNECTIONS; i++) {
    if (s_connections[i].state == PLC_TCP_CONN_CONNECTED) {
      if (plc_platform_tcp_send(s_connections[i].fd, data, len) > 0) {
        sent_count++;
        s_connections[i].last_activity_ms = plc_platform_tick_ms();
      }
    }
  }

  return sent_count;
}

void plc_tcp_set_rx_callback(PlcTcpRxCallback callback)
{
  s_rx_callback = callback;
}

uint8_t plc_tcp_get_connection_count(void)
{
  return s_conn_count;
}

/* ========== 轮询处理（在主循环中调用） ========== */

void plc_tcp_poll(void)
{
  uint8_t i;
  uint8_t recv_buf[PLC_TCP_MAX_RECV_BUF];
  int received;

  if (!s_initialized || !s_running) return;

  /* 服务器模式：尝试接受新连接 */
  if (s_config.role == PLC_TCP_ROLE_SERVER && s_listen_fd >= 0) {
    plc_tcp_accept();
  }

  /* 遍历所有活跃连接，检查数据和超时 */
  for (i = 0; i < PLC_TCP_MAX_CONNECTIONS; i++) {
    PlcTcpConnection* conn = &s_connections[i];

    if (conn->state == PLC_TCP_CONN_DISCONNECTED) continue;

    /* 检查保活超时 */
    if (s_config.keepalive_ms > 0 &&
        conn->state == PLC_TCP_CONN_CONNECTED) {
      uint32_t elapsed = plc_platform_tick_ms() - conn->last_activity_ms;
      if (elapsed > s_config.keepalive_ms) {
        plc_platform_log(PLC_LOG_WARN, "TCP: 连接 #%d 保活超时", i);
        close_connection(i);
        continue;
      }
    }

    /* 尝试接收数据 */
    if (conn->state == PLC_TCP_CONN_CONNECTED) {
      received = plc_platform_tcp_recv(conn->fd, recv_buf,
                                        sizeof(recv_buf), 0);
      if (received > 0) {
        conn->last_activity_ms = plc_platform_tick_ms();

        /* 分发到回调函数 */
        if (s_rx_callback) {
          s_rx_callback(i, recv_buf, (uint32_t)received);
        }
      } else if (received < 0) {
        plc_platform_log(PLC_LOG_WARN, "TCP: 连接 #%d 接收错误", i);
        close_connection(i);
      }
    }
  }
}
