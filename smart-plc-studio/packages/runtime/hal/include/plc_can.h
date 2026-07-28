/**
 * plc_can.h - CAN 总线硬件抽象层接口
 *
 * 提供 CAN 收发、过滤器配置等统一接口
 * 支持 CAN 2.0A/B 和 CAN FD
 */

#ifndef PLC_CAN_H
#define PLC_CAN_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_CAN_MAX_MSG
  #define PLC_CAN_MAX_MSG         32
#endif

#ifndef PLC_CAN_MAX_FILTERS
  #define PLC_CAN_MAX_FILTERS     16
#endif

#ifndef PLC_CAN_DATA_MAX
  #define PLC_CAN_DATA_MAX        8
#endif

/* ========== 枚举定义 ========== */

/** CAN 波特率 */
typedef enum {
  PLC_CAN_BAUD_125K  = 125000,
  PLC_CAN_BAUD_250K  = 250000,
  PLC_CAN_BAUD_500K  = 500000,
  PLC_CAN_BAUD_1M    = 1000000,
} PlcCanBaud;

/** CAN 消息格式 */
typedef enum {
  PLC_CAN_STD    = 0,   /* 标准帧 (11-bit ID) */
  PLC_CAN_EXT    = 1,   /* 扩展帧 (29-bit ID) */
} PlcCanFrame;

/** CAN 模式 */
typedef enum {
  PLC_CAN_MODE_NORMAL  = 0,
  PLC_CAN_MODE_LOOPBACK = 1,
  PLC_CAN_MODE_LISTEN_ONLY = 2,
} PlcCanMode;

/** CAN 状态 */
typedef enum {
  PLC_CAN_STATE_STOPPED   = 0,
  PLC_CAN_STATE_ACTIVE    = 1,
  PLC_CAN_STATE_BUS_OFF   = 2,
  PLC_CAN_STATE_ERROR     = 3,
} PlcCanState;

/* ========== 结构体定义 ========== */

/** CAN 消息 */
typedef struct {
  uint32_t     id;                /* 报文 ID */
  PlcCanFrame  frame;             /* 标准/扩展帧 */
  uint8_t      dlc;               /* 数据长度 (0-8) */
  uint8_t      data[PLC_CAN_DATA_MAX]; /* 数据 */
  uint32_t     timestamp;         /* 时间戳 (毫秒) */
  bool         is_remote;         /* 远程帧标志 */
} PlcCanMsg;

/** CAN 过滤器 */
typedef struct {
  uint32_t     id;                /* 过滤 ID */
  uint32_t     mask;              /* 掩码 */
  PlcCanFrame  frame;             /* 标准/扩展帧 */
  bool         enabled;           /* 是否启用 */
} PlcCanFilter;

/** CAN 配置 */
typedef struct {
  PlcCanBaud   baud;              /* 波特率 */
  PlcCanMode   mode;              /* 工作模式 */
  PlcCanFilter filters[PLC_CAN_MAX_FILTERS]; /* 过滤器 */
  uint8_t      filter_count;      /* 过滤器数量 */
} PlcCanConfig;

/** CAN 统计信息 */
typedef struct {
  uint32_t     tx_count;          /* 发送计数 */
  uint32_t     rx_count;          /* 接收计数 */
  uint32_t     error_count;       /* 错误计数 */
  uint32_t     overrun_count;     /* 溢出计数 */
  PlcCanState  state;             /* 当前状态 */
} PlcCanStats;

/** CAN 接收回调函数原型 */
typedef void (*PlcCanRxCallback)(uint8_t can_id, const PlcCanMsg* msg);

/* ========== 函数声明 ========== */

/**
 * 初始化 CAN 接口
 * @param can_id CAN 接口编号
 * @param config 配置参数
 * @return 0 成功, 负值错误码
 */
int plc_can_init(uint8_t can_id, const PlcCanConfig* config);

/**
 * 发送 CAN 消息
 * @param can_id CAN 接口编号
 * @param msg    消息指针
 * @return 0 成功, 负值错误码
 */
int plc_can_send(uint8_t can_id, const PlcCanMsg* msg);

/**
 * 接收 CAN 消息
 * @param can_id CAN 接口编号
 * @param msg    输出消息
 * @param timeout_ms 超时（毫秒）
 * @return 1 收到消息, 0 超时, 负值错误
 */
int plc_can_recv(uint8_t can_id, PlcCanMsg* msg, uint32_t timeout_ms);

/**
 * 启动 CAN 接口
 */
int plc_can_start(uint8_t can_id);

/**
 * 停止 CAN 接口
 */
int plc_can_stop(uint8_t can_id);

/**
 * 设置过滤器
 */
int plc_can_set_filter(uint8_t can_id, uint8_t filter_idx, const PlcCanFilter* filter);

/**
 * 注册接收回调
 */
int plc_can_set_rx_callback(uint8_t can_id, PlcCanRxCallback callback);

/**
 * 获取统计信息
 */
int plc_can_get_stats(uint8_t can_id, PlcCanStats* stats);

/**
 * 清除统计信息
 */
int plc_can_clear_stats(uint8_t can_id);

#ifdef __cplusplus
}
#endif

#endif /* PLC_CAN_H */
