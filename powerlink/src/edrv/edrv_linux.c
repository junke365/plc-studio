/**
 * edrv_linux.c - Linux 以太网驱动（AF_PACKET 原始套接字）
 *
 * 适用于 ARM/x86 全系列 Linux 平台，与 STM32 一样作为协议核心的网卡驱动。
 * 架构：独立收包线程（recvfrom 阻塞轮询）+ 环形队列，
 *       协议层周期性调用 poll() 取出帧并按接收过滤器转发给回调。
 *
 * 设备选择：环境变量 PLK_EDRV_DEVICE 指定网卡名（如 eth0 / ens33），
 *           未指定时默认 "eth0"。
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "plk/edrv.h"

#define PLK_EDRV_QUEUE_CAP 32   /* 收包环形队列容量（帧数） */
#define PLK_EDRV_MAX_FRAME 1522 /* 单帧最大长度（含 14 字节以太网头） */
#define PLK_EDRV_DEVICE_MAX 64  /* 网卡名缓冲 */
#define PLK_EDRV_RECV_TIMEOUT_MS 100 /* recvfrom 阻塞超时（用于退出检查） */

typedef struct
{
  uint8_t  data[PLK_EDRV_MAX_FRAME];
  uint16_t len;
} PlkEdrvRxSlot;

typedef struct PlkEdrvLinux
{
  int               sock;          /* AF_PACKET 原始套接字 */
  int               ifIndex;       /* 网卡接口索引 */
  char              deviceName[PLK_EDRV_DEVICE_MAX];
  uint8_t           mac[6];        /* 本机 MAC */
  uint16_t          rxFilterMask;  /* 当前接收过滤器 */
  PlkEdrvRxCallback rxCallback;    /* 接收回调 */
  void*             rxCtx;         /* 回调上下文 */

  pthread_t         rxThread;      /* 收包线程 */
  pthread_mutex_t   lock;          /* 保护队列/过滤器 */
  pthread_cond_t    cond;          /* 有新帧时通知 poll */
  volatile int      running;       /* 运行标志 */

  PlkEdrvRxSlot     rxQueue[PLK_EDRV_QUEUE_CAP];
  int               rxHead;        /* 读位置 */
  int               rxTail;        /* 写位置 */
  int               rxCount;       /* 队内帧数 */
} PlkEdrvLinux;

static PlkEdrv s_edrv;
static PlkEdrvLinux s_priv;

/* ========== 内部辅助 ========== */

/* 帧入队（满则丢弃最旧帧，保留最新数据） */
static void edrvQueuePush(PlkEdrvLinux* d, const uint8_t* data, uint16_t len)
{
  PlkEdrvRxSlot* slot;

  if (d->rxCount == PLK_EDRV_QUEUE_CAP) {
    d->rxTail = (d->rxTail + 1) % PLK_EDRV_QUEUE_CAP;
    d->rxCount--;
  }
  slot = &d->rxQueue[d->rxTail];
  memcpy(slot->data, data, len);
  slot->len = len;
  d->rxTail = (d->rxTail + 1) % PLK_EDRV_QUEUE_CAP;
  d->rxCount++;
}

/* 帧出队，返回是否成功 */
static bool edrvQueuePop(PlkEdrvLinux* d, PlkEdrvRxSlot* out)
{
  PlkEdrvRxSlot* slot;

  if (d->rxCount == 0) {
    return false;
  }
  slot = &d->rxQueue[d->rxHead];
  memcpy(out, slot, sizeof(*out));
  d->rxHead = (d->rxHead + 1) % PLK_EDRV_QUEUE_CAP;
  d->rxCount--;
  return true;
}

/* 按接收过滤器掩码匹配目的 MAC */
static bool edrvMatchFilter(const PlkEdrvLinux* d, const uint8_t* frame, uint16_t len)
{
  static const uint8_t mcast[5][6] = {
    {0x01, 0x11, 0x1E, 0x00, 0x00, 0x01}, /* SoC  */
    {0x01, 0x11, 0x1E, 0x00, 0x00, 0x02}, /* PRes */
    {0x01, 0x11, 0x1E, 0x00, 0x00, 0x03}, /* SoA  */
    {0x01, 0x11, 0x1E, 0x00, 0x00, 0x04}, /* ASnd */
    {0x01, 0x11, 0x1E, 0x00, 0x00, 0x05}, /* AMNI */
  };
  uint16_t mask;
  uint16_t i;

  if (len < 14) {
    return false;
  }
  mask = d->rxFilterMask;
  if (mask == PLK_RX_FILTER_ALL) {
    return true;
  }
  for (i = 0; i < 5; i++) {
    if (memcmp(frame, mcast[i], 6) == 0) {
      return (mask & (1u << i)) != 0;
    }
  }
  if (frame[0] == 0xFF && frame[1] == 0xFF && frame[2] == 0xFF &&
      frame[3] == 0xFF && frame[4] == 0xFF && frame[5] == 0xFF) {
    return (mask & PLK_RX_FILTER_BROADCAST) != 0;
  }
  if (memcmp(frame, d->mac, 6) == 0) {
    return (mask & PLK_RX_FILTER_UNICAST) != 0;
  }
  return false;
}

/* ========== 收包线程 ========== */

static void* edrvRxThread(void* arg)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)arg;
  struct sockaddr_ll sll;
  socklen_t sllLen = sizeof(sll);
  uint8_t buf[PLK_EDRV_MAX_FRAME];
  ssize_t n;

  while (d->running) {
    n = recvfrom(d->sock, buf, sizeof(buf), 0, (struct sockaddr*)&sll, &sllLen);
    if (n <= 0) {
      if (!d->running) {
        break;
      }
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue; /* 超时或信号打断，继续检查运行标志 */
      }
      break; /* 致命错误 */
    }
    if (n >= 14) {
      pthread_mutex_lock(&d->lock);
      edrvQueuePush(d, buf, (uint16_t)n);
      pthread_mutex_unlock(&d->lock);
      pthread_cond_signal(&d->cond);
    }
  }
  return NULL;
}

/* ========== PlkEdrv 接口实现 ========== */

static int edrvLinuxInit(PlkEdrv* self)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;
  struct ifreq ifr;
  struct sockaddr_ll sll;
  struct timeval tv;
  const char* devName;
  int ret;

  if (d == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  memset(d, 0, sizeof(*d));
  d->rxFilterMask = PLK_RX_FILTER_ALL;
  d->sock = -1;

  devName = getenv("PLK_EDRV_DEVICE");
  if (devName == NULL || devName[0] == '\0') {
    devName = "eth0";
  }
  strncpy(d->deviceName, devName, sizeof(d->deviceName) - 1);
  d->deviceName[sizeof(d->deviceName) - 1] = '\0';

  d->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (d->sock < 0) {
    return PLK_ERR_LINK_DOWN;
  }
  d->ifIndex = (int)if_nametoindex(d->deviceName);
  if (d->ifIndex == 0) {
    close(d->sock);
    d->sock = -1;
    return PLK_ERR_LINK_DOWN;
  }
  memset(&sll, 0, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = d->ifIndex;
  if (bind(d->sock, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
    close(d->sock);
    d->sock = -1;
    return PLK_ERR_LINK_DOWN;
  }
  /* 收包阻塞超时 100ms，便于线程退出检查 */
  tv.tv_sec = 0;
  tv.tv_usec = PLK_EDRV_RECV_TIMEOUT_MS * 1000;
  setsockopt(d->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  /* 读取本机 MAC */
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, d->deviceName, IFNAMSIZ - 1);
  if (ioctl(d->sock, SIOCGIFHWADDR, &ifr) == 0) {
    memcpy(d->mac, ifr.ifr_hwaddr.sa_data, 6);
  }

  pthread_mutex_init(&d->lock, NULL);
  pthread_cond_init(&d->cond, NULL);
  d->running = 1;
  ret = pthread_create(&d->rxThread, NULL, edrvRxThread, d);
  if (ret != 0) {
    d->running = 0;
    pthread_cond_destroy(&d->cond);
    pthread_mutex_destroy(&d->lock);
    close(d->sock);
    d->sock = -1;
    return PLK_ERR_NO_MEMORY;
  }
  return PLK_ERR_OK;
}

static int edrvLinuxExit(PlkEdrv* self)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;

  if (d == NULL || d->sock < 0) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  d->running = 0;
  shutdown(d->sock, SHUT_RDWR);
  pthread_join(d->rxThread, NULL);
  close(d->sock);
  pthread_cond_destroy(&d->cond);
  pthread_mutex_destroy(&d->lock);
  memset(d, 0, sizeof(*d));
  return PLK_ERR_OK;
}

static int edrvLinuxGetMacAddr(PlkEdrv* self, uint8_t mac[6])
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;

  if (d == NULL || mac == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  memcpy(mac, d->mac, 6);
  return PLK_ERR_OK;
}

static int edrvLinuxSend(PlkEdrv* self, const uint8_t* frame, uint16_t len)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;
  struct sockaddr_ll sll;
  ssize_t sent;

  if (d == NULL || d->sock < 0) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  if (frame == NULL || len == 0 || len > PLK_EDRV_MAX_FRAME) {
    return PLK_ERR_INVALID_PARAM;
  }
  memset(&sll, 0, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = d->ifIndex;
  sent = sendto(d->sock, frame, len, 0, (struct sockaddr*)&sll, sizeof(sll));
  if (sent < 0 || sent != (ssize_t)len) {
    return PLK_ERR_LINK_DOWN;
  }
  return PLK_ERR_OK;
}

static int edrvLinuxSetRxFilter(PlkEdrv* self, uint16_t filterMask)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;

  if (d == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  pthread_mutex_lock(&d->lock);
  d->rxFilterMask = filterMask;
  pthread_mutex_unlock(&d->lock);
  return PLK_ERR_OK;
}

static int edrvLinuxSetRxCallback(PlkEdrv* self, PlkEdrvRxCallback cb, void* ctx)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;

  if (d == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  d->rxCallback = cb;
  d->rxCtx = ctx;
  return PLK_ERR_OK;
}

static int edrvLinuxLinkState(PlkEdrv* self, PlkLinkState* state)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;
  struct ifreq ifr;
  int ret;

  if (d == NULL || state == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  *state = PLK_LINK_DOWN;
  if (d->sock < 0) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, d->deviceName, IFNAMSIZ - 1);
  ret = ioctl(d->sock, SIOCGIFFLAGS, &ifr);
  if (ret == 0 && (ifr.ifr_flags & IFF_UP) != 0) {
    *state = PLK_LINK_UP;
  }
  return PLK_ERR_OK;
}

static int edrvLinuxPoll(PlkEdrv* self, uint32_t timeoutMs)
{
  PlkEdrvLinux* d = (PlkEdrvLinux*)self->priv;
  PlkEdrvRxSlot slot;
  struct timespec ts;
  int ret;

  if (d == NULL || d->sock < 0) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  if (timeoutMs > 0) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (long)(timeoutMs % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&d->lock);
    while (d->rxCount == 0 && d->running) {
      ret = pthread_cond_timedwait(&d->cond, &d->lock, &ts);
      if (ret != 0) {
        break;
      }
    }
    pthread_mutex_unlock(&d->lock);
  }
  for (;;) {
    pthread_mutex_lock(&d->lock);
    if (!edrvQueuePop(d, &slot)) {
      pthread_mutex_unlock(&d->lock);
      break;
    }
    pthread_mutex_unlock(&d->lock);
    if (d->rxCallback != NULL &&
        edrvMatchFilter(d, slot.data, slot.len)) {
      d->rxCallback(slot.data, slot.len, d->rxCtx);
    }
  }
  return PLK_ERR_OK;
}

/* ========== 工厂 ========== */

/**
 * 获取 Linux 驱动单例（静态存储，vtable 已填好）。
 * 使用前需调用 init，退出时调用 exit。
 */
PlkEdrv* plk_edrv_linux_get(void)
{
  s_edrv.priv = &s_priv;
  s_edrv.init = edrvLinuxInit;
  s_edrv.exit = edrvLinuxExit;
  s_edrv.getMacAddr = edrvLinuxGetMacAddr;
  s_edrv.send = edrvLinuxSend;
  s_edrv.setRxFilter = edrvLinuxSetRxFilter;
  s_edrv.setRxCallback = edrvLinuxSetRxCallback;
  s_edrv.linkState = edrvLinuxLinkState;
  s_edrv.poll = edrvLinuxPoll;
  return &s_edrv;
}
