/**
 * plc_var.h - 变量管理模块
 *
 * 管理 IEC 61131-3 变量的内存存储、读写、强制等操作
 * 支持 BOOL, INT, DINT, REAL, STRING 等 IEC 标准类型
 */

#ifndef PLC_VAR_H
#define PLC_VAR_H

#include "plc_platform.h"

/* ========== 变量类型 ========== */

typedef enum {
  VAR_TYPE_BOOL,
  VAR_TYPE_SINT,
  VAR_TYPE_INT,
  VAR_TYPE_DINT,
  VAR_TYPE_LINT,
  VAR_TYPE_USINT,
  VAR_TYPE_UINT,
  VAR_TYPE_UDINT,
  VAR_TYPE_ULINT,
  VAR_TYPE_BYTE,
  VAR_TYPE_WORD,
  VAR_TYPE_DWORD,
  VAR_TYPE_LWORD,
  VAR_TYPE_REAL,
  VAR_TYPE_LREAL,
  VAR_TYPE_TIME,
  VAR_TYPE_STRING,
  VAR_TYPE_ARRAY,
  VAR_TYPE_STRUCT,
} VarType;

/* 变量属性 */
typedef enum {
  VAR_ATTR_LOCAL   = 0x00,
  VAR_ATTR_INPUT   = 0x01,
  VAR_ATTR_OUTPUT  = 0x02,
  VAR_ATTR_INOUT   = 0x03,
  VAR_ATTR_GLOBAL  = 0x04,
  VAR_ATTR_TEMP    = 0x05,
} VarAttr;

/* 变量描述 */
typedef struct {
  const char*    name;
  VarType        type;
  VarAttr        attr;
  uint32_t       size;        /* 字节数 */
  uint32_t       offset;      /* 在变量区中的偏移 */
  void*          data;        /* 指向实际数据 */
  bool           forced;      /* 是否被强制 */
  uint8_t        quality;     /* 质量码: 0=good, 1=uncertain, 2=bad */
  const char*    comment;     /* 注释 */
} PlcVariable;

/* 变量表 */
typedef struct {
  PlcVariable    vars[PLC_MAX_VARIABLES];
  uint32_t       count;
  uint8_t        var_data[PLC_MAX_VARIABLES * 256]; /* 变量数据区 */
  uint32_t       data_offset;                        /* 当前偏移 */
} PlcVarTable;

/* ========== 接口函数 ========== */

/**
 * 初始化变量表
 */
void plc_var_init(PlcVarTable* table);

/**
 * 注册变量
 * @return 0=成功, -1=表满, -2=名称重复
 */
int plc_var_register(PlcVarTable* table, const char* name, VarType type,
                     VarAttr attr, uint32_t size, const char* comment);

/**
 * 按名称查找变量
 * @return 变量指针，未找到返回 NULL
 */
PlcVariable* plc_var_find(PlcVarTable* table, const char* name);

/**
 * 按索引查找变量
 */
PlcVariable* plc_var_find_by_index(PlcVarTable* table, uint32_t index);

/**
 * 读取变量值
 * @param dest 目标缓冲区
 * @param max_size 缓冲区大小
 * @return 实际读取的字节数
 */
int plc_var_read(PlcVarTable* table, const char* name, void* dest, uint32_t max_size);

/**
 * 写入变量值
 * @param src 源数据
 * @param size 数据大小
 * @return 0=成功
 */
int plc_var_write(PlcVarTable* table, const char* name, const void* src, uint32_t size);

/**
 * 强制变量值（调试用）
 */
int plc_var_force(PlcVarTable* table, const char* name, const void* value);

/**
 * 取消强制
 */
int plc_var_unforce(PlcVarTable* table, const char* name);

/**
 * 获取变量总数
 */
uint32_t plc_var_count(PlcVarTable* table);

/**
 * 获取类型名称字符串
 */
const char* plc_var_type_name(VarType type);

/**
 * 获取类型大小（字节）
 */
uint32_t plc_var_type_size(VarType type);

#endif /* PLC_VAR_H */
