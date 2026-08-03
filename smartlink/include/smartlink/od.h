/**
 * od.h - 对象字典 (Object Dictionary) 定义
 *
 * 依据 EPSG DS 301 V1.2.0 第 5 章（对象字典）与 CANopen CiA 数据约定。
 * 对象字典是 智能总线 的参数中枢：SDO 读写、PDO 映射、诊断均通过 OD 访问。
 *
 * 采用扁平条目表设计：每个条目 = 一个 (index, subIndex) 主键，
 * 数组/记录对象通过 0x00 子索引（元素个数）加子项条目表表达。
 */

#ifndef SL_OD_H
#define SL_OD_H

#include "smartlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 数据访问标志 ========== */

#define SL_OD_ACC_READ  0x01   /* 可读 */
#define SL_OD_ACC_WRITE 0x02   /* 可写 */
#define SL_OD_ACC_CONST 0x04   /* 常量 */
#define SL_OD_ACC_PDO   0x08   /* 可映射到 PDO */
#define SL_OD_ACC_ARRAY 0x10   /* 数值数组 */
#define SL_OD_ACC_RANGE 0x20   /* 带上下限 */
#define SL_OD_ACC_VAR   0x40   /* 数据位于应用变量 */
#define SL_OD_ACC_STORE 0x80   /* 可存入非易失存储 */

/* 常用组合 */
#define SL_OD_ACC_R     SL_OD_ACC_READ
#define SL_OD_ACC_W     SL_OD_ACC_WRITE
#define SL_OD_ACC_RW    (SL_OD_ACC_READ | SL_OD_ACC_WRITE)
#define SL_OD_ACC_CR    (SL_OD_ACC_CONST | SL_OD_ACC_READ)
#define SL_OD_ACC_VR    (SL_OD_ACC_VAR | SL_OD_ACC_READ)
#define SL_OD_ACC_VW    (SL_OD_ACC_VAR | SL_OD_ACC_WRITE)
#define SL_OD_ACC_VRW   (SL_OD_ACC_VAR | SL_OD_ACC_READ | SL_OD_ACC_WRITE)
#define SL_OD_ACC_VPR   (SL_OD_ACC_VAR | SL_OD_ACC_PDO | SL_OD_ACC_READ)
#define SL_OD_ACC_VPW   (SL_OD_ACC_VAR | SL_OD_ACC_PDO | SL_OD_ACC_WRITE)
#define SL_OD_ACC_VPRW  (SL_OD_ACC_VAR | SL_OD_ACC_PDO | SL_OD_ACC_READ | SL_OD_ACC_WRITE)

/* ========== 数据类型 (CiA 标准码) ========== */

typedef enum
{
  SL_OD_TYPE_BOOL       = 0x01,
  SL_OD_TYPE_INT8       = 0x02,
  SL_OD_TYPE_INT16      = 0x03,
  SL_OD_TYPE_INT32      = 0x04,
  SL_OD_TYPE_UINT8      = 0x05,
  SL_OD_TYPE_UINT16     = 0x06,
  SL_OD_TYPE_UINT32     = 0x07,
  SL_OD_TYPE_REAL32     = 0x08,
  SL_OD_TYPE_VSTRING    = 0x09,   /* 可见字符串 */
  SL_OD_TYPE_OSTRING    = 0x0A,   /* 八位字节串 */
  SL_OD_TYPE_TIME_OF_DAY = 0x0C,
  SL_OD_TYPE_TIME_DIFF  = 0x0D,
  SL_OD_TYPE_DOMAIN     = 0x0F,
  SL_OD_TYPE_INT24      = 0x10,
  SL_OD_TYPE_REAL64     = 0x11,
  SL_OD_TYPE_INT40      = 0x12,
  SL_OD_TYPE_INT48      = 0x13,
  SL_OD_TYPE_INT56      = 0x14,
  SL_OD_TYPE_INT64      = 0x15,
  SL_OD_TYPE_UINT24     = 0x16,
  SL_OD_TYPE_UINT40     = 0x18,
  SL_OD_TYPE_UINT48     = 0x19,
  SL_OD_TYPE_UINT56     = 0x1A,
  SL_OD_TYPE_UINT64     = 0x1B,
  SL_OD_TYPE_BITSTRING  = 0x1F,
  SL_OD_TYPE_MAX        = 0x1C
} SlOdDataType;

/* ========== 对象类型 (CiA 标准码) ========== */

typedef enum
{
  SL_OD_OBJ_NULL    = 0x00,
  SL_OD_OBJ_DOMAIN  = 0x02,
  SL_OD_OBJ_VAR     = 0x07,
  SL_OD_OBJ_ARRAY   = 0x08,
  SL_OD_OBJ_RECORD  = 0x09,
} SlOdObjectType;

/* ========== 对象条目 ========== */

typedef struct SlOdEntry
{
  uint16_t            index;       /* 对象索引 */
  uint8_t             subIndex;    /* 子索引 (0=主项/个数) */
  uint8_t             access;      /* SL_OD_ACC_* 组合 */
  uint8_t             objectType;  /* SlOdObjectType */
  uint8_t             dataType;    /* SlOdDataType */
  uint16_t            size;        /* 单元素字节数 */
  void*               data;        /* 数据指针 */
  uint32_t            arraySize;   /* 数组/记录元素个数（主项 0x00 使用） */
  struct SlOdEntry*  subEntries;  /* 子项条目表（ARRAY/RECORD 使用，可空） */
  struct SlOdEntry*  next;        /* 表内链式指针 */
} SlOdEntry;

/* ========== 对象字典 ========== */

typedef struct
{
  SlOdEntry* head;   /* 条目链表头 */
  uint32_t    count;  /* 条目总数 */
} SlOd;

/* ========== 标准对象索引 ========== */

#define SL_OD_IDX_DEVICE_TYPE       0x1000
#define SL_OD_IDX_ERROR_REGISTER    0x1001
#define SL_OD_IDX_STD_DEVICE_PROFILE 0x1002
#define SL_OD_IDX_VENDOR_ID         0x1018
#define SL_OD_IDX_PRODUCT_CODE      0x1019
#define SL_OD_IDX_REVISION_NUMBER   0x1020
#define SL_OD_IDX_SERIAL_NUMBER     0x1021
#define SL_OD_IDX_NODE_ID           0x1100
#define SL_OD_IDX_CYCLE_TIME        0x1101
#define SL_OD_IDX_POLL_RESP_TIMEOUT 0x1102
#define SL_OD_IDX_ASYNCH_MTU        0x1103
#define SL_OD_IDX_MULTIPLEXED_CYCLE 0x1104
#define SL_OD_IDX_ASYNC_SLOT_TIMEOUT 0x1105
#define SL_OD_IDX_SYNC_TOLERANCE    0x1106
#define SL_OD_IDX_SDO_TIMEOUT       0x1107
#define SL_OD_IDX_PRESCALER         0x1108
#define SL_OD_IDX_IP_ADDRESS        0x1300
#define SL_OD_IDX_SUBNET_MASK       0x1301
#define SL_OD_IDX_DEFAULT_GATEWAY   0x1302
#define SL_OD_IDX_RX_PDO_MAP        0x1600   /* 接收 PDO 映射起点 */
#define SL_OD_IDX_TX_PDO_MAP        0x1A00   /* 发送 PDO 映射起点 */
#define SL_OD_IDX_RX_PDO            0x6000   /* 接收 PDO 数据起点 (CiA401) */
#define SL_OD_IDX_TX_PDO            0x6200   /* 发送 PDO 数据起点 (CiA401) */

/* ========== 接口 ========== */

/**
 * 初始化对象字典（空表）。
 */
void sl_od_init(SlOd* od);

/**
 * 添加一个条目到对象字典。
 * 若 (index, subIndex) 已存在则更新之。
 */
int sl_od_add(SlOd* od, SlOdEntry* entry);

/**
 * 查找条目。
 */
int sl_od_find(SlOd* od, uint16_t index, uint8_t subIndex, SlOdEntry** out);

/**
 * 读取对象数据（自动按访问权限校验）。
 * @param size  输入缓冲容量，输出实际字节数
 */
int sl_od_read(SlOd* od, uint16_t index, uint8_t subIndex,
                void* data, uint16_t* size);

/**
 * 写入对象数据（自动按访问权限校验）。
 * @param size  数据字节数
 */
int sl_od_write(SlOd* od, uint16_t index, uint8_t subIndex,
                 const void* data, uint16_t size);

/**
 * 查询对象数据大小（字节）。
 */
int sl_od_get_size(SlOd* od, uint16_t index, uint8_t subIndex,
                    uint16_t* size);

/**
 * 将数据类型码映射为字节数（0 表示非定长类型）。
 */
uint16_t sl_od_type_size(uint8_t dataType);

#ifdef __cplusplus
}
#endif

#endif /* SL_OD_H */
