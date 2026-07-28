/**
 * plc_canopen.c - CANopen DS301 协议实现
 *
 * 实现 CANopen 通信协议栈：
 * - NMT 网络管理状态机
 * - SDO 快速传输（≤4字节）
 * - PDO 发送/接收
 * - 心跳协议
 * - 简单对象字典
 */

#include "plc_canopen.h"
#include "plc_platform.h"
#include "plc_can.h"
#include <string.h>

/* ========== 内部常量 ========== */

#define CAN_ID_SELF  0  /* 使用 CAN 接口 0 */

/* ========== 内部状态 ========== */

static plc_bool s_initialized = false;
static plc_bool s_running = false;

/** 本节点信息 */
static PlcCanopenNode s_node;

/** PDO 配置表 */
static PlcCanopenPdo s_pdos[PLC_CANOPEN_MAX_PDO];

/** PDO 接收回调 */
static PlcCanopenPdoCallback s_pdo_callbacks[PLC_CANOPEN_MAX_PDO];

/** 简单对象字典 */
static PlcCanOdEntry s_od[PLC_CANOPEN_MAX_OD_ENTRIES];
static uint16_t s_od_count = 0;

/** 心跳定时器 */
static uint32_t s_heartbeat_last_ms = 0;

/** SDO 请求/响应缓冲区 */
static uint8_t s_sdo_tx_data[8];
static uint8_t s_sdo_rx_data[8];
static plc_bool s_sdo_response_ready = false;

/** 从站节点表（主站维护） */
static PlcCanopenNode s_slave_nodes[128];

/* ========== 内部辅助函数 ========== */

/** 在对象字典中查找条目 */
static PlcCanOdEntry* od_find(uint16_t index, uint8_t subindex)
{
  uint16_t i;
  for (i = 0; i < s_od_count; i++) {
    if (s_od[i].index == index && s_od[i].subindex == subindex) {
      return &s_od[i];
    }
  }
  return NULL;
}

/** 添加对象字典条目 */
static int od_add(uint16_t index, uint8_t subindex,
                   const uint8_t* data, uint8_t len)
{
  PlcCanOdEntry* entry;

  if (s_od_count >= PLC_CANOPEN_MAX_OD_ENTRIES) return -1;

  /* 检查是否已存在 */
  entry = od_find(index, subindex);
  if (entry) {
    memcpy(entry->data, data, len < 4 ? len : 4);
    entry->len = len;
    return 0;
  }

  /* 新增条目 */
  entry = &s_od[s_od_count++];
  entry->index = index;
  entry->subindex = subindex;
  memcpy(entry->data, data, len < 4 ? len : 4);
  entry->len = len;
  return 0;
}

/** 发送 CAN 报文 */
static int can_send(uint32_t cob_id, const uint8_t* data, uint8_t len)
{
  PlcCanMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.id = cob_id;
  msg.frame = PLC_CAN_STD;
  msg.dlc = len > 8 ? 8 : len;
  if (data && len > 0) {
    memcpy(msg.data, data, msg.dlc);
  }
  return plc_can_send(CAN_ID_SELF, &msg);
}

/** 接收 CAN 报文 */
static int can_recv(PlcCanMsg* msg, uint32_t timeout_ms)
{
  return plc_can_recv(CAN_ID_SELF, msg, timeout_ms);
}

/** 构建 SDO 响应（快速传输确认） */
static void build_sdo_response(uint8_t* buf, uint8_t server_cmd,
                                uint16_t index, uint8_t subindex,
                                const uint8_t* data, uint8_t len)
{
  buf[0] = server_cmd;
  buf[1] = (uint8_t)(index & 0xFF);
  buf[2] = (uint8_t)(index >> 8);
  buf[3] = subindex;

  /* 快速传输：数据在字节 4-7，不足 4 字节用 0 填充 */
  memset(buf + 4, 0, 4);
  if (data && len > 0) {
    memcpy(buf + 4, data, len < 4 ? len : 4);
  }
}

/* ========== NMT 处理 ========== */

static void process_nmt(const PlcCanMsg* msg)
{
  uint8_t command = msg->data[0];
  uint8_t target = msg->data[1];

  /* 仅处理针对本节点或广播的命令 */
  if (target != 0 && target != s_node.node_id) return;

  switch (command) {
    case PLC_CANOPEN_NMT_START_REMOTE_NODE:
      s_node.state = PLC_CANOPEN_STATE_OP;
      plc_platform_log(PLC_LOG_INFO, "CANopen: NMT 进入 OPERATIONAL");
      break;
    case PLC_CANOPEN_NMT_STOP_REMOTE_NODE:
      s_node.state = PLC_CANOPEN_STATE_INIT;
      plc_platform_log(PLC_LOG_INFO, "CANopen: NMT 进入 INIT");
      break;
    case PLC_CANOPEN_NMT_ENTER_PRE_OP:
      s_node.state = PLC_CANOPEN_STATE_PRE_OP;
      plc_platform_log(PLC_LOG_INFO, "CANopen: NMT 进入 PRE-OPERATIONAL");
      break;
    case PLC_CANOPEN_NMT_RESET_NODE:
      plc_canopen_reset();
      break;
    case PLC_CANOPEN_NMT_RESET_COMM:
      s_node.state = PLC_CANOPEN_STATE_PRE_OP;
      plc_platform_log(PLC_LOG_INFO, "CANopen: NMT 通信复位 → PRE-OP");
      break;
    default:
      break;
  }
}

/** 处理 SDO 请求（从站端） */
static void process_sdo_request(const PlcCanMsg* msg)
{
  uint8_t client_cmd = msg->data[0];
  uint16_t index = (uint16_t)msg->data[1] | ((uint16_t)msg->data[2] << 8);
  uint8_t subindex = msg->data[3];
  PlcCanOdEntry* entry;
  uint8_t resp[8];
  uint8_t server_cmd;

  /* 只处理快速下载请求（客户端命令 = 0x2F） */
  if ((client_cmd & 0xE0) == 0x20) {
    /* 快速传输下载 */
    uint8_t data_len = 4 - ((client_cmd >> 2) & 0x03);

    /* 写入对象字典 */
    if (od_add(index, subindex, msg->data + 4, data_len) < 0) {
      server_cmd = 0x60; /* 中止 */
    } else {
      server_cmd = 0x60; /* 传输确认 */
    }

    build_sdo_response(resp, server_cmd, index, subindex, NULL, 0);
    can_send(PLC_CANOPEN_COB_SDO_TX_BASE + s_node.node_id, resp, 8);
  }
  /* 快速上传请求（客户端命令 = 0x40） */
  else if ((client_cmd & 0xE0) == 0x40) {
    entry = od_find(index, subindex);
    if (entry) {
      server_cmd = 0x43 | (uint8_t)((4 - entry->len) << 2);
      build_sdo_response(resp, server_cmd, index, subindex, entry->data, entry->len);
    } else {
      /* 对象不存在 → 中止传输 */
      server_cmd = 0x80;
      memset(resp, 0, 8);
      resp[0] = server_cmd;
      resp[1] = (uint8_t)(index & 0xFF);
      resp[2] = (uint8_t)(index >> 8);
      resp[3] = subindex;
      /* 错误码: 0x06020000 = 对象不存在 */
      resp[4] = 0x00;
      resp[5] = 0x00;
      resp[6] = 0x02;
      resp[7] = 0x06;
    }
    can_send(PLC_CANOPEN_COB_SDO_TX_BASE + s_node.node_id, resp, 8);
  }
}

/** 处理心跳报文 */
static void process_heartbeat(const PlcCanMsg* msg)
{
  uint8_t node_id = (uint8_t)(msg->id & 0x7F);
  uint8_t state = msg->data[0];

  if (node_id < 128) {
    s_slave_nodes[node_id].node_id = node_id;
    s_slave_nodes[node_id].state = (PlcCanopenState)state;
  }
}

/** 处理接收的 CAN 报文 */
static void can_rx_handler(uint8_t can_id, const PlcCanMsg* msg)
{
  (void)can_id;

  /* NMT 报文 */
  if (msg->id == PLC_CANOPEN_COB_NMT) {
    process_nmt(msg);
    return;
  }

  /* SDO 接收（发给本节点的 SDO 请求） */
  if (msg->id == (PLC_CANOPEN_COB_SDO_RX_BASE + s_node.node_id)) {
    process_sdo_request(msg);
    return;
  }

  /* SDO 响应（来自主站/服务器的响应） */
  if (msg->id == (PLC_CANOPEN_COB_SDO_TX_BASE + s_node.node_id)) {
    memcpy(s_sdo_rx_data, msg->data, 8);
    s_sdo_response_ready = true;
    return;
  }

  /* PDO 接收 */
  {
    uint8_t i;
    for (i = 0; i < PLC_CANOPEN_MAX_PDO; i++) {
      if (s_pdos[i].cob_id == msg->id && s_pdo_callbacks[i]) {
        s_pdo_callbacks[i](i, msg->data, msg->dlc);
        return;
      }
    }
  }

  /* 心跳报文 */
  if ((msg->id & 0x700) == 0x700 && msg->id != 0) {
    process_heartbeat(msg);
  }
}

/* ========== 公开接口实现 ========== */

int plc_canopen_init(uint8_t node_id)
{
  PlcCanConfig can_cfg;

  if (node_id == 0 || node_id > 127) return -1;

  memset(&s_node, 0, sizeof(s_node));
  memset(s_pdos, 0, sizeof(s_pdos));
  memset(s_pdo_callbacks, 0, sizeof(s_pdo_callbacks));
  memset(s_od, 0, sizeof(s_od));
  memset(s_slave_nodes, 0, sizeof(s_slave_nodes));
  s_od_count = 0;
  s_sdo_response_ready = false;
  s_heartbeat_last_ms = 0;

  /* 配置节点基本信息 */
  s_node.node_id = node_id;
  s_node.state = PLC_CANOPEN_STATE_INIT;
  s_node.heartbeat_producer_ms = 1000; /* 默认 1 秒心跳 */
  s_node.error_code = 0;

  /* 初始化 CAN 接口 */
  memset(&can_cfg, 0, sizeof(can_cfg));
  can_cfg.baud = PLC_CAN_BAUD_500K;
  can_cfg.mode = PLC_CAN_MODE_NORMAL;
  can_cfg.filter_count = 0;

  if (plc_can_init(CAN_ID_SELF, &can_cfg) < 0) {
    return -2;
  }

  /* 注册接收回调 */
  plc_can_set_rx_callback(CAN_ID_SELF, can_rx_handler);

  /* 写入默认对象字典条目（设备信息） */
  {
    uint8_t val8;
    uint16_t val16;
    uint32_t val32;

    val8 = node_id;
    od_add(0x1018, 0x01, &val8, 1); /* 节点 ID */

    val32 = 0x12345678;
    od_add(0x1018, 0x02, (const uint8_t*)&val32, 4); /* 厂商 ID */

    val16 = 0x0100;
    od_add(0x1018, 0x03, (const uint8_t*)&val16, 2); /* 产品代码 */
  }

  s_initialized = true;
  plc_platform_log(PLC_LOG_INFO, "CANopen 初始化完成 (节点 ID=%d)", node_id);
  return 0;
}

int plc_canopen_start(void)
{
  if (!s_initialized) return -1;

  plc_can_start(CAN_ID_SELF);
  s_node.state = PLC_CANOPEN_STATE_OP;
  s_running = true;
  s_heartbeat_last_ms = plc_platform_tick_ms();

  plc_platform_log(PLC_LOG_INFO, "CANopen 启动 (OP 状态)");
  return 0;
}

int plc_canopen_stop(void)
{
  if (!s_initialized) return -1;

  s_running = false;
  s_node.state = PLC_CANOPEN_STATE_INIT;

  plc_platform_log(PLC_LOG_INFO, "CANopen 停止");
  return 0;
}

int plc_canopen_reset(void)
{
  if (!s_initialized) return -1;

  s_running = false;
  s_node.state = PLC_CANOPEN_STATE_INIT;
  s_node.error_code = 0;
  s_od_count = 0;
  s_sdo_response_ready = false;

  plc_platform_log(PLC_LOG_INFO, "CANopen 复位");
  return 0;
}

int plc_canopen_nmt(uint8_t command, uint8_t target_node)
{
  uint8_t data[2];

  if (!s_initialized) return -1;

  data[0] = command;
  data[1] = target_node;

  return can_send(PLC_CANOPEN_COB_NMT, data, 2);
}

int plc_canopen_sdo_write(uint8_t node_id, uint16_t index, uint8_t subindex,
                           const uint8_t* data, uint8_t len)
{
  uint8_t cmd;
  uint8_t req[8];

  if (!s_initialized || !s_running) return -1;
  if (len > 4) return -2;

  /* 快速传输下载命令: 0x2F（4字节）/ 0x27（3字节）/ 0x23（2字节）/ 0x2F（1字节） */
  switch (len) {
    case 1: cmd = 0x2F; break;
    case 2: cmd = 0x2B; break;
    case 3: cmd = 0x27; break;
    case 4: cmd = 0x23; break;
    default: cmd = 0x23; break;
  }

  /* 设置大小位 (e=1 表示加速传输, s=1 表示大小有效) */
  cmd = 0x23 | (uint8_t)((4 - len) << 2) | 0x01;

  req[0] = cmd;
  req[1] = (uint8_t)(index & 0xFF);
  req[2] = (uint8_t)(index >> 8);
  req[3] = subindex;
  memset(req + 4, 0, 4);
  if (data && len > 0) {
    memcpy(req + 4, data, len);
  }

  /* 发送 SDO 请求 */
  s_sdo_response_ready = false;
  if (can_send(PLC_CANOPEN_COB_SDO_RX_BASE + node_id, req, 8) < 0) {
    return -3;
  }

  /* 等待响应（简单轮询） */
  {
    uint32_t start = plc_platform_tick_ms();
    PlcCanMsg msg;
    while (!s_sdo_response_ready) {
      if (plc_platform_tick_ms() - start > 100) {
        return -4; /* 超时 */
      }
      if (can_recv(&msg, 10) > 0) {
        can_rx_handler(0, &msg);
      }
    }
  }

  /* 检查服务器响应码 */
  if ((s_sdo_rx_data[0] & 0xE0) == 0x60) {
    return 0; /* 成功 */
  }
  return -5; /* 传输中止 */
}

int plc_canopen_sdo_read(uint8_t node_id, uint16_t index, uint8_t subindex,
                          uint8_t* data, uint8_t max_len)
{
  uint8_t req[8];

  if (!s_initialized || !s_running) return -1;
  if (!data) return -2;

  /* 快速上传请求 */
  req[0] = 0x40; /* 客户端命令: 上传请求 */
  req[1] = (uint8_t)(index & 0xFF);
  req[2] = (uint8_t)(index >> 8);
  req[3] = subindex;
  memset(req + 4, 0, 4);

  s_sdo_response_ready = false;
  if (can_send(PLC_CANOPEN_COB_SDO_RX_BASE + node_id, req, 8) < 0) {
    return -3;
  }

  /* 等待响应 */
  {
    uint32_t start = plc_platform_tick_ms();
    PlcCanMsg msg;
    while (!s_sdo_response_ready) {
      if (plc_platform_tick_ms() - start > 100) {
        return -4;
      }
      if (can_recv(&msg, 10) > 0) {
        can_rx_handler(0, &msg);
      }
    }
  }

  /* 检查响应码 */
  {
    uint8_t resp_cmd = s_sdo_rx_data[0];
    if ((resp_cmd & 0xE0) == 0x40) {
      /* 有效上传响应 */
      uint8_t data_len = 4 - ((resp_cmd >> 2) & 0x03);
      uint8_t copy_len = data_len < max_len ? data_len : max_len;
      memcpy(data, s_sdo_rx_data + 4, copy_len);
      return (int)copy_len;
    }
  }

  return -5; /* 传输中止 */
}

int plc_canopen_pdo_send(uint8_t pdo_index, const uint8_t* data, uint8_t len)
{
  if (!s_initialized) return -1;
  if (pdo_index >= PLC_CANOPEN_MAX_PDO) return -2;

  if (s_pdos[pdo_index].cob_id == 0) return -3; /* PDO 未配置 */

  return can_send(s_pdos[pdo_index].cob_id, data, len);
}

int plc_canopen_pdo_register_callback(uint8_t pdo_index,
                                       PlcCanopenPdoCallback callback)
{
  if (pdo_index >= PLC_CANOPEN_MAX_PDO) return -1;
  s_pdo_callbacks[pdo_index] = callback;
  return 0;
}

int plc_canopen_sync(void)
{
  if (!s_initialized) return -1;
  return can_send(PLC_CANOPEN_COB_SYNC, NULL, 0);
}

int plc_canopen_heartbeat(PlcCanopenState state)
{
  uint8_t data[1];

  if (!s_initialized) return -1;

  data[0] = (uint8_t)state;
  return can_send(PLC_CANOPEN_COB_HEARTBEAT + s_node.node_id, data, 1);
}

int plc_canopen_process(void)
{
  PlcCanMsg msg;
  uint32_t now;
  uint8_t i;

  if (!s_initialized) return -1;

  /* 处理所有待接收的 CAN 报文 */
  while (can_recv(&msg, 0) > 0) {
    can_rx_handler(CAN_ID_SELF, &msg);
  }

  if (!s_running) return 0;

  now = plc_platform_tick_ms();

  /* 心跳发送 */
  if (s_node.heartbeat_producer_ms > 0 &&
      (now - s_heartbeat_last_ms) >= s_node.heartbeat_producer_ms) {
    plc_canopen_heartbeat(s_node.state);
    s_heartbeat_last_ms = now;
  }

  /* PDO 周期发送 */
  for (i = 0; i < PLC_CANOPEN_MAX_PDO; i++) {
    if (s_pdos[i].event_timer_ms > 0 && s_pdos[i].cob_id != 0) {
      /* 收集映射变量数据到发送缓冲区 */
      uint8_t pdo_data[8] = {0};
      uint8_t offset = 0;
      uint8_t j;

      for (j = 0; j < s_pdos[i].mapped_count && offset < 8; j++) {
        uint8_t size = s_pdos[i].mapped_sizes[j];
        if (s_pdos[i].mapped_vars[j] && (offset + size) <= 8) {
          memcpy(pdo_data + offset, s_pdos[i].mapped_vars[j], size);
          offset += size;
        }
      }

      can_send(s_pdos[i].cob_id, pdo_data, offset);
    }
  }

  return 0;
}
