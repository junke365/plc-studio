/**
 * plc_io.h - I/O 管理模块
 *
 * 管理数字量/模拟量输入输出通道
 * 支持映射到物理地址或变量
 */

#ifndef PLC_IO_H
#define PLC_IO_H

#include "plc_platform.h"
#include "plc_var.h"

/* ========== I/O 类型 ========== */

typedef enum {
  IO_TYPE_DI,       /* 数字量输入 */
  IO_TYPE_DO,       /* 数字量输出 */
  IO_TYPE_AI,       /* 模拟量输入 (16bit) */
  IO_TYPE_AO,       /* 模拟量输出 (16bit) */
  IO_TYPE_PWM,      /* PWM 输出 */
  IO_TYPE_ENCODER,  /* 编码器输入 */
  IO_TYPE_COUNTER,  /* 高速计数器 */
} IoType;

/* I/O 通道状态标志 */
#define IO_STATUS_OK        0x00
#define IO_STATUS_OVERFLOW  0x01
#define IO_STATUS_UNDERFLOW 0x02
#define IO_STATUS_FAULT     0x04
#define IO_STATUS_OVERRANGE 0x08

/* I/O 通道描述 */
typedef struct {
  uint16_t       channel_id;
  IoType         type;
  const char*    name;
  const char*    var_name;       /* 绑定的变量名 */
  PlcVariable*   var;            /* 绑定的变量指针 */
  uint32_t       physical_addr;  /* 物理地址（平台相关） */
  int32_t        raw_value;      /* 原始值 */
  int32_t        scaled_value;   /* 缩放后值 */
  float          scale_factor;   /* 缩放因子 */
  int32_t        offset;         /* 偏移量 */
  int32_t        min_value;      /* 最小值 */
  int32_t        max_value;      /* 最大值 */
  uint8_t        status;         /* 状态标志 */
  bool           enabled;        /* 是否启用 */
} PlcIoChannel;

/* I/O 配置 */
typedef struct {
  PlcIoChannel   channels[PLC_MAX_IO_CHANNELS];
  uint16_t       channel_count;
  bool           initialized;
} PlcIoConfig;

/* ========== 接口函数 ========== */

/**
 * 初始化 I/O 系统
 */
void plc_io_init(PlcIoConfig* config);

/**
 * 注册 I/O 通道
 * @return 通道ID，-1=失败
 */
int plc_io_register(PlcIoConfig* config, IoType type, const char* name,
                    const char* var_name, uint32_t physical_addr);

/**
 * 绑定 I/O 通道到变量
 */
int plc_io_bind(PlcIoConfig* config, uint16_t channel_id, PlcVarTable* var_table);

/**
 * 读取所有输入通道（从硬件读取到变量）
 */
void plc_io_read_inputs(PlcIoConfig* config);

/**
 * 写入所有输出通道（从变量写入到硬件）
 */
void plc_io_write_outputs(PlcIoConfig* config);

/**
 * 读取单个通道值
 */
int32_t plc_io_read_channel(PlcIoConfig* config, uint16_t channel_id);

/**
 * 写入单个通道值
 */
void plc_io_write_channel(PlcIoConfig* config, uint16_t channel_id, int32_t value);

/**
 * 设置缩放参数
 */
void plc_io_set_scale(PlcIoConfig* config, uint16_t channel_id,
                      float factor, int32_t offset);

/**
 * 平台层实现：读取物理输入
 * 由各平台的 platform.c 实现
 */
int32_t plc_hal_read_input(uint32_t physical_addr, IoType type);

/**
 * 平台层实现：写入物理输出
 */
void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value);

#endif /* PLC_IO_H */
