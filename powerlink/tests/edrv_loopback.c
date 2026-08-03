/**
 * edrv_loopback.c - 内存回环以太网驱动实现（测试用）
 *
 * 实现完整的 PlkEdrv 接口，模拟两台直连网卡：
 *   send → 对端接收队列（按对端接收过滤器过滤）
 *   poll → 取出队列帧并调用本端口接收回调
 */

#include <string.h>

#include "edrv_loopback.h"
#include "plk/plk_core.h"

/* ========== 内部辅助 ========== */

/* 依据目标 MAC 判断帧类别（对应 PLK_RX_FILTER_* 位） */
static uint16_t classify_frame(const uint8_t* frame)
{
  const uint8_t* dst = frame;
  uint64_t mac;
  int i;

  if (dst[0] == 0xFF && dst[1] == 0xFF && dst[2] == 0xFF &&
      dst[3] == 0xFF && dst[4] == 0xFF && dst[5] == 0xFF) {
    return PLK_RX_FILTER_BROADCAST;
  }

  mac = 0;
  for (i = 0; i < 6; i++) {
    mac = (mac << 8) | dst[i];
  }
  if (mac == PLK_MCAST_SOC)  return PLK_RX_FILTER_SOC;
  if (mac == PLK_MCAST_PRES) return PLK_RX_FILTER_PRES;
  if (mac == PLK_MCAST_SOA)  return PLK_RX_FILTER_SOA;
  if (mac == PLK_MCAST_ASND) return PLK_RX_FILTER_ASND;
  if (mac == PLK_MCAST_AMNI) return PLK_RX_FILTER_AMNI;

  return PLK_RX_FILTER_UNICAST;
}

static int loopback_enqueue(PlkEdrvLoopbackPort* p, const uint8_t* frame,
                            uint16_t len)
{
  if (len > PLK_ETH_FRAME_MAX) {
    return PLK_ERR_INVALID_PARAM;
  }
  if (p->count == PLK_LOOP_QUEUE_CAP) {
    return PLK_ERR_BUSY;
  }
  memcpy(p->queueData[p->tail], frame, len);
  p->queueLen[p->tail] = len;
  p->tail = (p->tail + 1) % PLK_LOOP_QUEUE_CAP;
  p->count++;
  return PLK_ERR_OK;
}

/* ========== PlkEdrv 接口实现 ========== */

static int loopbackInit(PlkEdrv* self)
{
  (void)self;
  return PLK_ERR_OK;
}

static int loopbackExit(PlkEdrv* self)
{
  (void)self;
  return PLK_ERR_OK;
}

static int loopbackGetMac(PlkEdrv* self, uint8_t mac[6])
{
  PlkEdrvLoopbackPort* p = (PlkEdrvLoopbackPort*)self->priv;

  if (p == NULL || mac == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  memcpy(mac, p->mac, 6);
  return PLK_ERR_OK;
}

static int loopbackSend(PlkEdrv* self, const uint8_t* frame, uint16_t len)
{
  PlkEdrvLoopbackPort* p = (PlkEdrvLoopbackPort*)self->priv;
  PlkEdrvLoopbackPort* peer;
  uint16_t category;

  if (p == NULL || frame == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  peer = p->peer;
  if (peer == NULL) {
    return PLK_ERR_LINK_DOWN;
  }

  /* 按对端接收过滤器过滤 */
  category = classify_frame(frame);
  if ((peer->rxFilter & category) == 0) {
    return PLK_ERR_OK;   /* 被过滤丢弃，发送端视为成功 */
  }
  return loopback_enqueue(peer, frame, len);
}

static int loopbackSetRxFilter(PlkEdrv* self, uint16_t filterMask)
{
  PlkEdrvLoopbackPort* p = (PlkEdrvLoopbackPort*)self->priv;

  if (p == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  p->rxFilter = filterMask;
  return PLK_ERR_OK;
}

static int loopbackSetRxCallback(PlkEdrv* self, PlkEdrvRxCallback cb, void* ctx)
{
  PlkEdrvLoopbackPort* p = (PlkEdrvLoopbackPort*)self->priv;

  if (p == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  p->rxCb = cb;
  p->rxCtx = ctx;
  return PLK_ERR_OK;
}

static int loopbackLinkState(PlkEdrv* self, PlkLinkState* state)
{
  (void)self;
  if (state == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  *state = PLK_LINK_UP;
  return PLK_ERR_OK;
}

static int loopbackPoll(PlkEdrv* self, uint32_t timeoutMs)
{
  PlkEdrvLoopbackPort* p = (PlkEdrvLoopbackPort*)self->priv;
  uint8_t buf[PLK_ETH_FRAME_MAX];
  uint16_t len;
  int n = 0;

  (void)timeoutMs;

  if (p == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  while (p->count > 0) {
    len = p->queueLen[p->head];
    memcpy(buf, p->queueData[p->head], len);
    p->head = (p->head + 1) % PLK_LOOP_QUEUE_CAP;
    p->count--;
    if (p->rxCb != NULL) {
      p->rxCb(buf, len, p->rxCtx);
    }
    n++;
  }
  return (n > 0) ? PLK_ERR_OK : PLK_ERR_TIMEOUT;
}

/* ========== 公共接口 ========== */

int plk_edrv_loopback_create(PlkEdrvLoopbackPort* a, PlkEdrvLoopbackPort* b,
                             const uint8_t macA[6], const uint8_t macB[6])
{
  if (a == NULL || b == NULL || macA == NULL || macB == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  memset(a, 0, sizeof(*a));
  memset(b, 0, sizeof(*b));

  a->peer = b;
  b->peer = a;
  memcpy(a->mac, macA, 6);
  memcpy(b->mac, macB, 6);
  a->rxFilter = PLK_RX_FILTER_ALL;
  b->rxFilter = PLK_RX_FILTER_ALL;

  a->edrv.priv = a;
  a->edrv.init = loopbackInit;
  a->edrv.exit = loopbackExit;
  a->edrv.getMacAddr = loopbackGetMac;
  a->edrv.send = loopbackSend;
  a->edrv.setRxFilter = loopbackSetRxFilter;
  a->edrv.setRxCallback = loopbackSetRxCallback;
  a->edrv.linkState = loopbackLinkState;
  a->edrv.poll = loopbackPoll;

  b->edrv.priv = b;
  b->edrv.init = loopbackInit;
  b->edrv.exit = loopbackExit;
  b->edrv.getMacAddr = loopbackGetMac;
  b->edrv.send = loopbackSend;
  b->edrv.setRxFilter = loopbackSetRxFilter;
  b->edrv.setRxCallback = loopbackSetRxCallback;
  b->edrv.linkState = loopbackLinkState;
  b->edrv.poll = loopbackPoll;

  return PLK_ERR_OK;
}

int plk_edrv_loopback_inject(PlkEdrvLoopbackPort* p, const uint8_t* frame,
                             uint16_t len)
{
  if (p == NULL || frame == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  return loopback_enqueue(p, frame, len);
}

int plk_edrv_loopback_pop(PlkEdrvLoopbackPort* p, uint8_t* frame,
                          uint16_t cap, uint16_t* len)
{
  if (p == NULL || frame == NULL || len == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  if (p->count == 0) {
    return PLK_ERR_TIMEOUT;
  }
  if (cap < p->queueLen[p->head]) {
    return PLK_ERR_INVALID_PARAM;
  }
  memcpy(frame, p->queueData[p->head], p->queueLen[p->head]);
  *len = p->queueLen[p->head];
  p->head = (p->head + 1) % PLK_LOOP_QUEUE_CAP;
  p->count--;
  return PLK_ERR_OK;
}
