/**
 * plc_ethercat.h - EtherCAT 协议接口（简化版）
 *
 * 基于原始以太网帧实现简化版 EtherCAT 协议栈
 * 支持 AL 状态机、SDO 参数配置、PDO 过程数据交换
 * 需要底层 plc_eth HAL 支持
 */

#ifndef PLC_ETHERCAT_H
#define PLC_ETHERCAT_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_ETHERCAT_MAX_SLAVES
  #define PLC_ETHERCAT_MAX_SLAVES      128
#endif

#ifndef PLC_ETHERCAT_MAX_PDO_ENTRIES
  #define PLC_ETHERCAT_MAX_PDO_ENTRIES 16
#endif

#ifndef PLC_ETHERCAT_SDO_TIMEOUT_MS
  #define PLC_ETHERCAT_SDO_TIMEOUT_MS  1000
#endif

#ifndef PLC_ETHERCAT_MAX_OD_ENTRIES
  #define PLC_ETHERCAT_MAX_OD_ENTRIES  64
#endif

#ifndef PLC_ETHERCAT_NAME_MAX
  #define PLC_ETHERCAT_NAME_MAX        32
#endif

/* ========== EtherCAT 帧类型 ========== */

#define PLC_ETHERCAT_ETH_TYPE          0x88A4

/* EtherCAT 命令码 */
#define PLC_ETHERCAT_CMD_NOP           0x00
#define PLC_ETHERCAT_CMD_APRD          0x01
#define PLC_ETHERCAT_CMD_APWR          0x02
#define PLC_ETHERCAT_CMD_AWRD          0x03
#define PLC_ETHERCAT_CMD_FPRD          0x04
#define PLC_ETHERCAT_CMD_FPWR          0x05
#define PLC_ETHERCAT_CMD_BRD           0x06
#define PLC_ETHERCAT_CMD_BWR           0x07
#define PLC_ETHERCAT_CMD_LRD           0x08
#define PLC_ETHERCAT_CMD_LWR           0x09
#define PLC_ETHERCAT_CMD_LRW           0x0A
#define PLC_ETHERCAT_CMD_LCS           0x0B

/* AL 状态 */
typedef enum {
  PLC_ETHERCAT_AL_INIT     = 0x01,
  PLC_ETHERCAT_AL_PRE_OP   = 0x02,
  PLC_ETHERCAT_AL_SAFE_OP  = 0x04,
  PLC_ETHERCAT_AL_OP       = 0x08
} PlcEthercatAlState;

/* ========== 结构体定义 ========== */

/** EtherCAT 主站配置 */
typedef struct {
  char       station_name[PLC_ETHERCAT_NAME_MAX]; /* 站名 */
  plc_bool   auto_remap;           /* PDO 自动重映射 */
  uint32_t   watchdog_ms;          /* 看门狗超时（毫秒，0=禁用） */
} PlcEthercatConfig;

/** 从站信息 */
typedef struct {
  uint8_t              node_id;       /* 从站节点 ID */
  uint32_t             vendor_id;     /* 厂商 ID */
  uint32_t             product_code;  /* 产品代码 */
  uint32_t             revision;      /* 版本号 */
  PlcEthercatAlState   state;         /* AL 状态 */
  uint8_t              pdo_in_size;   /* 输入 PDO 大小（字节） */
  uint8_t              pdo_out_size;  /* 输出 PDO 大小（字节） */
  char                 name[PLC_ETHERCAT_NAME_MAX]; /* 设备名 */
} PlcEthercatSlave;

/** PDO 映射条目 */
typedef struct {
  uint8_t   slave_index;    /* 从站索引 */
  uint16_t  offset;         /* PDO 字节偏移 */
  void*     var_ptr;        /* 映射变量指针 */
  uint8_t   size;           /* 数据大小（字节） */
} PlcEthercatPdoEntry;

/** CoE 对象字典条目 */
typedef struct {
  uint16_t  index;          /* 对象索引 */
  uint8_t   subindex;       /* 子索引 */
  uint8_t   data[4];        /* 数据（快速传输 ≤4字节） */
  uint8_t   len;            /* 数据长度 */
} PlcEthercatOdEntry;

/* ========== 函数声明 ========== */

/**
 * 初始化 EtherCAT 主站
 * @param config 配置参数
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_init(const PlcEthercatConfig* config);

/**
 * 启动 EtherCAT（进入 OP 状态）
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_start(void);

/**
 * 停止 EtherCAT
 * @return 0 成功
 */
int plc_ethercat_stop(void);

/**
 * 扫描从站拓扑
 * @return 检测到的从站数量, 负值错误码
 */
int plc_ethercat_scan(void);

/**
 * 获取从站数量
 * @return 从站数量
 */
uint8_t plc_ethercat_get_slave_count(void);

/**
 * 获取从站信息
 * @param index 从站索引
 * @param info  输出从站信息
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_get_slave_info(uint8_t index, PlcEthercatSlave* info);

/**
 * 映射输入 PDO（从站→主站）
 * @param slave  从站索引
 * @param offset PDO 字节偏移
 * @param var_ptr 映射变量指针
 * @param size   数据大小（字节）
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_map_pdo_input(uint8_t slave, uint16_t offset,
                                void* var_ptr, uint8_t size);

/**
 * 映射输出 PDO（主站→从站）
 * @param slave  从站索引
 * @param offset PDO 字节偏移
 * @param var_ptr 映射变量指针
 * @param size   数据大小（字节）
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_map_pdo_output(uint8_t slave, uint16_t offset,
                                 void* var_ptr, uint8_t size);

/**
 * 执行过程数据交换
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_exchange(void);

/**
 * SDO 写入
 * @param node_id  从站节点 ID
 * @param index    对象索引
 * @param subindex 子索引
 * @param data     数据指针
 * @param len      数据长度
 * @return 0 成功, 负值错误码
 */
int plc_ethercat_sdo_write(uint8_t node_id, uint16_t index, uint8_t subindex,
                            const uint8_t* data, uint8_t len);

/**
 * SDO 读取
 * @param node_id  从站节点 ID
 * @param index    对象索引
 * @param subindex 子索引
 * @param data     输出缓冲区
 * @param max_len  缓冲区最大长度
 * @return 读取的数据长度, 负值错误码
 */
int plc_ethercat_sdo_read(uint8_t node_id, uint16_t index, uint8_t subindex,
                           uint8_t* data, uint8_t max_len);

/**
 * 获取主站 AL 状态
 * @return 当前 AL 状态
 */
PlcEthercatAlState plc_ethercat_get_state(void);

/**
 * 获取工作计数器（Working Counter）
 * @return WKC 值
 */
uint16_t plc_ethercat_get_wkc(void);

#ifdef __cplusplus
}
#endif

#endif /* PLC_ETHERCAT_H */
