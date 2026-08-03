/**
 * edrv_loopback.c - 内存回环以太网驱动实现（测试用）
 *
 * 实现完整的 SlEdrv 接口，模拟两台直连网卡：
 *   send → 对端接收队列（按对端接收过滤器过滤）
 *   poll → 取出队列帧并调用本端口接收回调
 */

#include <string.h>

#include "edrv_loopback.h"
#include "smartlink/sl_core.h"

/* ========== 内部辅助 ========== */

/* 依据目标 MAC 判断帧类别（对应 SL_RX_FILTER_* 位） */
static uint16_t classify_frame(const uint8_t* frame)
{
  const uint8_t* dst = frame;
  uint64_t mac;
  int i;

  if (dst[0] == 0xFF && dst[1] == 0xFF && dst[2] == 0xFF &&
      dst[3] == 0xFF && dst[4] == 0xFF && dst[5] == 0xFF) {
    return SL_RX_FILTER_BROADCAST;
  }

  mac = 0;
  for (i = 0; i < 6; i++) {
    mac = (mac << 8) | dst[i];
  }
  if (mac == SL_MCAST_SOC)  return SL_RX_FILTER_SOC;
  if (mac == SL_MCAST_PRES) return SL_RX_FILTER_PRES;
  if (mac == SL_MCAST_SOA)  return SL_RX_FILTER_SOA;
  if (mac == SL_MCAST_ASND) return SL_RX_FILTER_ASND;
  if (mac == SL_MCAST_AMNI) return SL_RX_FILTER_AMNI;

  return SL_RX_FILTER_UNICAST;
}

static int loopback_enqueue(SlEdrvLoopbackPort* p, const uint8_t* frame,
                            uint16_t len)
{
  if (len > SL_ETH_FRAME_MAX) {
    return SL_ERR_INVALID_PARAM;
  }
  if (p->count == SL_LOOP_QUEUE_CAP) {
    return SL_ERR_BUSY;
  }
  memcpy(p->queueData[p->tail], frame, len);
  p->queueLen[p->tail] = len;
  p->tail = (p->tail + 1) % SL_LOOP_QUEUE_CAP;
  p->count++;
  return SL_ERR_OK;
}

/* ========== SlEdrv 接口实现 ========== */

static int loopbackInit(SlEdrv* self)
{
  (void)self;
  return SL_ERR_OK;
}

static int loopbackExit(SlEdrv* self)
{
  (void)self;
  return SL_ERR_OK;
}

static int loopbackGetMac(SlEdrv* self, uint8_t mac[6])
{
  SlEdrvLoopbackPort* p = (SlEdrvLoopbackPort*)self->priv;

  if (p == NULL || mac == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  memcpy(mac, p->mac, 6);
  return SL_ERR_OK;
}

static int loopbackSend(SlEdrv* self, const uint8_t* frame, uint16_t len)
{
  SlEdrvLoopbackPort* p = (SlEdrvLoopbackPort*)self->priv;
  SlEdrvLoopbackPort* peer;
  uint16_t category;
  int i;

  if (p == NULL || frame == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* hub 模式：广播到除自己外的所有端口 */
  if (p->hub != NULL) {
    category = classify_frame(frame);
    for (i = 0; i < p->hub->count; i++) {
      SlEdrvLoopbackPort* q = &p->hub->ports[i];

      if (q == p) {
        continue;
      }
      if ((q->rxFilter & category) != 0) {
        if (loopback_enqueue(q, frame, len) != SL_ERR_OK) {
          return SL_ERR_BUSY;
        }
      }
    }
    return SL_ERR_OK;
  }

  peer = p->peer;
  if (peer == NULL) {
    return SL_ERR_LINK_DOWN;
  }

  /* 按对端接收过滤器过滤 */
  category = classify_frame(frame);
  if ((peer->rxFilter & category) == 0) {
    return SL_ERR_OK;   /* 被过滤丢弃，发送端视为成功 */
  }
  return loopback_enqueue(peer, frame, len);
}

static int loopbackSetRxFilter(SlEdrv* self, uint16_t filterMask)
{
  SlEdrvLoopbackPort* p = (SlEdrvLoopbackPort*)self->priv;

  if (p == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  p->rxFilter = filterMask;
  return SL_ERR_OK;
}

static int loopbackSetRxCallback(SlEdrv* self, SlEdrvRxCallback cb, void* ctx)
{
  SlEdrvLoopbackPort* p = (SlEdrvLoopbackPort*)self->priv;

  if (p == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  p->rxCb = cb;
  p->rxCtx = ctx;
  return SL_ERR_OK;
}

static int loopbackLinkState(SlEdrv* self, SlLinkState* state)
{
  (void)self;
  if (state == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  *state = SL_LINK_UP;
  return SL_ERR_OK;
}

static int loopbackPoll(SlEdrv* self, uint32_t timeoutMs)
{
  SlEdrvLoopbackPort* p = (SlEdrvLoopbackPort*)self->priv;
  uint8_t buf[SL_ETH_FRAME_MAX];
  uint16_t len;
  int n = 0;

  (void)timeoutMs;

  if (p == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  while (p->count > 0) {
    len = p->queueLen[p->head];
    memcpy(buf, p->queueData[p->head], len);
    p->head = (p->head + 1) % SL_LOOP_QUEUE_CAP;
    p->count--;
    if (p->rxCb != NULL) {
      p->rxCb(buf, len, p->rxCtx);
    }
    n++;
  }
  return (n > 0) ? SL_ERR_OK : SL_ERR_TIMEOUT;
}

/* ========== 公共接口 ========== */

int sl_edrv_loopback_create(SlEdrvLoopbackPort* a, SlEdrvLoopbackPort* b,
                             const uint8_t macA[6], const uint8_t macB[6])
{
  if (a == NULL || b == NULL || macA == NULL || macB == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  memset(a, 0, sizeof(*a));
  memset(b, 0, sizeof(*b));

  a->peer = b;
  b->peer = a;
  memcpy(a->mac, macA, 6);
  memcpy(b->mac, macB, 6);
  a->rxFilter = SL_RX_FILTER_ALL;
  b->rxFilter = SL_RX_FILTER_ALL;

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

  return SL_ERR_OK;
}

int sl_edrv_loopback_inject(SlEdrvLoopbackPort* p, const uint8_t* frame,
                             uint16_t len)
{
  if (p == NULL || frame == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  return loopback_enqueue(p, frame, len);
}

int sl_edrv_loopback_pop(SlEdrvLoopbackPort* p, uint8_t* frame,
                          uint16_t cap, uint16_t* len)
{
  if (p == NULL || frame == NULL || len == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (p->count == 0) {
    return SL_ERR_TIMEOUT;
  }
  if (cap < p->queueLen[p->head]) {
    return SL_ERR_INVALID_PARAM;
  }
  memcpy(frame, p->queueData[p->head], p->queueLen[p->head]);
  *len = p->queueLen[p->head];
  p->head = (p->head + 1) % SL_LOOP_QUEUE_CAP;
  p->count--;
  return SL_ERR_OK;
}

int sl_edrv_loopback_hub_create(SlEdrvHub* hub, int count,
                                 const uint8_t macs[][6])
{
  int i;
  SlEdrvLoopbackPort* p;

  if (hub == NULL || macs == NULL || count < 1 || count > SL_HUB_MAX_PORTS) {
    return SL_ERR_INVALID_PARAM;
  }

  memset(hub, 0, sizeof(*hub));
  hub->count = count;

  for (i = 0; i < count; i++) {
    p = &hub->ports[i];
    p->peer = NULL;
    p->hub = hub;
    p->hubIndex = i;
    memcpy(p->mac, macs[i], 6);
    p->rxFilter = SL_RX_FILTER_ALL;

    p->edrv.priv = p;
    p->edrv.init = loopbackInit;
    p->edrv.exit = loopbackExit;
    p->edrv.getMacAddr = loopbackGetMac;
    p->edrv.send = loopbackSend;
    p->edrv.setRxFilter = loopbackSetRxFilter;
    p->edrv.setRxCallback = loopbackSetRxCallback;
    p->edrv.linkState = loopbackLinkState;
    p->edrv.poll = loopbackPoll;
  }
  return SL_ERR_OK;
}
