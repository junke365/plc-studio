/**
 * edrv_winpcap.c - WinPcap/Npcap 以太网驱动（Windows 用户态原型）
 *
 * 通过 WinPcap/Npcap 的 pcap 接口收发原始以太网帧，实现 SlEdrv 接口。
 * 架构：独立收包线程（pcap_next_ex 轮询）+ 环形队列，
 *       协议层周期性调用 poll() 取出帧并按接收过滤器转发给回调。
 *
 * 设备选择：环境变量 SL_EDRV_DEVICE 指定（匹配设备 name 或 description 子串），
 *           未指定时自动选择第一个非回环以太网设备。
 *
 * 注意：仅用于 Windows 原型验证，真实产品在 STM32/Linux 上使用对应驱动。
 */

#define _WIN32_WINNT 0x0601
#define WPCAP

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iphlpapi.h>

#include <pcap.h>

#include "smartlink/edrv.h"

#ifndef PCAP_IF_LOOPBACK
  #define PCAP_IF_LOOPBACK 0x00000001
#endif

#define SL_EDRV_QUEUE_CAP 32   /* 收包环形队列容量（帧数） */
#define SL_EDRV_MAX_FRAME 1522 /* 单帧最大长度（含 14 字节以太网头） */
#define SL_EDRV_DEVICE_MAX 128 /* 设备名缓冲 */

typedef struct
{
  uint8_t  data[SL_EDRV_MAX_FRAME];
  uint16_t len;
} SlEdrvRxSlot;

typedef struct SlEdrvWinPcap
{
  pcap_t*           pcap;          /* pcap 实例 */
  char              deviceName[SL_EDRV_DEVICE_MAX];
  uint8_t           mac[6];        /* 本机 MAC */
  uint16_t          rxFilterMask;  /* 当前接收过滤器 */
  SlEdrvRxCallback rxCallback;    /* 接收回调 */
  void*             rxCtx;         /* 回调上下文 */

  CRITICAL_SECTION  lock;          /* 保护队列/过滤器 */
  HANDLE            rxEvent;       /* 有新帧时置位（自动复位） */
  HANDLE            rxThread;      /* 收包线程 */
  volatile int      running;       /* 运行标志 */

  SlEdrvRxSlot     rxQueue[SL_EDRV_QUEUE_CAP];
  int               rxHead;        /* 读位置 */
  int               rxTail;        /* 写位置 */
  int               rxCount;       /* 队内帧数 */
} SlEdrvWinPcap;

static SlEdrv s_edrv;
static SlEdrvWinPcap s_priv;

/* ========== 内部辅助 ========== */

/* 从设备名（\Device\NPF_{GUID}）中解析本机 MAC（通过 IP Helper API） */
static int edrvGetMacByDevice(const char* devName, uint8_t mac[6])
{
  ULONG bufLen = 16384;
  int result = SL_ERR_NO_DATA;
  PIP_ADAPTER_ADDRESSES addrs;
  PIP_ADAPTER_ADDRESSES p;
  ULONG ret;

  addrs = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
  if (addrs == NULL) {
    return SL_ERR_NO_MEMORY;
  }
  ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addrs, &bufLen);
  if (ret == ERROR_BUFFER_OVERFLOW) {
    free(addrs);
    addrs = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
    if (addrs == NULL) {
      return SL_ERR_NO_MEMORY;
    }
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addrs, &bufLen);
  }
  if (ret == NO_ERROR) {
    for (p = addrs; p != NULL; p = p->Next) {
      if (p->IfType == IF_TYPE_ETHERNET_CSMACD &&
          strstr(devName, p->AdapterName) != NULL) {
        if (p->PhysicalAddressLength >= 6) {
          memcpy(mac, p->PhysicalAddress, 6);
          result = SL_ERR_OK;
          break;
        }
      }
    }
  }
  free(addrs);
  return result;
}

/* 打开 pcap 设备：优先环境变量指定，否则选第一个非回环设备 */
static int edrvOpenDevice(SlEdrvWinPcap* d, char* errbuf)
{
  pcap_if_t* alldevs = NULL;
  pcap_if_t* dev = NULL;
  pcap_if_t* fallback = NULL;
  pcap_if_t* it;
  const char* want;

  if (pcap_findalldevs(&alldevs, errbuf) != 0) {
    return SL_ERR_LINK_DOWN;
  }
  want = getenv("SL_EDRV_DEVICE");
  for (it = alldevs; it != NULL; it = it->next) {
    if ((it->flags & PCAP_IF_LOOPBACK) != 0) {
      continue;
    }
    if (fallback == NULL) {
      fallback = it;
    }
    if (want != NULL && want[0] != '\0') {
      if (strstr(it->name, want) != NULL ||
          (it->description != NULL && strstr(it->description, want) != NULL)) {
        dev = it;
        break;
      }
    }
  }
  if (dev == NULL) {
    dev = fallback;
  }
  if (dev == NULL) {
    pcap_freealldevs(alldevs);
    return SL_ERR_LINK_DOWN;
  }

  d->pcap = pcap_open_live(dev->name, 65535, 1, 1, errbuf);
  if (d->pcap == NULL) {
    pcap_freealldevs(alldevs);
    return SL_ERR_LINK_DOWN;
  }
  pcap_setmintocopy(d->pcap, 0);
  strncpy(d->deviceName, dev->name, sizeof(d->deviceName) - 1);
  d->deviceName[sizeof(d->deviceName) - 1] = '\0';
  edrvGetMacByDevice(dev->name, d->mac);
  pcap_freealldevs(alldevs);
  return SL_ERR_OK;
}

/* 帧入队（满则丢弃最旧帧，保留最新数据） */
static void edrvQueuePush(SlEdrvWinPcap* d, const uint8_t* data, uint16_t len)
{
  SlEdrvRxSlot* slot;

  if (d->rxCount == SL_EDRV_QUEUE_CAP) {
    d->rxTail = (d->rxTail + 1) % SL_EDRV_QUEUE_CAP;
    d->rxCount--;
  }
  slot = &d->rxQueue[d->rxTail];
  memcpy(slot->data, data, len);
  slot->len = len;
  d->rxTail = (d->rxTail + 1) % SL_EDRV_QUEUE_CAP;
  d->rxCount++;
}

/* 帧出队，返回是否成功 */
static bool edrvQueuePop(SlEdrvWinPcap* d, SlEdrvRxSlot* out)
{
  SlEdrvRxSlot* slot;

  if (d->rxCount == 0) {
    return false;
  }
  slot = &d->rxQueue[d->rxHead];
  memcpy(out, slot, sizeof(*out));
  d->rxHead = (d->rxHead + 1) % SL_EDRV_QUEUE_CAP;
  d->rxCount--;
  return true;
}

/* 按接收过滤器掩码匹配目的 MAC */
static bool edrvMatchFilter(const SlEdrvWinPcap* d, const uint8_t* frame, uint16_t len)
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
  if (mask == SL_RX_FILTER_ALL) {
    return true;
  }
  for (i = 0; i < 5; i++) {
    if (memcmp(frame, mcast[i], 6) == 0) {
      return (mask & (1u << i)) != 0;
    }
  }
  if (frame[0] == 0xFF && frame[1] == 0xFF && frame[2] == 0xFF &&
      frame[3] == 0xFF && frame[4] == 0xFF && frame[5] == 0xFF) {
    return (mask & SL_RX_FILTER_BROADCAST) != 0;
  }
  if (memcmp(frame, d->mac, 6) == 0) {
    return (mask & SL_RX_FILTER_UNICAST) != 0;
  }
  return false;
}

/* ========== 收包线程 ========== */

static DWORD WINAPI edrvRxThread(LPVOID arg)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)arg;
  struct pcap_pkthdr* hdr;
  const u_char* pkt;
  int ret;
  uint16_t len;

  while (d->running) {
    ret = pcap_next_ex(d->pcap, &hdr, &pkt);
    if (ret == 1) {
      len = hdr->caplen > SL_EDRV_MAX_FRAME ? SL_EDRV_MAX_FRAME
                                             : (uint16_t)hdr->caplen;
      if (len >= 14) {
        EnterCriticalSection(&d->lock);
        edrvQueuePush(d, pkt, len);
        LeaveCriticalSection(&d->lock);
        SetEvent(d->rxEvent);
      }
    } else if (ret == -1) {
      break; /* pcap 错误，退出线程 */
    }
  }
  return 0;
}

/* ========== SlEdrv 接口实现 ========== */

static int edrvWinPcapInit(SlEdrv* self)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;
  char errbuf[PCAP_ERRBUF_SIZE];
  DWORD threadId;
  int result;

  if (d == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  memset(d, 0, sizeof(*d));
  d->rxFilterMask = SL_RX_FILTER_ALL;

  result = edrvOpenDevice(d, errbuf);
  if (result != SL_ERR_OK) {
    return result;
  }
  InitializeCriticalSection(&d->lock);
  d->rxEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (d->rxEvent == NULL) {
    pcap_close(d->pcap);
    d->pcap = NULL;
    return SL_ERR_NO_MEMORY;
  }
  d->running = 1;
  d->rxThread = CreateThread(NULL, 0, edrvRxThread, d, 0, &threadId);
  if (d->rxThread == NULL) {
    d->running = 0;
    CloseHandle(d->rxEvent);
    pcap_close(d->pcap);
    d->pcap = NULL;
    return SL_ERR_NO_MEMORY;
  }
  return SL_ERR_OK;
}

static int edrvWinPcapExit(SlEdrv* self)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;

  if (d == NULL || d->pcap == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  d->running = 0;
  WaitForSingleObject(d->rxThread, 2000);
  CloseHandle(d->rxThread);
  CloseHandle(d->rxEvent);
  pcap_close(d->pcap);
  DeleteCriticalSection(&d->lock);
  memset(d, 0, sizeof(*d));
  return SL_ERR_OK;
}

static int edrvWinPcapGetMacAddr(SlEdrv* self, uint8_t mac[6])
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;

  if (d == NULL || mac == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  memcpy(mac, d->mac, 6);
  return SL_ERR_OK;
}

static int edrvWinPcapSend(SlEdrv* self, const uint8_t* frame, uint16_t len)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;
  uint8_t txBuf[60];
  int ret;

  if (d == NULL || d->pcap == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  if (frame == NULL || len == 0 || len > SL_EDRV_MAX_FRAME) {
    return SL_ERR_INVALID_PARAM;
  }
  if (len < 60) {
    memcpy(txBuf, frame, len);
    memset(txBuf + len, 0, 60 - len);
    frame = txBuf;
    len = 60;
  }
  EnterCriticalSection(&d->lock);
  ret = pcap_sendpacket(d->pcap, frame, (int)len);
  LeaveCriticalSection(&d->lock);
  return ret == 0 ? SL_ERR_OK : SL_ERR_LINK_DOWN;
}

static int edrvWinPcapSetRxFilter(SlEdrv* self, uint16_t filterMask)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;

  if (d == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  EnterCriticalSection(&d->lock);
  d->rxFilterMask = filterMask;
  LeaveCriticalSection(&d->lock);
  return SL_ERR_OK;
}

static int edrvWinPcapSetRxCallback(SlEdrv* self, SlEdrvRxCallback cb, void* ctx)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;

  if (d == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  d->rxCallback = cb;
  d->rxCtx = ctx;
  return SL_ERR_OK;
}

static int edrvWinPcapLinkState(SlEdrv* self, SlLinkState* state)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;

  if (d == NULL || state == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  *state = (d->pcap != NULL) ? SL_LINK_UP : SL_LINK_DOWN;
  return SL_ERR_OK;
}

static int edrvWinPcapPoll(SlEdrv* self, uint32_t timeoutMs)
{
  SlEdrvWinPcap* d = (SlEdrvWinPcap*)self->priv;
  SlEdrvRxSlot slot;

  if (d == NULL || d->pcap == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  WaitForSingleObject(d->rxEvent, timeoutMs);
  for (;;) {
    EnterCriticalSection(&d->lock);
    if (!edrvQueuePop(d, &slot)) {
      LeaveCriticalSection(&d->lock);
      break;
    }
    LeaveCriticalSection(&d->lock);
    if (d->rxCallback != NULL &&
        edrvMatchFilter(d, slot.data, slot.len)) {
      d->rxCallback(slot.data, slot.len, d->rxCtx);
    }
  }
  return SL_ERR_OK;
}

/* ========== 工厂 ========== */

/**
 * 获取 WinPcap 驱动单例（静态存储，vtable 已填好）。
 * 使用前需调用 init，退出时调用 exit。
 */
SlEdrv* sl_edrv_winpcap_get(void)
{
  s_edrv.priv = &s_priv;
  s_edrv.init = edrvWinPcapInit;
  s_edrv.exit = edrvWinPcapExit;
  s_edrv.getMacAddr = edrvWinPcapGetMacAddr;
  s_edrv.send = edrvWinPcapSend;
  s_edrv.setRxFilter = edrvWinPcapSetRxFilter;
  s_edrv.setRxCallback = edrvWinPcapSetRxCallback;
  s_edrv.linkState = edrvWinPcapLinkState;
  s_edrv.poll = edrvWinPcapPoll;
  return &s_edrv;
}
