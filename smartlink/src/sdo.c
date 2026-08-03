/**
 * sdo.c - SDO 服务实现
 *
 * 依据 EPSG DS 301 V1.2.0 第 6.3 章。
 * 实现：
 *   - 序列层：发送/接收序列号管理
 *   - 命令层：WriteByIndex / ReadByIndex（expedited 传输，<= 4 字节）
 *   - 服务器侧：处理接收的 SDO 请求并构造响应
 *   - 客户端侧：阻塞式按索引读写（内部轮询接收泵）
 *
 * 分段传输（segmented）留待后续扩展，超过 4 字节返回 Abort。
 */

#include "smartlink/sdo.h"
#include "smartlink/sl_core.h"
#include "smartlink/edrv.h"

/* ========== 内部上下文 ========== */

typedef struct
{
  bool          initialized;
  SlOd*        od;              /* 服务器侧对象字典 */
  uint8_t       localNode;       /* 本机节点 ID */
  uint8_t       sendSeqNum;      /* 发送序列号 */
  uint8_t       transactionId;   /* 事务 ID 计数 */
  SlSdoSendFn  sendFn;          /* 发送回调 */
  SlSdoPumpFn  pumpFn;          /* 接收泵 */

  /* 客户端等待状态 */
  struct
  {
    bool        pending;
    uint8_t     remoteNode;
    uint8_t     waitTid;
    uint8_t*    buf;             /* 数据缓冲 */
    uint16_t*   size;            /* 容量/实际长度 */
  } client;
} SlSdoCtx;

static SlSdoCtx s_ctx;

/* ========== 辅助 ========== */

static void sdo_get_mac(uint8_t mac[6])
{
  SlEdrv* edrv;

  edrv = sl_edrv_get();
  if (edrv != NULL && edrv->getMacAddr != NULL &&
      edrv->getMacAddr(edrv, mac) == SL_ERR_OK) {
    return;
  }
  memset(mac, 0, 6);
}

static uint16_t sdo_build_asnd(SlFrame* frame, uint8_t dstNode,
                               uint8_t serviceId, const uint8_t* payload,
                               uint16_t size)
{
  uint8_t srcMac[6];
  uint8_t dstMac[6];

  sdo_get_mac(srcMac);
  memset(dstMac, 0xFF, 6);   /* 广播，简化 MAC 解析 */
  return sl_build_asnd(frame, srcMac, dstMac, s_ctx.localNode,
                        dstNode, serviceId, payload, size);
}

/* ========== 服务端：处理请求 ========== */

static void sdo_respond(SlFrame* frame, const SlAsySdoCom* req,
                        uint8_t respFlags, const uint8_t* data,
                        uint16_t size)
{
  SlAsySdoSeq seq;
  uint8_t payload[32];
  uint16_t len;

  /* 序列层响应头 */
  seq.recvSeqNumCon = req->transactionId;   /* 回显事务 */
  seq.sendSeqNumCon = s_ctx.sendSeqNum++;
  seq.aReserved[0] = 0;
  seq.aReserved[1] = 0;

  /* 命令层响应 */
  seq.sdoSeqPayload.reserved1 = 0;
  seq.sdoSeqPayload.transactionId = req->transactionId;
  seq.sdoSeqPayload.flags = respFlags;
  seq.sdoSeqPayload.commandId = req->commandId;
  seq.sdoSeqPayload.segmentSizeLe = req->segmentSizeLe;
  seq.sdoSeqPayload.reserved2 = 0;
  memset(seq.sdoSeqPayload.aCommandData, 0,
         sizeof(seq.sdoSeqPayload.aCommandData));

  /* 拷贝变量头（index/subindex/reserved 4 字节）+ 数据 */
  if (size > 4) {
    size = 4;
  }
  memcpy(seq.sdoSeqPayload.aCommandData, req->aCommandData, 4);
  if (size > 0 && data != NULL) {
    memcpy(&seq.sdoSeqPayload.aCommandData[4], data, size);
  }

  memcpy(payload, &seq, sizeof(seq));
  len = sdo_build_asnd(frame, frame->srcNodeId, SL_ASND_SDO,
                       payload, (uint16_t)(4 + sizeof(seq)));
  if (s_ctx.sendFn != NULL) {
    s_ctx.sendFn((const uint8_t*)frame, len);
  }
}

static void sdo_abort(SlFrame* frame, const SlAsySdoCom* req,
                      uint32_t abortCode)
{
  uint8_t code[4];

  code[0] = (uint8_t)(abortCode & 0xFF);
  code[1] = (uint8_t)((abortCode >> 8) & 0xFF);
  code[2] = (uint8_t)((abortCode >> 16) & 0xFF);
  code[3] = (uint8_t)((abortCode >> 24) & 0xFF);
  sdo_respond(frame, req, SL_SDO_CMDL_FLAG_RESPONSE |
                           SL_SDO_CMDL_FLAG_ABORT,
              code, 4);
}

static int sdo_handle_request(SlFrame* frame)
{
  SlAsySdoSeq* seq;
  SlAsySdoCom* com;
  uint8_t data[4];
  uint16_t size;
  uint16_t index;
  uint8_t subIndex;
  uint8_t padSize;
  int ret;

  seq = (SlAsySdoSeq*)&frame->data.asnd.payload;
  com = &seq->sdoSeqPayload;

  /* 序列层：校验发送方序列号连续（简化，仅记录） */
  seq->recvSeqNumCon = com->transactionId;

  switch (com->commandId) {
    case SL_SDO_CID_WRITE_BY_INDEX:
      /* 变量头：index(2) + subindex(1) + reserved(1) */
      index = (uint16_t)(com->aCommandData[0] |
                         (com->aCommandData[1] << 8));
      subIndex = com->aCommandData[2];
      padSize = com->flags & SL_SDO_CMDL_FLAG_PADSIZE_MASK;
      size = (uint16_t)(4 - padSize);

      ret = sl_od_write(s_ctx.od, index, subIndex,
                         &com->aCommandData[4], size);
      if (ret == SL_ERR_OD_INDEX) {
        sdo_abort(frame, com, SL_SDO_ABORT_INDEX_NOT_EXIST);
      } else if (ret == SL_ERR_OD_SUBINDEX) {
        sdo_abort(frame, com, SL_SDO_ABORT_SUBINDEX_NOT_EXIST);
      } else if (ret == SL_ERR_OD_ACCESS) {
        sdo_abort(frame, com, SL_SDO_ABORT_UNSUPPORTED_ACCESS);
      } else if (ret == SL_ERR_OD_SIZE) {
        sdo_abort(frame, com, SL_SDO_ABORT_DATA_TYPE_MISMATCH);
      } else if (ret == SL_ERR_OK) {
        sdo_respond(frame, com, SL_SDO_CMDL_FLAG_RESPONSE |
                                SL_SDO_CMDL_FLAG_EXPEDITED,
                    NULL, 0);
      } else {
        sdo_abort(frame, com, SL_SDO_ABORT_GENERAL);
      }
      return SL_ERR_OK;

    case SL_SDO_CID_READ_BY_INDEX:
      index = (uint16_t)(com->aCommandData[0] |
                         (com->aCommandData[1] << 8));
      subIndex = com->aCommandData[2];
      size = sizeof(data);
      ret = sl_od_read(s_ctx.od, index, subIndex, data, &size);
      if (ret == SL_ERR_OD_INDEX) {
        sdo_abort(frame, com, SL_SDO_ABORT_INDEX_NOT_EXIST);
      } else if (ret == SL_ERR_OD_SUBINDEX) {
        sdo_abort(frame, com, SL_SDO_ABORT_SUBINDEX_NOT_EXIST);
      } else if (ret == SL_ERR_OD_ACCESS) {
        sdo_abort(frame, com, SL_SDO_ABORT_UNSUPPORTED_ACCESS);
      } else if (ret == SL_ERR_OD_SIZE) {
        /* 对象超过 4 字节：分段暂不支持 */
        sdo_abort(frame, com, SL_SDO_ABORT_DATA_TYPE_MISMATCH);
      } else if (ret == SL_ERR_OK) {
        sdo_respond(frame, com,
                    SL_SDO_CMDL_FLAG_RESPONSE |
                    SL_SDO_CMDL_FLAG_EXPEDITED |
                    (uint8_t)(4 - size),   /* padsize */
                    data, size);
      } else {
        sdo_abort(frame, com, SL_SDO_ABORT_GENERAL);
      }
      return SL_ERR_OK;

    default:
      sdo_abort(frame, com, SL_SDO_ABORT_CMD_SPECIFIER);
      return SL_ERR_OK;
  }
}

/* ========== 客户端：响应处理 ========== */

static int sdo_handle_response(SlFrame* frame)
{
  SlAsySdoSeq* seq;
  SlAsySdoCom* com;
  uint16_t size;
  int ret;

  seq = (SlAsySdoSeq*)&frame->data.asnd.payload;
  com = &seq->sdoSeqPayload;

  if (!s_ctx.client.pending ||
      com->transactionId != s_ctx.client.waitTid) {
    return SL_ERR_NO_DATA;   /* 不是我们在等的响应 */
  }

  s_ctx.client.pending = false;

  if (com->flags & SL_SDO_CMDL_FLAG_ABORT) {
    return SL_ERR_SDO_ABORT;   /* 对端报告中止 */
  }

  /* 仅 ReadByIndex 响应携带数据 */
  if (com->commandId == SL_SDO_CID_READ_BY_INDEX) {
    size = 4 - (com->flags & SL_SDO_CMDL_FLAG_PADSIZE_MASK);
    if (s_ctx.client.buf != NULL) {
      if (size > *s_ctx.client.size) {
        size = *s_ctx.client.size;
      }
      memcpy(s_ctx.client.buf, &com->aCommandData[4], size);
      *s_ctx.client.size = size;
    }
  }

  ret = SL_ERR_OK;
  return ret;
}

/* ========== 公共接口 ========== */

int sl_sdo_init(uint32_t maxCnx, SlOd* od)
{
  (void)maxCnx;
  if (od == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  memset(&s_ctx, 0, sizeof(s_ctx));
  s_ctx.od = od;
  s_ctx.localNode = SL_ADR_INVALID;
  s_ctx.initialized = true;
  return SL_ERR_OK;
}

int sl_sdo_set_local_node(uint8_t nodeId)
{
  if (!s_ctx.initialized) {
    return SL_ERR_NOT_INITIALIZED;
  }
  s_ctx.localNode = nodeId;
  return SL_ERR_OK;
}

int sl_sdo_set_io(SlSdoSendFn sendFn, SlSdoPumpFn pumpFn)
{
  if (!s_ctx.initialized) {
    return SL_ERR_NOT_INITIALIZED;
  }
  s_ctx.sendFn = sendFn;
  s_ctx.pumpFn = pumpFn;
  return SL_ERR_OK;
}

int sl_sdo_process_rx(const SlFrame* frame)
{
  SlFrame* f = (SlFrame*)frame;

  if (!s_ctx.initialized || frame == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  if (frame->messageType != SL_MSG_ASND ||
      frame->data.asnd.serviceId != SL_ASND_SDO) {
    return SL_ERR_NO_DATA;
  }

  if (frame->dstNodeId != SL_ADR_BROADCAST &&
      frame->dstNodeId != s_ctx.localNode &&
      frame->dstNodeId != 0) {
    return SL_ERR_NO_DATA;   /* 非本机 */
  }

  if (frame->data.asnd.payload.sdoSequenceFrame.sdoSeqPayload.flags
      & SL_SDO_CMDL_FLAG_RESPONSE) {
    return sdo_handle_response(f);   /* 响应 */
  }
  return sdo_handle_request(f);      /* 请求 */
}

int sl_sdo_read_req(uint8_t remoteNode, uint16_t index, uint8_t subIndex,
                     void* data, uint16_t* size, uint32_t timeoutMs)
{
  SlFrame frame;
  SlAsySdoSeq seq;
  uint8_t payload[32];
  uint16_t len;
  int ret;
  uint32_t elapsed = 0;

  if (!s_ctx.initialized || data == NULL || size == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* 构造 ReadByIndex 请求 */
  memset(&seq, 0, sizeof(seq));
  seq.sendSeqNumCon = s_ctx.sendSeqNum++;
  seq.sdoSeqPayload.transactionId = ++s_ctx.transactionId;
  seq.sdoSeqPayload.flags = SL_SDO_CMDL_FLAG_EXPEDITED;
  seq.sdoSeqPayload.commandId = SL_SDO_CID_READ_BY_INDEX;
  seq.sdoSeqPayload.aCommandData[0] = (uint8_t)(index & 0xFF);
  seq.sdoSeqPayload.aCommandData[1] = (uint8_t)(index >> 8);
  seq.sdoSeqPayload.aCommandData[2] = subIndex;
  seq.sdoSeqPayload.aCommandData[3] = 0;

  memcpy(payload, &seq, sizeof(seq));
  len = sdo_build_asnd(&frame, remoteNode, SL_ASND_SDO,
                       payload, (uint16_t)(4 + sizeof(seq)));
  if (s_ctx.sendFn == NULL || s_ctx.pumpFn == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }

  s_ctx.client.pending = true;
  s_ctx.client.remoteNode = remoteNode;
  s_ctx.client.waitTid = s_ctx.transactionId;
  s_ctx.client.buf = (uint8_t*)data;
  s_ctx.client.size = size;

  s_ctx.sendFn((const uint8_t*)&frame, len);

  /* 轮询等待响应 */
  while (elapsed < timeoutMs) {
    uint8_t rxBuf[SL_ETH_FRAME_MAX];
    uint16_t rxLen = 0;

    ret = s_ctx.pumpFn(rxBuf, sizeof(rxBuf), &rxLen, 10);
    if (ret == SL_ERR_OK) {
      ret = sl_sdo_process_rx((const SlFrame*)rxBuf);
      if (ret == SL_ERR_OK && !s_ctx.client.pending) {
        return SL_ERR_OK;
      }
      if (ret == SL_ERR_SDO_ABORT) {
        return SL_ERR_SDO_ABORT;
      }
    }
    elapsed += 10;
  }

  s_ctx.client.pending = false;
  return SL_ERR_TIMEOUT;
}

int sl_sdo_write_req(uint8_t remoteNode, uint16_t index, uint8_t subIndex,
                      const void* data, uint16_t size, uint32_t timeoutMs)
{
  SlFrame frame;
  SlAsySdoSeq seq;
  uint8_t payload[32];
  uint16_t len;
  uint8_t padSize;
  uint16_t sendSize;
  int ret;
  uint32_t elapsed = 0;

  if (!s_ctx.initialized || data == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* 仅支持 expedited（<= 4 字节） */
  if (size > 4) {
    return SL_ERR_OD_SIZE;
  }
  sendSize = size;
  padSize = (uint8_t)(4 - sendSize);

  /* 构造 WriteByIndex 请求 */
  memset(&seq, 0, sizeof(seq));
  seq.sendSeqNumCon = s_ctx.sendSeqNum++;
  seq.sdoSeqPayload.transactionId = ++s_ctx.transactionId;
  seq.sdoSeqPayload.flags = SL_SDO_CMDL_FLAG_EXPEDITED | padSize;
  seq.sdoSeqPayload.commandId = SL_SDO_CID_WRITE_BY_INDEX;
  seq.sdoSeqPayload.aCommandData[0] = (uint8_t)(index & 0xFF);
  seq.sdoSeqPayload.aCommandData[1] = (uint8_t)(index >> 8);
  seq.sdoSeqPayload.aCommandData[2] = subIndex;
  seq.sdoSeqPayload.aCommandData[3] = 0;
  memcpy(&seq.sdoSeqPayload.aCommandData[4], data, sendSize);

  memcpy(payload, &seq, sizeof(seq));
  len = sdo_build_asnd(&frame, remoteNode, SL_ASND_SDO,
                       payload, (uint16_t)(4 + sizeof(seq)));
  if (s_ctx.sendFn == NULL || s_ctx.pumpFn == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }

  s_ctx.client.pending = true;
  s_ctx.client.remoteNode = remoteNode;
  s_ctx.client.waitTid = s_ctx.transactionId;
  s_ctx.client.buf = NULL;
  s_ctx.client.size = NULL;

  s_ctx.sendFn((const uint8_t*)&frame, len);

  /* 轮询等待确认 */
  while (elapsed < timeoutMs) {
    uint8_t rxBuf[SL_ETH_FRAME_MAX];
    uint16_t rxLen = 0;

    ret = s_ctx.pumpFn(rxBuf, sizeof(rxBuf), &rxLen, 10);
    if (ret == SL_ERR_OK) {
      ret = sl_sdo_process_rx((const SlFrame*)rxBuf);
      if (ret == SL_ERR_OK && !s_ctx.client.pending) {
        return SL_ERR_OK;
      }
      if (ret == SL_ERR_SDO_ABORT) {
        return SL_ERR_SDO_ABORT;
      }
    }
    elapsed += 10;
  }

  s_ctx.client.pending = false;
  return SL_ERR_TIMEOUT;
}
