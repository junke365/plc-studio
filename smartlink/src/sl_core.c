/**
 * sl_core.c - 智能总线 帧构建/解析与 CRC32 实现
 *
 * 帧内字段小端序，EtherType 大端序（即帧内容为 AB 88）。
 * 以太网 CRC32 采用 IEEE 802.3 反射多项式 0xEDB88320。
 */

#include "smartlink/sl_core.h"

int sl_get_mcast_mac(SlMsgType type, uint8_t mac[6])
{
  uint64_t v;

  switch (type) {
    case SL_MSG_SOC:  v = SL_MCAST_SOC;  break;
    case SL_MSG_PRES: v = SL_MCAST_PRES; break;
    case SL_MSG_SOA:  v = SL_MCAST_SOA;  break;
    case SL_MSG_ASND: v = SL_MCAST_ASND; break;
    case SL_MSG_AMNI: v = SL_MCAST_AMNI; break;
    default:
      return SL_ERR_INVALID_PARAM;
  }

  /* 高位字节先存（网络字节序）：MAC = 01:11:1E:00:00:0x */
  mac[0] = (uint8_t)(v >> 40);
  mac[1] = (uint8_t)(v >> 32);
  mac[2] = (uint8_t)(v >> 24);
  mac[3] = (uint8_t)(v >> 16);
  mac[4] = (uint8_t)(v >> 8);
  mac[5] = (uint8_t)v;
  return SL_ERR_OK;
}

void sl_frame_fill_header(SlFrame* frame, const uint8_t dstMac[6],
                           const uint8_t srcMac[6], SlMsgType type,
                           uint8_t dstNode, uint8_t srcNode)
{
  memcpy(frame->aDstMac, dstMac, 6);
  memcpy(frame->aSrcMac, srcMac, 6);
  frame->etherType = sl_hton16(SL_ETHERTYPE);   /* 帧内容 AB 88 */
  frame->messageType = type;
  frame->dstNodeId = dstNode;
  frame->srcNodeId = srcNode;
}

uint16_t sl_build_soc(SlFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint32_t cycleLen,
                       uint64_t relativeTimeUs)
{
  uint8_t dstMac[6];

  if (frame == NULL || srcMac == NULL) {
    return 0;
  }

  sl_get_mcast_mac(SL_MSG_SOC, dstMac);
  sl_frame_fill_header(frame, dstMac, srcMac, SL_MSG_SOC,
                        SL_ADR_BROADCAST, srcNode);

  memset(&frame->data.soc, 0, sizeof(frame->data.soc));
  frame->data.soc.flag1 = 0;                    /* 无 PS/MC */
  frame->data.soc.relativeTimeLe = (uint64_t)relativeTimeUs;

  /* 实际长度至少 36 字节，以太网最小 60 由发送端补齐 */
  return 36;
}

uint16_t sl_build_preq(SlFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, const uint8_t* payload,
                        uint16_t size, uint8_t flag1)
{
  uint16_t i;

  if (frame == NULL || srcMac == NULL || dstMac == NULL ||
      (size > 0 && payload == NULL) || size > 256) {
    return 0;
  }

  sl_frame_fill_header(frame, dstMac, srcMac, SL_MSG_PREQ,
                        dstNode, srcNode);
  memset(&frame->data.preq, 0, sizeof(frame->data.preq));
  frame->data.preq.flag1 = flag1;
  frame->data.preq.sizeLe = (uint16_t)size;
  for (i = 0; i < size; i++) {
    frame->data.preq.aPayload[i] = payload[i];
  }

  return (uint16_t)(24 + size);
}

uint16_t sl_build_pres(SlFrame* frame, const uint8_t srcMac[6],
                        uint8_t srcNode, uint8_t nmtStatus,
                        const uint8_t* payload, uint16_t size,
                        uint8_t flag1, uint8_t flag2)
{
  uint8_t dstMac[6];
  uint16_t i;

  if (frame == NULL || srcMac == NULL ||
      (size > 0 && payload == NULL) || size > 256) {
    return 0;
  }

  sl_get_mcast_mac(SL_MSG_PRES, dstMac);
  sl_frame_fill_header(frame, dstMac, srcMac, SL_MSG_PRES,
                        SL_ADR_BROADCAST, srcNode);
  memset(&frame->data.pres, 0, sizeof(frame->data.pres));
  frame->data.pres.nmtStatus = nmtStatus;
  frame->data.pres.flag1 = flag1;
  frame->data.pres.flag2 = flag2;
  frame->data.pres.sizeLe = (uint16_t)size;
  for (i = 0; i < size; i++) {
    frame->data.pres.aPayload[i] = payload[i];
  }

  return (uint16_t)(24 + size);
}

uint16_t sl_build_soa(SlFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint8_t nmtStatus,
                       uint8_t reqServiceId, uint8_t reqServiceTarget,
                       uint8_t flag1)
{
  uint8_t dstMac[6];

  if (frame == NULL || srcMac == NULL) {
    return 0;
  }

  sl_get_mcast_mac(SL_MSG_SOA, dstMac);
  sl_frame_fill_header(frame, dstMac, srcMac, SL_MSG_SOA,
                        SL_ADR_BROADCAST, srcNode);
  memset(&frame->data.soa, 0, sizeof(frame->data.soa));
  frame->data.soa.nmtStatus = nmtStatus;
  frame->data.soa.flag1 = flag1;
  frame->data.soa.reqServiceId = reqServiceId;
  frame->data.soa.reqServiceTarget = reqServiceTarget;
  frame->data.soa.slVersion = SL_VERSION_FULL >> 24;
  frame->data.soa.flag3 = 0;                    /* 无 MN 冗余 */

  return 54;
}

uint16_t sl_build_asnd(SlFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, uint8_t serviceId,
                        const uint8_t* payload, uint16_t size)
{
  uint16_t i;

  if (frame == NULL || srcMac == NULL || dstMac == NULL ||
      (size > 0 && payload == NULL) || size > 256) {
    return 0;
  }

  sl_frame_fill_header(frame, dstMac, srcMac, SL_MSG_ASND,
                        dstNode, srcNode);
  frame->data.asnd.serviceId = serviceId;
  for (i = 0; i < size; i++) {
    frame->data.asnd.payload.aPayload[i] = payload[i];
  }

  return (uint16_t)(18 + size);
}

uint32_t sl_crc32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFFu;
  size_t i;
  int b;

  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (b = 0; b < 8; b++) {
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1));
    }
  }
  return ~crc;
}

uint16_t sl_frame_min_len(SlMsgType type)
{
  switch (type) {
    case SL_MSG_SOC:  return SL_MINSIZE_SOC;
    case SL_MSG_PREQ: return SL_MINSIZE_PREQ;
    case SL_MSG_PRES: return SL_MINSIZE_PRES;
    case SL_MSG_SOA:  return SL_MINSIZE_SOA;
    case SL_MSG_ASND: return 20;
    default:           return 0;
  }
}

int sl_frame_validate(const SlFrame* frame, uint16_t len)
{
  if (frame == NULL || len < 14) {
    return SL_ERR_INVALID_PARAM;
  }

  /* EtherType 需为 0x88AB（网络字节序 0xAB88） */
  if (frame->etherType != sl_hton16(SL_ETHERTYPE)) {
    return SL_ERR_PROTOCOL;
  }

  switch (frame->messageType) {
    case SL_MSG_SOC:
    case SL_MSG_PREQ:
    case SL_MSG_PRES:
    case SL_MSG_SOA:
    case SL_MSG_ASND:
    case SL_MSG_AMNI:
      break;
    default:
      return SL_ERR_PROTOCOL;
  }

  return SL_ERR_OK;
}
