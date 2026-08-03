/**
 * edrv_loopback.h - 内存回环以太网驱动（测试用）
 *
 * 两个配对端口 A/B 模拟一对直连网卡：
 *   A.send(帧) → 帧进入 B 的接收队列（按 B 的接收过滤器过滤）
 *   B.poll()    → 取出队列帧并调用 B 注册的接收回调
 * 双向同理。另提供 inject：直接把帧投入本端口队列，模拟对端发来。
 *
 * 用于 MN/CN 在单进程内通过真实 SlEdrv 抽象层互通测试。
 */

#ifndef SL_EDRV_LOOPBACK_H
#define SL_EDRV_LOOPBACK_H

#include "smartlink/smartlink.h"
#include "smartlink/edrv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SL_LOOP_QUEUE_CAP 16   /* 每端口接收队列容量 */
#define SL_HUB_MAX_PORTS 8     /* hub 最大端口数 */

typedef struct SlEdrvHub SlEdrvHub;

typedef struct SlEdrvLoopbackPort
{
  SlEdrv edrv;                  /* 公开的驱动接口 */

  struct SlEdrvLoopbackPort* peer;  /* 对端端口（send 的目的） */
  SlEdrvHub* hub;               /* 非 NULL 表示 hub 模式（fanout） */
  int hubIndex;                  /* 自身在 hub 中的索引 */

  uint8_t mac[6];                /* 本端口 MAC */
  uint16_t rxFilter;             /* 接收过滤器掩码 SL_RX_FILTER_* */

  SlEdrvRxCallback rxCb;        /* 接收回调 */
  void* rxCtx;

  uint8_t  queueData[SL_LOOP_QUEUE_CAP][SL_ETH_FRAME_MAX];
  uint16_t queueLen[SL_LOOP_QUEUE_CAP];
  int      head;
  int      tail;
  int      count;
} SlEdrvLoopbackPort;

/**
 * 创建一对回环端口，互相配对。
 * @param a    端口 A 实例（内存由调用方提供）
 * @param b    端口 B 实例
 * @param macA 端口 A 的 MAC
 * @param macB 端口 B 的 MAC
 */
int sl_edrv_loopback_create(SlEdrvLoopbackPort* a, SlEdrvLoopbackPort* b,
                             const uint8_t macA[6], const uint8_t macB[6]);

/**
 * 向端口注入一帧（模拟对端发来的帧），进入其接收队列，等待 poll 处理。
 */
int sl_edrv_loopback_inject(SlEdrvLoopbackPort* p, const uint8_t* frame,
                             uint16_t len);

/**
 * 直接从端口接收队列取出一帧（不触发回调）。
 * @return 0 成功取到；SL_ERR_TIMEOUT 队列为空
 */
int sl_edrv_loopback_pop(SlEdrvLoopbackPort* p, uint8_t* frame,
                          uint16_t cap, uint16_t* len);

/**
 * hub（fanout）模式：模拟共享以太网介质。
 * 任一端口 send 会把帧广播到所有其他端口（按各端口接收过滤器过滤）。
 * 用于 MN 多节点拓扑测试（1 台 MN 挂多台 CN）。
 */
struct SlEdrvHub
{
  SlEdrvLoopbackPort ports[SL_HUB_MAX_PORTS];
  int count;
};

/**
 * 创建 hub：count 个端口共享介质。
 * @param hub   hub 实例（内存由调用方提供）
 * @param count 端口数 (1..SL_HUB_MAX_PORTS)
 * @param macs  各端口 MAC 数组（[count][6]）
 */
int sl_edrv_loopback_hub_create(SlEdrvHub* hub, int count,
                                 const uint8_t macs[][6]);

#ifdef __cplusplus
}
#endif

#endif /* SL_EDRV_LOOPBACK_H */
