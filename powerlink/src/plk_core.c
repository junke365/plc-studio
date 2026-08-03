/**
 * plk_core.c - POWERLINK 帧构建/解析与 CRC32 实现
 *
 * 帧内字段小端序，EtherType 大端序（即帧内容为 AB 88）。
 * 以太网 CRC32 采用 IEEE 802.3 反射多项式 0xEDB88320。
 */

#include "plk/plk_core.h"

int plk_get_mcast_mac(PlkMsgType type, uint8_t mac[6])
{
  uint64_t v;

  switch (type) {
    case PLK_MSG_SOC:  v = PLK_MCAST_SOC;  break;
    case PLK_MSG_PRES: v = PLK_MCAST_PRES; break;
    case PLK_MSG_SOA:  v = PLK_MCAST_SOA;  break;
    case PLK_MSG_ASND: v = PLK_MCAST_ASND; break;
    case PLK_MSG_AMNI: v = PLK_MCAST_AMNI; break;
    default:
      return PLK_ERR_INVALID_PARAM;
  }

  /* 高位字节先存（网络字节序）：MAC = 01:11:1E:00:00:0x */
  mac[0] = (uint8_t)(v >> 40);
  mac[1] = (uint8_t)(v >> 32);
  mac[2] = (uint8_t)(v >> 24);
  mac[3] = (uint8_t)(v >> 16);
  mac[4] = (uint8_t)(v >> 8);
  mac[5] = (uint8_t)v;
  return PLK_ERR_OK;
}

void plk_frame_fill_header(PlkFrame* frame, const uint8_t dstMac[6],
                           const uint8_t srcMac[6], PlkMsgType type,
                           uint8_t dstNode, uint8_t srcNode)
{
  memcpy(frame->aDstMac, dstMac, 6);
  memcpy(frame->aSrcMac, srcMac, 6);
  frame->etherType = plk_hton16(PLK_ETHERTYPE);   /* 帧内容 AB 88 */
  frame->messageType = type;
  frame->dstNodeId = dstNode;
  frame->srcNodeId = srcNode;
}

uint16_t plk_build_soc(PlkFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint32_t cycleLen,
                       uint64_t relativeTimeUs)
{
  uint8_t dstMac[6];

  if (frame == NULL || srcMac == NULL) {
    return 0;
  }

  plk_get_mcast_mac(PLK_MSG_SOC, dstMac);
  plk_frame_fill_header(frame, dstMac, srcMac, PLK_MSG_SOC,
                        PLK_ADR_BROADCAST, srcNode);

  memset(&frame->data.soc, 0, sizeof(frame->data.soc));
  frame->data.soc.flag1 = 0;                    /* 无 PS/MC */
  frame->data.soc.relativeTimeLe = (uint64_t)relativeTimeUs;

  /* 实际长度至少 36 字节，以太网最小 60 由发送端补齐 */
  return 36;
}

uint16_t plk_build_preq(PlkFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, const uint8_t* payload,
                        uint16_t size, uint8_t flag1)
{
  uint16_t i;

  if (frame == NULL || srcMac == NULL || dstMac == NULL ||
      (size > 0 && payload == NULL) || size > 256) {
    return 0;
  }

  plk_frame_fill_header(frame, dstMac, srcMac, PLK_MSG_PREQ,
                        dstNode, srcNode);
  memset(&frame->data.preq, 0, sizeof(frame->data.preq));
  frame->data.preq.flag1 = flag1;
  frame->data.preq.sizeLe = (uint16_t)size;
  for (i = 0; i < size; i++) {
    frame->data.preq.aPayload[i] = payload[i];
  }

  return (uint16_t)(24 + size);
}

uint16_t plk_build_pres(PlkFrame* frame, const uint8_t srcMac[6],
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

  plk_get_mcast_mac(PLK_MSG_PRES, dstMac);
  plk_frame_fill_header(frame, dstMac, srcMac, PLK_MSG_PRES,
                        PLK_ADR_BROADCAST, srcNode);
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

uint16_t plk_build_soa(PlkFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint8_t nmtStatus,
                       uint8_t reqServiceId, uint8_t reqServiceTarget,
                       uint8_t flag1)
{
  uint8_t dstMac[6];

  if (frame == NULL || srcMac == NULL) {
    return 0;
  }

  plk_get_mcast_mac(PLK_MSG_SOA, dstMac);
  plk_frame_fill_header(frame, dstMac, srcMac, PLK_MSG_SOA,
                        PLK_ADR_BROADCAST, srcNode);
  memset(&frame->data.soa, 0, sizeof(frame->data.soa));
  frame->data.soa.nmtStatus = nmtStatus;
  frame->data.soa.flag1 = flag1;
  frame->data.soa.reqServiceId = reqServiceId;
  frame->data.soa.reqServiceTarget = reqServiceTarget;
  frame->data.soa.powerlinkVersion = PLK_VERSION_FULL >> 24;
  frame->data.soa.flag3 = 0;                    /* 无 MN 冗余 */

  return 54;
}

uint16_t plk_build_asnd(PlkFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, uint8_t serviceId,
                        const uint8_t* payload, uint16_t size)
{
  uint16_t i;

  if (frame == NULL || srcMac == NULL || dstMac == NULL ||
      (size > 0 && payload == NULL) || size > 256) {
    return 0;
  }

  plk_frame_fill_header(frame, dstMac, srcMac, PLK_MSG_ASND,
                        dstNode, srcNode);
  frame->data.asnd.serviceId = serviceId;
  for (i = 0; i < size; i++) {
    frame->data.asnd.payload.aPayload[i] = payload[i];
  }

  return (uint16_t)(18 + size);
}

uint32_t plk_crc32(const uint8_t* data, size_t len)
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

uint16_t plk_frame_min_len(PlkMsgType type)
{
  switch (type) {
    case PLK_MSG_SOC:  return PLK_MINSIZE_SOC;
    case PLK_MSG_PREQ: return PLK_MINSIZE_PREQ;
    case PLK_MSG_PRES: return PLK_MINSIZE_PRES;
    case PLK_MSG_SOA:  return PLK_MINSIZE_SOA;
    case PLK_MSG_ASND: return 20;
    default:           return 0;
  }
}

int plk_frame_validate(const PlkFrame* frame, uint16_t len)
{
  if (frame == NULL || len < 14) {
    return PLK_ERR_INVALID_PARAM;
  }

  /* EtherType 需为 0x88AB（网络字节序 0xAB88） */
  if (frame->etherType != plk_hton16(PLK_ETHERTYPE)) {
    return PLK_ERR_PROTOCOL;
  }

  switch (frame->messageType) {
    case PLK_MSG_SOC:
    case PLK_MSG_PREQ:
    case PLK_MSG_PRES:
    case PLK_MSG_SOA:
    case PLK_MSG_ASND:
    case PLK_MSG_AMNI:
      break;
    default:
      return PLK_ERR_PROTOCOL;
  }

  return PLK_ERR_OK;
}
