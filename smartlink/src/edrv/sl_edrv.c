/**
 * sl_edrv.c - 以太网驱动注册表
 *
 * 全局单例驱动表：驱动实例通过 sl_edrv_register 注册，
 * 协议核心（如 SDO）通过 sl_edrv_get 获取 MAC 地址等信息。
 */

#include "smartlink/edrv.h"

static SlEdrv* s_edrv = NULL;

int sl_edrv_register(SlEdrv* edrv)
{
  if (edrv == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  s_edrv = edrv;
  return SL_ERR_OK;
}

SlEdrv* sl_edrv_get(void)
{
  return s_edrv;
}
