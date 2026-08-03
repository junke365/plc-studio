/**
 * plk_edrv.c - 以太网驱动注册表
 *
 * 全局单例驱动表：驱动实例通过 plk_edrv_register 注册，
 * 协议核心（如 SDO）通过 plk_edrv_get 获取 MAC 地址等信息。
 */

#include "plk/edrv.h"

static PlkEdrv* s_edrv = NULL;

int plk_edrv_register(PlkEdrv* edrv)
{
  if (edrv == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  s_edrv = edrv;
  return PLK_ERR_OK;
}

PlkEdrv* plk_edrv_get(void)
{
  return s_edrv;
}
