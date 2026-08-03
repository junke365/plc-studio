/**
 * edrv.h - 以太网驱动抽象层接口
 *
 * 平台无关的以太网驱动接口，为协议核心提供统一的帧收发能力。
 * 具体实现：
 *   - Win32 原型:  edrv_winpcap.c (WinPcap/Npcap 用户态驱动)
 *   - Linux 目标:  edrv_linux.c   (AF_PACKET 原始套接字, ARM/x86)
 *   - STM32 目标:  edrv_stm32_eth.c (HAL ETH + DMA, 与 EtherCAT 主站按 EtherType 分发)
 *
 * 帧收发方向：
 *   send      - 发送一个完整的以太网帧（含 14 字节以太网头，不含 CRC）
 *   recv/回调 - 接收完整以太网帧
 * 多播过滤：
 *   智能总线 使用 01:11:1E:00:00:0x 组播地址，驱动需支持按组播地址过滤，
 *   减少无关帧对 CPU 的打扰。
 */

#ifndef SL_EDRV_H
#define SL_EDRV_H

#include "smartlink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 接收过滤器掩码 ========== */

#define SL_RX_FILTER_SOC    0x0001  /* 多播 SoC  (01:11:1E:00:00:01) */
#define SL_RX_FILTER_PRES   0x0002  /* 多播 PRes (01:11:1E:00:00:02) */
#define SL_RX_FILTER_SOA    0x0004  /* 多播 SoA  (01:11:1E:00:00:03) */
#define SL_RX_FILTER_ASND   0x0008  /* 多播 ASnd (01:11:1E:00:00:04) */
#define SL_RX_FILTER_AMNI   0x0010  /* 多播 AMNI (01:11:1E:00:00:05) */
#define SL_RX_FILTER_BROADCAST 0x0020 /* 广播地址 */
#define SL_RX_FILTER_UNICAST   0x0040 /* 本机单播 */
#define SL_RX_FILTER_ALL        0xFFFF /* 全部接收 */

/* ========== 链路状态 ========== */

typedef enum
{
  SL_LINK_DOWN = 0,   /* 链路断开 */
  SL_LINK_UP,         /* 链路就绪 */
  SL_LINK_ANEG,       /* 自协商进行中 */
} SlLinkState;

/* ========== 接收回调 ========== */

/**
 * 接收帧回调：驱动收到 智能总线 相关帧时调用。
 * @param frame  完整以太网帧（不含 CRC）
 * @param len    帧长度
 * @param ctx    注册回调时传入的上下文
 * @return 0 成功
 */
typedef int (*SlEdrvRxCallback)(const uint8_t* frame, uint16_t len, void* ctx);

/* ========== 驱动接口 ========== */

typedef struct SlEdrv
{
  void* priv;   /* 驱动私有数据 */

  /* 初始化/反初始化 */
  int (*init)(struct SlEdrv* self);
  int (*exit)(struct SlEdrv* self);

  /* 读取本机 MAC 地址 */
  int (*getMacAddr)(struct SlEdrv* self, uint8_t mac[6]);

  /* 发送完整以太网帧（含 14 字节以太网头，不含 CRC） */
  int (*send)(struct SlEdrv* self, const uint8_t* frame, uint16_t len);

  /* 设置接收过滤器（SL_RX_FILTER_* 组合） */
  int (*setRxFilter)(struct SlEdrv* self, uint16_t filterMask);

  /* 注册接收回调 */
  int (*setRxCallback)(struct SlEdrv* self, SlEdrvRxCallback cb, void* ctx);

  /* 查询链路状态 */
  int (*linkState)(struct SlEdrv* self, SlLinkState* state);

  /* 轮询处理（WinPcap 用户态驱动需周期性调用以取出接收帧） */
  int (*poll)(struct SlEdrv* self, uint32_t timeoutMs);
} SlEdrv;

/* ========== 驱动注册表 ========== */

/**
 * 注册一个以太网驱动实例。
 * @param edrv  驱动接口指针（生命周期由调用方管理）
 */
int sl_edrv_register(SlEdrv* edrv);

/**
 * 获取当前活动的以太网驱动。
 * @return 驱动指针，未注册时返回 NULL
 */
SlEdrv* sl_edrv_get(void);

#ifdef __cplusplus
}
#endif

#endif /* SL_EDRV_H */
