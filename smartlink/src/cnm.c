/**
 * cnm.c - CN 协议栈实例实现
 *
 * 帧分发：
 *   SoC           → NMT 状态机（周期同步）
 *   ASnd/NMT 命令 → NMT 状态机
 *   ASnd/SDO      → SDO 服务器（操作对象字典并应答）
 *   PReq          → 拷贝输出 PDO，回发 PRes（载荷为输入 PDO）
 *
 * SDO 服务器经绑定的 edrv 发送应答；NMT 事件映射见 sl_cn_nmt_process。
 */

#include "smartlink/cnm.h"
#include "smartlink/sdo.h"
#include "smartlink/sl_core.h"

/* SDO 发送回调无上下文参数，使用当前活动 CN 实例 */
static SlCnm* s_activeCnm;

static int cnm_sdo_send(const uint8_t* frame, uint16_t len)
{
  SlCnm* cnm = s_activeCnm;

  if (cnm == NULL || cnm->edrv == NULL || cnm->edrv->send == NULL) {
    return SL_ERR_NOT_INITIALIZED;
  }
  return cnm->edrv->send(cnm->edrv, frame, len);
}

static int cnm_sdo_pump(uint8_t* frame, uint16_t maxLen, uint16_t* len,
                        uint32_t timeoutMs)
{
  (void)frame; (void)maxLen; (void)len; (void)timeoutMs;
  /* 服务器侧被动响应：不主动收帧 */
  return SL_ERR_TIMEOUT;
}

/* NMT 命令 ID → 状态机事件 */
static SlNmtEvent cnm_cmd_to_event(uint8_t cmd)
{
  switch (cmd) {
    case SL_NMT_CMD_SW_RESET:
      return SL_NMT_EVENT_SW_RESET;
    case SL_NMT_CMD_START_UP:
      return SL_NMT_EVENT_START_UP;
    case SL_NMT_CMD_ENTER_PRE_OPERATIONAL_1:
      return SL_NMT_EVENT_TO_PRE_OPERATIONAL_1;
    case SL_NMT_CMD_ENTER_PRE_OPERATIONAL_2:
      return SL_NMT_EVENT_TO_PRE_OPERATIONAL_2;
    case SL_NMT_CMD_ENTER_READY_TO_OPERATE:
      return SL_NMT_EVENT_ENABLE_READY_TO_OPERATE;
    case SL_NMT_CMD_ENTER_OPERATIONAL:
      return SL_NMT_EVENT_TO_OPERATIONAL;
    case SL_NMT_CMD_START_NODE:
      return SL_NMT_EVENT_START_NODE;
    case SL_NMT_CMD_STOP_NODE:
      return SL_NMT_EVENT_STOP_NODE;
    case SL_NMT_CMD_RESYNC:
      return SL_NMT_EVENT_RESYNC;
    default:
      return SL_NMT_EVENT_SW_RESET;
  }
}

/* 处理 PReq：更新输出 PDO 并回发 PRes（输入 PDO） */
static void cnm_handle_preq(SlCnm* cnm, const SlFrame* f)
{
  SlFrame pres;
  uint16_t sz;
  uint16_t len;
  const uint8_t* inData;
  uint16_t inSize;

  sz = f->data.preq.sizeLe;
  if (sz > cnm->pollOutSize) {
    sz = cnm->pollOutSize;
  }
  if (cnm->pollOutData != NULL && sz > 0) {
    memcpy(cnm->pollOutData, f->data.preq.aPayload, sz);
  }
  if (cnm->pdoOutCb != NULL) {
    cnm->pdoOutCb(cnm->pollOutData, sz, cnm->pdoOutCtx);
  }

  inData = cnm->pollInData;
  inSize = cnm->pollInSize;
  if (inData == NULL) {
    inSize = 0;
  }
  len = sl_build_pres(&pres, cnm->mac, cnm->nodeId,
                       (uint8_t)(cnm->sm.state & 0xFF),
                       inData, inSize, SL_FRAME_FLAG1_RD, 0);
  if (len > 0 && cnm->edrv != NULL && cnm->edrv->send != NULL) {
    cnm->edrv->send(cnm->edrv, (const uint8_t*)&pres, len);
  }
}

int sl_cnm_init(SlCnm* cnm, uint8_t nodeId, const uint8_t mac[6],
                 SlEdrv* edrv)
{
  if (cnm == NULL || mac == NULL || edrv == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  if (edrv->getMacAddr == NULL || edrv->send == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  memset(cnm, 0, sizeof(*cnm));
  cnm->nodeId = nodeId;
  memcpy(cnm->mac, mac, 6);
  cnm->edrv = edrv;

  sl_od_init(&cnm->od);
  sl_cn_nmt_init(&cnm->sm);

  /* SDO 服务器：绑定本地对象字典与 edrv 发送 */
  s_activeCnm = cnm;
  if (sl_sdo_init(4, &cnm->od) != SL_ERR_OK) {
    s_activeCnm = NULL;
    return SL_ERR_INVALID_PARAM;
  }
  sl_sdo_set_local_node(nodeId);
  sl_sdo_set_io(cnm_sdo_send, cnm_sdo_pump);

  return SL_ERR_OK;
}

void sl_cnm_exit(SlCnm* cnm)
{
  if (cnm == NULL) {
    return;
  }
  if (s_activeCnm == cnm) {
    s_activeCnm = NULL;
  }
  cnm->started = false;
}

int sl_cnm_start(SlCnm* cnm)
{
  int ret;

  if (cnm == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  /* 上电事件序列 → NotActive（等待 MN 的 SoC） */
  sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_START_UP);
  sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_SWITCH_ON);
  sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_REQUEST_RESET_COM);
  sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_REQUEST_RESET_CONFIG);
  sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_SWITCH_ON);

  cnm->started = true;
  ret = (cnm->sm.state == SL_NMT_CS_NOT_ACTIVE) ? SL_ERR_OK : SL_ERR_NMT_STATE;
  return ret;
}

int sl_cnm_set_pdo_in(SlCnm* cnm, uint8_t* data, uint16_t size)
{
  if (cnm == NULL || (size > 0 && data == NULL)) {
    return SL_ERR_INVALID_PARAM;
  }
  cnm->pollInData = data;
  cnm->pollInSize = size;
  return SL_ERR_OK;
}

int sl_cnm_set_pdo_out(SlCnm* cnm, uint8_t* data, uint16_t size,
                        SlCnmPdoOutCb cb, void* ctx)
{
  if (cnm == NULL || (size > 0 && data == NULL)) {
    return SL_ERR_INVALID_PARAM;
  }
  cnm->pollOutData = data;
  cnm->pollOutSize = size;
  cnm->pdoOutCb = cb;
  cnm->pdoOutCtx = ctx;
  return SL_ERR_OK;
}

int sl_cnm_app_ready(SlCnm* cnm)
{
  if (cnm == NULL) {
    return SL_ERR_INVALID_PARAM;
  }
  return sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_ENTER_READY_TO_OPERATE);
}

/* 处理 IdentRequest：回发 IdentRes 宣告身份与 PDO 尺寸 */
static void cnm_send_ident_response(SlCnm* cnm, const SlFrame* req)
{
  SlFrame resp;
  SlIdentResponse ident;
  uint8_t mcast[6];
  uint16_t len;

  memset(&ident, 0, sizeof(ident));
  ident.flag2 = 0x18;                            /* PR | RS */
  ident.nmtStatus = (uint8_t)(cnm->sm.state & 0xFF);
  ident.slProfileVersion = SL_PROFILE_VERSION;
  ident.mtuLe = SL_ASYNC_MTU;
  ident.pollInSizeLe = cnm->pollInSize;
  ident.pollOutSizeLe = cnm->pollOutSize;
  ident.deviceTypeLe = 1;
  ident.vendorIdLe = 0x00000001;
  ident.productCodeLe = 0x00000001;
  ident.revisionNumberLe = 0x00000100;
  ident.serialNumberLe = 0x00000001;

  sl_get_mcast_mac(SL_MSG_ASND, mcast);
  len = sl_build_asnd(&resp, cnm->mac, mcast, cnm->nodeId, req->srcNodeId,
                       SL_ASND_IDENT_RESPONSE, (const uint8_t*)&ident,
                       sizeof(ident));
  if (cnm->edrv != NULL && cnm->edrv->send != NULL) {
    cnm->edrv->send(cnm->edrv, (const uint8_t*)&resp, len);
  }
}

int sl_cnm_process_rx(SlCnm* cnm, const uint8_t* raw, uint16_t len)
{
  const SlFrame* f;

  if (cnm == NULL || raw == NULL) {
    return SL_ERR_INVALID_PARAM;
  }

  f = (const SlFrame*)raw;
  if (sl_frame_validate(f, len) != SL_ERR_OK) {
    return SL_ERR_PROTOCOL;
  }

  switch (f->messageType) {
    case SL_MSG_SOC:
      sl_cn_nmt_process(&cnm->sm, SL_NMT_EVENT_RECEIVE_SOC);
      break;

    case SL_MSG_ASND:
      if (f->data.asnd.serviceId == SL_ASND_NMT_COMMAND) {
        uint8_t cmd = f->data.asnd.payload.nmtCommandService.nmtCommandId;
        sl_cn_nmt_process(&cnm->sm, cnm_cmd_to_event(cmd));
      } else if (f->data.asnd.serviceId == SL_ASND_IDENT_RESPONSE) {
        if (f->dstNodeId == cnm->nodeId ||
            f->dstNodeId == SL_ADR_BROADCAST ||
            f->dstNodeId == 0) {
          cnm_send_ident_response(cnm, f);
        }
      } else if (f->data.asnd.serviceId == SL_ASND_SDO) {
        if (f->dstNodeId == cnm->nodeId ||
            f->dstNodeId == SL_ADR_BROADCAST ||
            f->dstNodeId == 0) {
          sl_sdo_process_rx(f);
        }
      }
      break;

    case SL_MSG_PREQ:
      if (f->dstNodeId == cnm->nodeId) {
        cnm_handle_preq(cnm, f);
      }
      break;

    default:
      break;
  }

  return SL_ERR_OK;
}
