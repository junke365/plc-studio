/**
 * sdo.h - SDO (Service Data Object) 服务定义
 *
 * 依据 EPSG DS 301 V1.2.0 第 6.3 章（SDO Sequence Layer + Command Layer）。
 * SDO 用于参数访问（对象字典读写），传输方式：
 *   - ASnd 帧内嵌（异步阶段）
 *   - UDP (端口 3819)
 *
 * 序列层保证：不丢失、不重复、有序。命令层区分：WriteByIndex、
 * ReadByIndex、WriteMultParam、ReadMultParam；支持分段传输大对象。
 */

#ifndef SL_SDO_H
#define SL_SDO_H

#include "smartlink.h"
#include "frame.h"
#include "od.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== SDO 命令 ID ========== */

typedef enum
{
  SL_SDO_CID_WRITE_BY_INDEX  = 0x01,   /* 按索引写 */
  SL_SDO_CID_READ_BY_INDEX   = 0x02,   /* 按索引读 */
  SL_SDO_CID_WRITE_MULT_PARAM = 0x03,  /* 批量写 (V1.3.0) */
  SL_SDO_CID_READ_MULT_PARAM  = 0x04,  /* 批量读 (V1.3.0) */
} SlSdoCommandId;

/* ========== SDO abort 码 (CiA 标准) ========== */

#define SL_SDO_ABORT_OK                0x00000000
#define SL_SDO_ABORT_TOGGLE_BIT        0x05030000
#define SL_SDO_ABORT_TIMEOUT           0x05040000
#define SL_SDO_ABORT_CMD_SPECIFIER     0x05040001
#define SL_SDO_ABORT_UNSUPPORTED_ACCESS 0x06010000
#define SL_SDO_ABORT_READ_ONLY         0x06010001
#define SL_SDO_ABORT_WRITE_ONLY        0x06010002
#define SL_SDO_ABORT_INDEX_NOT_EXIST   0x06020000
#define SL_SDO_ABORT_MAPPING_ILLEGAL   0x06040041
#define SL_SDO_ABORT_PARAM_INCOMPAT    0x06040043
#define SL_SDO_ABORT_INTERNAL_INCOMPAT 0x06040047
#define SL_SDO_ABORT_HARDWARE_ERROR    0x06060000
#define SL_SDO_ABORT_DATA_TYPE_MISMATCH 0x06070010
#define SL_SDO_ABORT_SUBINDEX_NOT_EXIST 0x06090011
#define SL_SDO_ABORT_INVALID_VALUE     0x06090030
#define SL_SDO_ABORT_VALUE_TOO_HIGH    0x06090031
#define SL_SDO_ABORT_VALUE_TOO_LOW     0x06090032
#define SL_SDO_ABORT_GENERAL           0x08000000
#define SL_SDO_ABORT_CANNOT_STORE      0x08000020
#define SL_SDO_ABORT_DEVICE_STATE      0x08000022

/* ========== 传输方向 ========== */

typedef enum
{
  SL_SDO_DIR_NONE = 0,
  SL_SDO_DIR_DOWNLOAD,   /* 客户端 → 服务器（写对象） */
  SL_SDO_DIR_UPLOAD,     /* 服务器 → 客户端（读对象） */
} SlSdoDirection;

/* ========== 序列层状态 ========== */

typedef enum
{
  SL_SDO_SEQ_IDLE = 0,
  SL_SDO_SEQ_ACTIVE,      /* 序列激活 */
  SL_SDO_SEQ_WAIT_ACK,    /* 等待确认 */
  SL_SDO_SEQ_CLOSED,      /* 连接关闭 */
} SlSdoSeqState;

/* ========== 命令层状态 ========== */

typedef enum
{
  SL_SDO_CMD_IDLE = 0,
  SL_SDO_CMD_TRANSFER,    /* 传输中（含分段） */
  SL_SDO_CMD_COMPLETE,    /* 传输完成 */
  SL_SDO_CMD_ABORTED,     /* 已中止 */
} SlSdoCmdState;

/* ========== SDO 连接上下文（每远程节点一个） ========== */

typedef struct SlSdoCnx
{
  uint8_t           nodeId;          /* 对端节点 ID */
  SlSdoDirection   direction;       /* 传输方向 */
  SlSdoSeqState    seqState;        /* 序列层状态 */
  SlSdoCmdState    cmdState;        /* 命令层状态 */
  uint8_t           sendSeqNum;      /* 本端序列号 */
  uint8_t           recvSeqNum;      /* 对端最近确认序列号 */
  uint8_t           transactionId;   /* 事务 ID */
  uint8_t           commandId;       /* 当前命令 ID */
  uint16_t          index;           /* 当前对象索引 */
  uint8_t           subIndex;        /* 当前子索引 */
  uint32_t          abortCode;       /* 中止码 */
  uint8_t*          buffer;          /* 传输缓冲 */
  uint16_t          bufferSize;      /* 缓冲容量 */
  uint16_t          transferSize;    /* 传输总字节数 */
  uint16_t          offset;          /* 已传输偏移 */
  bool              segmented;       /* 是否分段传输 */
} SlSdoCnx;

/* ========== I/O 接口 ========== */

/**
 * 发送回调：发送一个完整以太网帧。
 */
typedef int (*SlSdoSendFn)(const uint8_t* frame, uint16_t len);

/**
 * 泵函数：从网络取回一帧（阻塞至多 timeoutMs）。
 * @return 0 有帧；SL_ERR_TIMEOUT 超时
 */
typedef int (*SlSdoPumpFn)(uint8_t* frame, uint16_t maxLen,
                            uint16_t* len, uint32_t timeoutMs);

/**
 * 配置 SDO 的收发通道（发送帧 + 接收泵）。
 */
int sl_sdo_set_io(SlSdoSendFn sendFn, SlSdoPumpFn pumpFn);

/**
 * 设置本机节点 ID（用于过滤/构建 ASnd 帧）。
 */
int sl_sdo_set_local_node(uint8_t nodeId);

/* ========== 接口 ========== */

/**
 * 初始化 SDO 服务。
 * @param maxCnx  最大并发连接数
 * @param od      本地对象字典（服务器侧访问目标）
 */
int sl_sdo_init(uint32_t maxCnx, SlOd* od);

/**
 * 处理接收到的 ASnd SDO 帧（服务器侧入口）。
 * @param frame  收到的完整 智能总线 帧
 * @return 0 已处理；SL_ERR_NO_DATA 表示非本节点 SDO 帧
 */
int sl_sdo_process_rx(const SlFrame* frame);

/**
 * 发起按索引读（客户端）。
 * 阻塞等待应答（内部轮询）。
 * @param remoteNode 目标节点
 * @param index      对象索引
 * @param subIndex   子索引
 * @param data       读出数据缓冲
 * @param size       输入容量，输出实际字节数
 * @param timeoutMs  超时
 */
int sl_sdo_read_req(uint8_t remoteNode, uint16_t index, uint8_t subIndex,
                     void* data, uint16_t* size, uint32_t timeoutMs);

/**
 * 发起按索引写（客户端）。
 * @param remoteNode 目标节点
 * @param index      对象索引
 * @param subIndex   子索引
 * @param data       数据
 * @param size       数据字节数
 * @param timeoutMs  超时
 */
int sl_sdo_write_req(uint8_t remoteNode, uint16_t index, uint8_t subIndex,
                      const void* data, uint16_t size, uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* SL_SDO_H */
