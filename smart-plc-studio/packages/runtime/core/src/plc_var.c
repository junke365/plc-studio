/**
 * plc_var.c - 变量管理模块实现
 *
 * 实现 IEC 61131-3 变量的内存存储、读写、强制等操作
 * 使用 var_data 数组作为简易碰撞分配器（bump allocator）
 */

#include "plc_var.h"
#include <string.h>

/* ========== 类型信息表 ========== */

/* 类型名称映射 */
static const char* type_names[] = {
  "BOOL", "SINT", "INT", "DINT", "LINT",
  "USINT", "UINT", "UDINT", "ULINT",
  "BYTE", "WORD", "DWORD", "LWORD",
  "REAL", "LREAL", "TIME", "STRING",
  "ARRAY", "STRUCT",
};

/* 类型大小映射（字节） */
static const uint32_t type_sizes[] = {
  sizeof(plc_bool),     /* BOOL */
  sizeof(plc_sint),     /* SINT */
  sizeof(plc_int),      /* INT */
  sizeof(plc_dint),     /* DINT */
  sizeof(plc_lint),     /* LINT */
  sizeof(plc_usint),    /* USINT */
  sizeof(plc_uint),     /* UINT */
  sizeof(plc_udint),    /* UDINT */
  sizeof(plc_ulint),    /* ULINT */
  sizeof(plc_byte),     /* BYTE */
  sizeof(plc_word),     /* WORD */
  sizeof(plc_dword),    /* DWORD */
  sizeof(plc_lword),    /* LWORD */
  sizeof(plc_real),     /* REAL */
  sizeof(plc_lreal),    /* LREAL */
  sizeof(plc_time),     /* TIME */
  255,                  /* STRING (默认长度) */
  0,                    /* ARRAY (大小在注册时指定) */
  0,                    /* STRUCT (大小在注册时指定) */
};

/* ========== 接口函数实现 ========== */

void plc_var_init(PlcVarTable* table) {
  if (table == NULL) return;
  memset(table, 0, sizeof(PlcVarTable));
}

int plc_var_register(PlcVarTable* table, const char* name, VarType type,
                     VarAttr attr, uint32_t size, const char* comment) {
  if (table == NULL || name == NULL) return -1;

  /* 检查表是否已满 */
  if (table->count >= PLC_MAX_VARIABLES) return -1;

  /* 检查名称是否重复 */
  for (uint32_t i = 0; i < table->count; i++) {
    if (table->vars[i].name != NULL && strcmp(table->vars[i].name, name) == 0) {
      return -2;
    }
  }

  /* 如果未指定大小，使用类型默认大小 */
  if (size == 0 && type < sizeof(type_sizes) / sizeof(type_sizes[0])) {
    size = type_sizes[type];
  }

  /* 检查数据区空间是否足够 */
  if (table->data_offset + size > sizeof(table->var_data)) return -1;

  /* 分配变量槽位 */
  PlcVariable* var = &table->vars[table->count];
  var->name = name;
  var->type = type;
  var->attr = attr;
  var->size = size;
  var->offset = table->data_offset;
  var->data = &table->var_data[table->data_offset];
  var->forced = false;
  var->quality = 0; /* good */
  var->comment = comment;

  /* 清零变量数据区域 */
  memset(var->data, 0, size);

  /* 碰撞分配：推进偏移量 */
  table->data_offset += size;
  table->count++;

  return (int)(table->count - 1);
}

PlcVariable* plc_var_find(PlcVarTable* table, const char* name) {
  if (table == NULL || name == NULL) return NULL;

  /* 线性搜索 */
  for (uint32_t i = 0; i < table->count; i++) {
    if (table->vars[i].name != NULL && strcmp(table->vars[i].name, name) == 0) {
      return &table->vars[i];
    }
  }
  return NULL;
}

PlcVariable* plc_var_find_by_index(PlcVarTable* table, uint32_t index) {
  if (table == NULL || index >= table->count) return NULL;
  return &table->vars[index];
}

int plc_var_read(PlcVarTable* table, const char* name, void* dest, uint32_t max_size) {
  if (table == NULL || name == NULL || dest == NULL) return 0;

  PlcVariable* var = plc_var_find(table, name);
  if (var == NULL || var->data == NULL) return 0;

  /* 计算实际拷贝大小，取缓冲区和变量大小中的较小值 */
  uint32_t copy_size = var->size < max_size ? var->size : max_size;
  memcpy(dest, var->data, copy_size);

  return (int)copy_size;
}

int plc_var_write(PlcVarTable* table, const char* name, const void* src, uint32_t size) {
  if (table == NULL || name == NULL || src == NULL) return -1;

  PlcVariable* var = plc_var_find(table, name);
  if (var == NULL || var->data == NULL) return -1;

  /* 如果变量被强制，拒绝写入 */
  if (var->forced) return -1;

  /* 大小检查：不允许写入超过变量大小的数据 */
  uint32_t copy_size = size < var->size ? size : var->size;
  memcpy(var->data, src, copy_size);

  return 0;
}

int plc_var_force(PlcVarTable* table, const char* name, const void* value) {
  if (table == NULL || name == NULL || value == NULL) return -1;

  PlcVariable* var = plc_var_find(table, name);
  if (var == NULL || var->data == NULL) return -1;

  /* 写入强制值 */
  memcpy(var->data, value, var->size);
  var->forced = true;

  return 0;
}

int plc_var_unforce(PlcVarTable* table, const char* name) {
  if (table == NULL || name == NULL) return -1;

  PlcVariable* var = plc_var_find(table, name);
  if (var == NULL) return -1;

  var->forced = false;
  return 0;
}

uint32_t plc_var_count(PlcVarTable* table) {
  if (table == NULL) return 0;
  return table->count;
}

const char* plc_var_type_name(VarType type) {
  if (type < sizeof(type_names) / sizeof(type_names[0])) {
    return type_names[type];
  }
  return "UNKNOWN";
}

uint32_t plc_var_type_size(VarType type) {
  if (type < sizeof(type_sizes) / sizeof(type_sizes[0])) {
    return type_sizes[type];
  }
  return 0;
}
