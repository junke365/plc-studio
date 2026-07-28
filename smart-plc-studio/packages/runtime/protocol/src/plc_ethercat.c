/**
 * plc_ethercat.c - EtherCAT 协议实现（简化版）
 *
 * 基于原始以太网帧实现简化版 EtherCAT 协议栈
 * 包含 AL 状态机、SDO 参数配置、PDO 过程数据交换
 */

#include "plc_ethercat.h"
#include "plc_platform.h"
#include "plc_eth.h"
#include <string.h>

/* ========== 内部常量 ========== */

#define ETH_ID_SELF  0  /* 使用以太网接口 0 */

/** EtherCAT 帧头结构 */
typedef struct {
  uint16_t  ether_type;     /* 必须为 0x88A4 */
  uint8_t   cmd;            /* 命令码 */
  uint8_t   idx;            /* 索引号 */
  uint16_t  addr;           /* 地址 */
  uint16_t  len;            /* 数据长度 */
  uint16_t  irq;            /* 中断标志 */
} __attribute__((packed)) EthercatHeader;

/* ========== 内部状态 ========== */

static plc_bool s_initialized = false;

/** 主站配置 */
static PlcEthercatConfig s_config;

/** 主站 AL 状态 */
static PlcEthercatAlState s_al_state = PLC_ETHERCAT_AL_INIT;

/** 工作计数器 */
static uint16_t s_wkc = 0;

/** 从站表 */
static PlcEthercatSlave s_slaves[PLC_ETHERCAT_MAX_SLAVES];
static uint8_t s_slave_count = 0;

/** PDO 输入映射（从站→主站） */
static PlcEthercatPdoEntry s_pdo_inputs[PLC_ETHERCAT_MAX_PDO_ENTRIES];
static uint8_t s_pdo_input_count = 0;

/** PDO 输出映射（主站→从站） */
static PlcEthercatPdoEntry s_pdo_outputs[PLC_ETHERCAT_MAX_PDO_ENTRIES];
static uint8_t s_pdo_output_count = 0;

/** CoE 对象字典 */
static PlcEthercatOdEntry s_od[PLC_ETHERCAT_MAX_OD_ENTRIES];
static uint16_t s_od_count = 0;

/** 过程数据缓冲区 */
static uint8_t s_pdo_in_buf[2048];
static uint8_t s_pdo_out_buf[2048];
static uint16_t s_pdo_in_size = 0;
static uint16_t s_pdo_out_size = 0;

/** 帧计数器（用于传输 ID） */
static uint8_t s_frame_idx = 0;

/* ========== 内部辅助函数 ========== */

/** 查找 CoE 对象字典条目 */
static PlcEthercatOdEntry* od_find(uint16_t index, uint8_t subindex)
{
  uint16_t i;
  for (i = 0; i < s_od_count; i++) {
    if (s_od[i].index == index && s_od[i].subindex == subindex) {
      return &s_od[i];
    }
  }
  return NULL;
}

/** 发送原始以太网帧 */
static int send_raw_frame(const uint8_t* data, uint16_t len)
{
  PlcEthFrame frame;
  memset(&frame, 0, sizeof(frame));

  /* 广播地址 */
  memset(frame.dst_mac, 0xFF, PLC_ETH_MAC_LEN);
  frame.ether_type = PLC_ETHERCAT_ETH_TYPE;
  frame.length = len;
  memcpy(frame.payload, data, len);

  return plc_eth_send_raw(ETH_ID_SELF, &frame);
}

/** 接收原始以太网帧 */
static int recv_raw_frame(uint8_t* data, uint16_t* len, uint32_t timeout_ms)
{
  PlcEthFrame frame;
  int ret;

  ret = plc_eth_recv_raw(ETH_ID_SELF, &frame, timeout_ms);
  if (ret < 0) return ret;

  if (frame.ether_type != PLC_ETHERCAT_ETH_TYPE) return -2;

  *len = frame.length;
  memcpy(data, frame.payload, frame.length);
  return 0;
}

/** 构建并发送 EtherCAT 帧 */
static int ethercat_send(uint8_t cmd, uint16_t addr,
                          const uint8_t* payload, uint16_t payload_len)
{
  uint8_t frame[256];
  EthercatHeader* hdr;
  uint16_t frame_len;

  if (payload_len + sizeof(EthercatHeader) > sizeof(frame)) return -1;

  hdr = (EthercatHeader*)frame;
  hdr->ether_type = PLC_ETHERCAT_ETH_TYPE;
  hdr->cmd = cmd;
  hdr->idx = s_frame_idx++;
  hdr->addr = addr;
  hdr->len = payload_len;
  hdr->irq = 0;

  if (payload && payload_len > 0) {
    memcpy(frame + sizeof(EthercatHeader), payload, payload_len);
  }

  frame_len = (uint16_t)(sizeof(EthercatHeader) + payload_len);
  return send_raw_frame(frame, frame_len);
}

/** 接收并解析 EtherCAT 帧 */
static int ethercat_recv(uint8_t* cmd_out, uint8_t* idx_out,
                          uint8_t* payload, uint16_t* payload_len,
                          uint32_t timeout_ms)
{
  uint8_t frame[256];
  uint16_t frame_len;
  EthercatHeader* hdr;
  int ret;

  ret = recv_raw_frame(frame, &frame_len, timeout_ms);
  if (ret < 0) return ret;

  if (frame_len < sizeof(EthercatHeader)) return -3;

  hdr = (EthercatHeader*)frame;
  *cmd_out = hdr->cmd;
  *idx_out = hdr->idx;
  *payload_len = hdr->len;

  if (hdr->len > 0 && hdr->len <= frame_len - sizeof(EthercatHeader)) {
    memcpy(payload, frame + sizeof(EthercatHeader), hdr->len);
  }

  return 0;
}

/** AL 状态转换 */
static int al_state_transition(PlcEthercatAlState target_state)
{
  uint8_t payload[2];
  uint8_t cmd, idx;
  uint16_t len;
  int ret;

  /* 写 AL 状态控制寄存器 (地址 0x0120) */
  payload[0] = (uint8_t)target_state;
  payload[1] = 0x00;

  ret = ethercat_send(PLC_ETHERCAT_CMD_APWR, 0x0120, payload, 2);
  if (ret < 0) return ret;

  /* 读取状态确认 */
  ret = ethercat_recv(&cmd, &idx, payload, &len, 100);
  if (ret < 0) return ret;

  s_al_state = (PlcEthercatAlState)(payload[0] & 0x0F);
  return 0;
}

/* ========== 公开接口实现 ========== */

int plc_ethercat_init(const PlcEthercatConfig* config)
{
  PlcEthConfig eth_cfg;

  if (!config) return -1;

  memcpy(&s_config, config, sizeof(PlcEthercatConfig));

  memset(s_slaves, 0, sizeof(s_slaves));
  memset(s_pdo_inputs, 0, sizeof(s_pdo_inputs));
  memset(s_pdo_outputs, 0, sizeof(s_pdo_outputs));
  memset(s_pdo_in_buf, 0, sizeof(s_pdo_in_buf));
  memset(s_pdo_out_buf, 0, sizeof(s_pdo_out_buf));
  memset(s_od, 0, sizeof(s_od));
  s_od_count = 0;
  s_slave_count = 0;
  s_pdo_input_count = 0;
  s_pdo_output_count = 0;
  s_pdo_in_size = 0;
  s_pdo_out_size = 0;
  s_wkc = 0;
  s_frame_idx = 0;
  s_al_state = PLC_ETHERCAT_AL_INIT;

  /* 初始化以太网接口 */
  memset(&eth_cfg, 0, sizeof(eth_cfg));
  /* 使用默认 MAC/IP 地址（实际项目中应从配置读取） */
  eth_cfg.mac[0] = 0x00; eth_cfg.mac[1] = 0x0E;
  eth_cfg.mac[2] = 0xC6; eth_cfg.mac[3] = 0x00;
  eth_cfg.mac[4] = 0x01; eth_cfg.mac[5] = 0x00;
  eth_cfg.ip[0] = 192; eth_cfg.ip[1] = 168;
  eth_cfg.ip[2] = 1;   eth_cfg.ip[3] = 100;
  eth_cfg.subnet[0] = 255; eth_cfg.subnet[1] = 255;
  eth_cfg.subnet[2] = 255; eth_cfg.subnet[3] = 0;
  eth_cfg.dhcp = false;
  eth_cfg.mtu = PLC_ETH_MTU;

  if (plc_eth_init(ETH_ID_SELF, &eth_cfg) < 0) {
    return -2;
  }

  s_initialized = true;
  plc_platform_log(PLC_LOG_INFO, "EtherCAT 初始化完成 (站名=%s)", s_config.station_name);
  return 0;
}

int plc_ethercat_start(void)
{
  if (!s_initialized) return -1;

  if (plc_eth_start(ETH_ID_SELF) < 0) return -2;

  /* 切换到 OP 状态 */
  if (s_config.auto_remap) {
    al_state_transition(PLC_ETHERCAT_AL_PRE_OP);
    al_state_transition(PLC_ETHERCAT_AL_SAFE_OP);
    al_state_transition(PLC_ETHERCAT_AL_OP);
  }

  plc_platform_log(PLC_LOG_INFO, "EtherCAT 启动 (AL状态=%d)", s_al_state);
  return 0;
}

int plc_ethercat_stop(void)
{
  if (!s_initialized) return -1;

  al_state_transition(PLC_ETHERCAT_AL_INIT);
  plc_eth_stop(ETH_ID_SELF);

  plc_platform_log(PLC_LOG_INFO, "EtherCAT 停止");
  return 0;
}

int plc_ethercat_scan(void)
{
  uint8_t payload[4];
  uint8_t cmd, idx;
  uint16_t len;
  int ret;
  uint8_t node;

  if (!s_initialized) return -1;

  s_slave_count = 0;

  /* 逐个探测从站地址 0-127 */
  for (node = 0; node < PLC_ETHERCAT_MAX_SLAVES && node < 128; node++) {
    /* 使用 APRD 命令读取从站 ID */
    ret = ethercat_send(PLC_ETHERCAT_CMD_APRD, 0, NULL, 0);
    if (ret < 0) continue;

    ret = ethercat_recv(&cmd, &idx, payload, &len, 50);
    if (ret < 0 || len < 2) continue;

    /* 从站存在，记录信息 */
    {
      PlcEthercatSlave* slave = &s_slaves[s_slave_count];
      memset(slave, 0, sizeof(PlcEthercatSlave));
      slave->node_id = (uint8_t)(payload[0] & 0x7F);
      slave->state = PLC_ETHERCAT_AL_INIT;
      s_slave_count++;
    }
  }

  plc_platform_log(PLC_LOG_INFO, "EtherCAT 扫描完成: 发现 %d 个从站", s_slave_count);
  return s_slave_count;
}

uint8_t plc_ethercat_get_slave_count(void)
{
  return s_slave_count;
}

int plc_ethercat_get_slave_info(uint8_t index, PlcEthercatSlave* info)
{
  if (!info || index >= s_slave_count) return -1;
  memcpy(info, &s_slaves[index], sizeof(PlcEthercatSlave));
  return 0;
}

int plc_ethercat_map_pdo_input(uint8_t slave, uint16_t offset,
                                void* var_ptr, uint8_t size)
{
  if (s_pdo_input_count >= PLC_ETHERCAT_MAX_PDO_ENTRIES) return -1;
  if (!var_ptr || size == 0) return -2;

  PlcEthercatPdoEntry* entry = &s_pdo_inputs[s_pdo_input_count++];
  entry->slave_index = slave;
  entry->offset = offset;
  entry->var_ptr = var_ptr;
  entry->size = size;

  /* 更新输入缓冲区总大小 */
  {
    uint16_t end = offset + size;
    if (end > s_pdo_in_size) s_pdo_in_size = end;
  }

  return 0;
}

int plc_ethercat_map_pdo_output(uint8_t slave, uint16_t offset,
                                 void* var_ptr, uint8_t size)
{
  if (s_pdo_output_count >= PLC_ETHERCAT_MAX_PDO_ENTRIES) return -1;
  if (!var_ptr || size == 0) return -2;

  PlcEthercatPdoEntry* entry = &s_pdo_outputs[s_pdo_output_count++];
  entry->slave_index = slave;
  entry->offset = offset;
  entry->var_ptr = var_ptr;
  entry->size = size;

  {
    uint16_t end = offset + size;
    if (end > s_pdo_out_size) s_pdo_out_size = end;
  }

  return 0;
}

int plc_ethercat_exchange(void)
{
  uint8_t i;
  uint8_t frame[256];
  uint16_t frame_len;
  uint8_t cmd, idx;
  uint16_t resp_len;
  int ret;

  if (!s_initialized) return -1;
  if (s_al_state != PLC_ETHERCAT_AL_OP &&
      s_al_state != PLC_ETHERCAT_AL_SAFE_OP) return -2;

  /* 收集输出 PDO 数据 */
  for (i = 0; i < s_pdo_output_count; i++) {
    PlcEthercatPdoEntry* entry = &s_pdo_outputs[i];
    if (entry->var_ptr && (entry->offset + entry->size) <= sizeof(s_pdo_out_buf)) {
      memcpy(s_pdo_out_buf + entry->offset, entry->var_ptr, entry->size);
    }
  }

  /* 构建 LRW（逻辑读写）帧并发送 */
  {
    EthercatHeader* hdr = (EthercatHeader*)frame;
    hdr->ether_type = PLC_ETHERCAT_ETH_TYPE;
    hdr->cmd = PLC_ETHERCAT_CMD_LRW;
    hdr->idx = s_frame_idx++;
    hdr->addr = 0x0000; /* 逻辑起始地址 */
    hdr->len = s_pdo_out_size > s_pdo_in_size ? s_pdo_out_size : s_pdo_in_size;
    hdr->irq = 0;

    if (hdr->len > 0 && hdr->len + sizeof(EthercatHeader) <= sizeof(frame)) {
      /* 复制输出数据 */
      if (s_pdo_out_size > 0) {
        memcpy(frame + sizeof(EthercatHeader), s_pdo_out_buf, s_pdo_out_size);
      }
      /* 填充输入数据区域（全 0） */
      if (s_pdo_in_size > s_pdo_out_size) {
        memset(frame + sizeof(EthercatHeader) + s_pdo_out_size, 0,
               s_pdo_in_size - s_pdo_out_size);
      }
    }

    frame_len = (uint16_t)(sizeof(EthercatHeader) + hdr->len);
    ret = send_raw_frame(frame, frame_len);
  }

  /* 接收响应 */
  ret = ethercat_recv(&cmd, &idx, frame, &resp_len, 10);
  if (ret == 0 && resp_len > 0) {
    /* 更新 WKC */
    s_wkc++;

    /* 从响应中提取输入数据 */
    if (resp_len >= s_pdo_in_size) {
      memcpy(s_pdo_in_buf, frame, s_pdo_in_size);
    }
  } else {
    /* 通信失败，WKC 归零 */
    s_wkc = 0;
  }

  /* 分发输入 PDO 到映射变量 */
  for (i = 0; i < s_pdo_input_count; i++) {
    PlcEthercatPdoEntry* entry = &s_pdo_inputs[i];
    if (entry->var_ptr && (entry->offset + entry->size) <= s_pdo_in_size) {
      memcpy(entry->var_ptr, s_pdo_in_buf + entry->offset, entry->size);
    }
  }

  return 0;
}

int plc_ethercat_sdo_write(uint8_t node_id, uint16_t index, uint8_t subindex,
                            const uint8_t* data, uint8_t len)
{
  uint8_t payload[16];
  uint8_t cmd, idx;
  uint16_t resp_len;
  int ret;
  uint8_t sdo_cmd;

  if (!s_initialized) return -1;
  if (len > 4) return -2;

  /* 构建 CoE SDO 下载请求 */
  switch (len) {
    case 1: sdo_cmd = 0x2F; break;
    case 2: sdo_cmd = 0x2B; break;
    case 3: sdo_cmd = 0x27; break;
    case 4: sdo_cmd = 0x23; break;
    default: sdo_cmd = 0x23; break;
  }

  payload[0] = sdo_cmd;
  payload[1] = (uint8_t)(index & 0xFF);
  payload[2] = (uint8_t)(index >> 8);
  payload[3] = subindex;
  memset(payload + 4, 0, 4);
  if (data && len > 0) {
    memcpy(payload + 4, data, len);
  }

  /* 通过 FPRD 写入 SDO 邮箱（地址 0x1800 + 节点偏移） */
  ret = ethercat_send(PLC_ETHERCAT_CMD_FPWR,
                       (uint16_t)(0x1800 + node_id),
                       payload, 8);
  if (ret < 0) return -3;

  /* 等待 SDO 响应 */
  ret = ethercat_recv(&cmd, &idx, payload, &resp_len,
                       PLC_ETHERCAT_SDO_TIMEOUT_MS);
  if (ret < 0) return -4;

  /* 检查响应码 */
  if ((payload[0] & 0xE0) == 0x60) {
    return 0; /* 成功 */
  }

  return -5;
}

int plc_ethercat_sdo_read(uint8_t node_id, uint16_t index, uint8_t subindex,
                           uint8_t* data, uint8_t max_len)
{
  uint8_t payload[16];
  uint8_t cmd, idx;
  uint16_t resp_len;
  int ret;
  uint8_t data_len;

  if (!s_initialized) return -1;
  if (!data) return -2;

  /* 构建 SDO 上传请求 */
  payload[0] = 0x40; /* 上传请求 */
  payload[1] = (uint8_t)(index & 0xFF);
  payload[2] = (uint8_t)(index >> 8);
  payload[3] = subindex;
  memset(payload + 4, 0, 4);

  /* 读取 SDO 邮箱 */
  ret = ethercat_send(PLC_ETHERCAT_CMD_FPRD,
                       (uint16_t)(0x1800 + node_id),
                       payload, 8);
  if (ret < 0) return -3;

  ret = ethercat_recv(&cmd, &idx, payload, &resp_len,
                       PLC_ETHERCAT_SDO_TIMEOUT_MS);
  if (ret < 0) return -4;

  /* 解析响应 */
  if ((payload[0] & 0xE0) == 0x40) {
    data_len = (uint8_t)(4 - ((payload[0] >> 2) & 0x03));
    if (data_len > max_len) data_len = max_len;
    memcpy(data, payload + 4, data_len);
    return (int)data_len;
  }

  return -5;
}

PlcEthercatAlState plc_ethercat_get_state(void)
{
  return s_al_state;
}

uint16_t plc_ethercat_get_wkc(void)
{
  return s_wkc;
}
