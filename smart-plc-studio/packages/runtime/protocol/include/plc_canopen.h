/**
 * plc_canopen.h - CANopen DS301 协议接口
 *
 * 实现 CANopen 通信协议栈，包括：
 * - NMT 网络管理（状态机控制）
 * - SDO 服务数据对象（参数配置）
 * - PDO 过程数据对象（实时数据交换）
 * - 心跳/节点保护
 * - 简单对象字典
 */

#ifndef PLC_CANOPEN_H
#define PLC_CANOPEN_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_CANOPEN_MAX_PDO
  #define PLC_CANOPEN_MAX_PDO         8
#endif

#ifndef PLC_CANOPEN_MAX_OD_ENTRIES
  #define PLC_CANOPEN_MAX_OD_ENTRIES  128
#endif

#ifndef PLC_CANOPEN_MAX_PDO_MAPPING
  #define PLC_CANOPEN_MAX_PDO_MAPPING 8
#endif

/* ========== COB-ID 偏移定义 ========== */

#define PLC_CANOPEN_COB_SYNC         0x080
#define PLC_CANOPEN_COB_PDO1_TX_BASE 0x180
#define PLC_CANOPEN_COB_PDO1_RX_BASE 0x200
#define PLC_CANOPEN_COB_SDO_TX_BASE  0x580
#define PLC_CANOPEN_COB_SDO_RX_BASE  0x600
#define PLC_CANOPEN_COB_HEARTBEAT    0x700
#define PLC_CANOPEN_COB_NMT          0x000

/* ========== NMT 状态定义 ========== */

typedef enum {
  PLC_CANOPEN_STATE_INIT      = 0x00,
  PLC_CANOPEN_STATE_PRE_OP    = 0x7F,
  PLC_CANOPEN_STATE_SAFE_OP   = 0x4F,
  PLC_CANOPEN_STATE_OP        = 0x05
} PlcCanopenState;

/* NMT 命令码 */
#define PLC_CANOPEN_NMT_START_REMOTE_NODE   0x01
#define PLC_CANOPEN_NMT_STOP_REMOTE_NODE    0x02
#define PLC_CANOPEN_NMT_ENTER_PRE_OP        0x80
#define PLC_CANOPEN_NMT_RESET_NODE          0x81
#define PLC_CANOPEN_NMT_RESET_COMM          0x82

/* ========== 结构体定义 ========== */

/** CANopen 节点信息 */
typedef struct {
  uint8_t           node_id;            /* 节点 ID (1-127) */
  PlcCanopenState   state;              /* 当前 NMT 状态 */
  uint32_t          heartbeat_producer_ms; /* 心跳产生周期（毫秒） */
  uint32_t          heartbeat_consumer_ms; /* 心跳监控超时（毫秒） */
  uint16_t          error_code;          /* 错误码 */
} PlcCanopenNode;

/** PDO 配置 */
typedef struct {
  uint32_t          cob_id;             /* COB-ID */
  uint8_t           transmission_type;  /* 传输类型 (0-255) */
  void*             mapped_vars[PLC_CANOPEN_MAX_PDO_MAPPING]; /* 映射变量指针 */
  uint8_t           mapped_sizes[PLC_CANOPEN_MAX_PDO_MAPPING]; /* 映射大小(字节) */
  uint8_t           mapped_count;       /* 映射数量 */
  uint32_t          event_timer_ms;     /* 事件定时器（毫秒，0=禁用） */
} PlcCanopenPdo;

/** SDO 回调函数原型 */
typedef void (*PlcCanopenSdoCallback)(uint8_t node_id, uint16_t index,
                                       uint8_t subindex, const uint8_t* data,
                                       uint8_t len);

/** PDO 接收回调函数原型 */
typedef void (*PlcCanopenPdoCallback)(uint8_t pdo_index, const uint8_t* data,
                                       uint8_t len);

/** 对象字典条目 */
typedef struct {
  uint16_t  index;
  uint8_t   subindex;
  uint8_t   data[4];
  uint8_t   len;
} PlcCanOdEntry;

/* ========== 函数声明 ========== */

/**
 * 初始化 CANopen 协议栈
 * @param node_id 本节点 ID (1-127)
 * @return 0 成功, 负值错误码
 */
int plc_canopen_init(uint8_t node_id);

/**
 * 启动 CANopen（进入 OPERATIONAL）
 * @return 0 成功
 */
int plc_canopen_start(void);

/**
 * 停止 CANopen
 * @return 0 成功
 */
int plc_canopen_stop(void);

/**
 * 复位 CANopen 协议栈
 * @return 0 成功
 */
int plc_canopen_reset(void);

/**
 * 发送 NMT 命令（仅主站）
 * @param command    NMT 命令码
 * @param target_node 目标节点 ID (0=广播)
 * @return 0 成功, 负值错误码
 */
int plc_canopen_nmt(uint8_t command, uint8_t target_node);

/**
 * SDO 写入（快速传输，数据≤4字节）
 * @param node_id  目标节点
 * @param index    对象索引
 * @param subindex 对象子索引
 * @param data     数据指针
 * @param len      数据长度（1-4）
 * @return 0 成功, 负值错误码
 */
int plc_canopen_sdo_write(uint8_t node_id, uint16_t index, uint8_t subindex,
                           const uint8_t* data, uint8_t len);

/**
 * SDO 读取（快速传输，数据≤4字节）
 * @param node_id  目标节点
 * @param index    对象索引
 * @param subindex 对象子索引
 * @param data     输出缓冲区
 * @param max_len  缓冲区最大长度
 * @return 0 成功, 负值错误码
 */
int plc_canopen_sdo_read(uint8_t node_id, uint16_t index, uint8_t subindex,
                          uint8_t* data, uint8_t max_len);

/**
 * 发送 PDO
 * @param pdo_index PDO 索引 (0-7)
 * @param data      数据指针
 * @param len       数据长度
 * @return 0 成功, 负值错误码
 */
int plc_canopen_pdo_send(uint8_t pdo_index, const uint8_t* data, uint8_t len);

/**
 * 注册 PDO 接收回调
 * @param pdo_index PDO 索引
 * @param callback  回调函数（NULL 取消注册）
 * @return 0 成功
 */
int plc_canopen_pdo_register_callback(uint8_t pdo_index,
                                       PlcCanopenPdoCallback callback);

/**
 * 发送 SYNC 同步报文
 * @return 0 成功, 负值错误码
 */
int plc_canopen_sync(void);

/**
 * 发送心跳报文
 * @param state 当前 NMT 状态
 * @return 0 成功, 负值错误码
 */
int plc_canopen_heartbeat(PlcCanopenState state);

/**
 * 主循环处理（状态机推进、心跳发送、PDO 周期发送）
 * 每个扫描周期调用一次
 * @return 0 成功
 */
int plc_canopen_process(void);

#ifdef __cplusplus
}
#endif

#endif /* PLC_CANOPEN_H */
