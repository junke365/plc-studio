/**
 * od.c - 对象字典实现
 *
 * 采用链表存储条目，支持运行时动态添加（智能从站变量 OD 动态映射所需）。
 * 所有读写操作先做访问权限与长度校验，再访问数据区。
 */

#include "plk/od.h"

void plk_od_init(PlkOd* od)
{
  if (od == NULL) {
    return;
  }
  od->head = NULL;
  od->count = 0;
}

int plk_od_add(PlkOd* od, PlkOdEntry* entry)
{
  PlkOdEntry* cur;
  PlkOdEntry* prev;

  if (od == NULL || entry == NULL || entry->data == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  /* 已存在则更新内容（保留链表位置） */
  prev = NULL;
  for (cur = od->head; cur != NULL; cur = cur->next) {
    if (cur->index == entry->index && cur->subIndex == entry->subIndex) {
      cur->access = entry->access;
      cur->objectType = entry->objectType;
      cur->dataType = entry->dataType;
      cur->size = entry->size;
      cur->data = entry->data;
      cur->arraySize = entry->arraySize;
      cur->subEntries = entry->subEntries;
      return PLK_ERR_OK;
    }
    prev = cur;
  }

  /* 插入链表尾部 */
  entry->next = NULL;
  if (prev == NULL) {
    od->head = entry;
  } else {
    prev->next = entry;
  }
  od->count++;
  return PLK_ERR_OK;
}

int plk_od_find(PlkOd* od, uint16_t index, uint8_t subIndex, PlkOdEntry** out)
{
  PlkOdEntry* cur;

  if (od == NULL || out == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  for (cur = od->head; cur != NULL; cur = cur->next) {
    if (cur->index == index && cur->subIndex == subIndex) {
      *out = cur;
      return PLK_ERR_OK;
    }
  }

  *out = NULL;
  return subIndex == 0 ? PLK_ERR_OD_INDEX : PLK_ERR_OD_SUBINDEX;
}

int plk_od_read(PlkOd* od, uint16_t index, uint8_t subIndex,
                void* data, uint16_t* size)
{
  PlkOdEntry* entry;
  int ret;

  if (od == NULL || data == NULL || size == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  ret = plk_od_find(od, index, subIndex, &entry);
  if (ret != PLK_ERR_OK) {
    return ret;
  }

  if ((entry->access & PLK_OD_ACC_READ) == 0) {
    return PLK_ERR_OD_ACCESS;
  }

  if (*size < entry->size) {
    return PLK_ERR_OD_SIZE;
  }

  memcpy(data, entry->data, entry->size);
  *size = entry->size;
  return PLK_ERR_OK;
}

int plk_od_write(PlkOd* od, uint16_t index, uint8_t subIndex,
                 const void* data, uint16_t size)
{
  PlkOdEntry* entry;
  int ret;

  if (od == NULL || data == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  ret = plk_od_find(od, index, subIndex, &entry);
  if (ret != PLK_ERR_OK) {
    return ret;
  }

  if ((entry->access & PLK_OD_ACC_WRITE) == 0) {
    return PLK_ERR_OD_ACCESS;
  }

  if (size != entry->size) {
    return PLK_ERR_OD_SIZE;
  }

  memcpy(entry->data, data, size);
  return PLK_ERR_OK;
}

int plk_od_get_size(PlkOd* od, uint16_t index, uint8_t subIndex, uint16_t* size)
{
  PlkOdEntry* entry;
  int ret;

  if (od == NULL || size == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  ret = plk_od_find(od, index, subIndex, &entry);
  if (ret != PLK_ERR_OK) {
    return ret;
  }

  *size = entry->size;
  return PLK_ERR_OK;
}

uint16_t plk_od_type_size(uint8_t dataType)
{
  switch (dataType) {
    case PLK_OD_TYPE_BOOL:
    case PLK_OD_TYPE_INT8:
    case PLK_OD_TYPE_UINT8:
      return 1;
    case PLK_OD_TYPE_INT16:
    case PLK_OD_TYPE_UINT16:
      return 2;
    case PLK_OD_TYPE_INT24:
    case PLK_OD_TYPE_UINT24:
      return 3;
    case PLK_OD_TYPE_INT32:
    case PLK_OD_TYPE_UINT32:
    case PLK_OD_TYPE_REAL32:
    case PLK_OD_TYPE_TIME_OF_DAY:
    case PLK_OD_TYPE_TIME_DIFF:
      return 4;
    case PLK_OD_TYPE_INT40:
    case PLK_OD_TYPE_UINT40:
      return 5;
    case PLK_OD_TYPE_INT48:
    case PLK_OD_TYPE_UINT48:
      return 6;
    case PLK_OD_TYPE_INT56:
    case PLK_OD_TYPE_UINT56:
      return 7;
    case PLK_OD_TYPE_INT64:
    case PLK_OD_TYPE_UINT64:
    case PLK_OD_TYPE_REAL64:
      return 8;
    default:
      return 0;   /* 非定长类型 */
  }
}
