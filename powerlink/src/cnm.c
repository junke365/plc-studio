/**
 * cnm.c - CN 协议栈实例实现
 *
 * 帧分发：
 *   SoC           → NMT 状态机（周期同步）
 *   ASnd/NMT 命令 → NMT 状态机
 *   ASnd/SDO      → SDO 服务器（操作对象字典并应答）
 *   PReq          → 拷贝输出 PDO，回发 PRes（载荷为输入 PDO）
 *
 * SDO 服务器经绑定的 edrv 发送应答；NMT 事件映射见 plk_cn_nmt_process。
 */

#include "plk/cnm.h"
#include "plk/sdo.h"
#include "plk/plk_core.h"

/* SDO 发送回调无上下文参数，使用当前活动 CN 实例 */
static PlkCnm* s_activeCnm;

static int cnm_sdo_send(const uint8_t* frame, uint16_t len)
{
  PlkCnm* cnm = s_activeCnm;

  if (cnm == NULL || cnm->edrv == NULL || cnm->edrv->send == NULL) {
    return PLK_ERR_NOT_INITIALIZED;
  }
  return cnm->edrv->send(cnm->edrv, frame, len);
}

static int cnm_sdo_pump(uint8_t* frame, uint16_t maxLen, uint16_t* len,
                        uint32_t timeoutMs)
{
  (void)frame; (void)maxLen; (void)len; (void)timeoutMs;
  /* 服务器侧被动响应：不主动收帧 */
  return PLK_ERR_TIMEOUT;
}

/* NMT 命令 ID → 状态机事件 */
static PlkNmtEvent cnm_cmd_to_event(uint8_t cmd)
{
  switch (cmd) {
    case PLK_NMT_CMD_SW_RESET:
      return PLK_NMT_EVENT_SW_RESET;
    case PLK_NMT_CMD_START_UP:
      return PLK_NMT_EVENT_START_UP;
    case PLK_NMT_CMD_ENTER_PRE_OPERATIONAL_1:
      return PLK_NMT_EVENT_TO_PRE_OPERATIONAL_1;
    case PLK_NMT_CMD_ENTER_PRE_OPERATIONAL_2:
      return PLK_NMT_EVENT_TO_PRE_OPERATIONAL_2;
    case PLK_NMT_CMD_ENTER_READY_TO_OPERATE:
      return PLK_NMT_EVENT_ENABLE_READY_TO_OPERATE;
    case PLK_NMT_CMD_ENTER_OPERATIONAL:
      return PLK_NMT_EVENT_TO_OPERATIONAL;
    case PLK_NMT_CMD_START_NODE:
      return PLK_NMT_EVENT_START_NODE;
    case PLK_NMT_CMD_STOP_NODE:
      return PLK_NMT_EVENT_STOP_NODE;
    case PLK_NMT_CMD_RESYNC:
      return PLK_NMT_EVENT_RESYNC;
    default:
      return PLK_NMT_EVENT_SW_RESET;
  }
}

/* 处理 PReq：更新输出 PDO 并回发 PRes（输入 PDO） */
static void cnm_handle_preq(PlkCnm* cnm, const PlkFrame* f)
{
  PlkFrame pres;
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
  len = plk_build_pres(&pres, cnm->mac, cnm->nodeId,
                       (uint8_t)(cnm->sm.state & 0xFF),
                       inData, inSize, PLK_FRAME_FLAG1_RD, 0);
  if (len > 0 && cnm->edrv != NULL && cnm->edrv->send != NULL) {
    cnm->edrv->send(cnm->edrv, (const uint8_t*)&pres, len);
  }
}

int plk_cnm_init(PlkCnm* cnm, uint8_t nodeId, const uint8_t mac[6],
                 PlkEdrv* edrv)
{
  if (cnm == NULL || mac == NULL || edrv == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  if (edrv->getMacAddr == NULL || edrv->send == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  memset(cnm, 0, sizeof(*cnm));
  cnm->nodeId = nodeId;
  memcpy(cnm->mac, mac, 6);
  cnm->edrv = edrv;

  plk_od_init(&cnm->od);
  plk_cn_nmt_init(&cnm->sm);

  /* SDO 服务器：绑定本地对象字典与 edrv 发送 */
  s_activeCnm = cnm;
  if (plk_sdo_init(4, &cnm->od) != PLK_ERR_OK) {
    s_activeCnm = NULL;
    return PLK_ERR_INVALID_PARAM;
  }
  plk_sdo_set_local_node(nodeId);
  plk_sdo_set_io(cnm_sdo_send, cnm_sdo_pump);

  return PLK_ERR_OK;
}

void plk_cnm_exit(PlkCnm* cnm)
{
  if (cnm == NULL) {
    return;
  }
  if (s_activeCnm == cnm) {
    s_activeCnm = NULL;
  }
  cnm->started = false;
}

int plk_cnm_start(PlkCnm* cnm)
{
  int ret;

  if (cnm == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  /* 上电事件序列 → NotActive（等待 MN 的 SoC） */
  plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_START_UP);
  plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_SWITCH_ON);
  plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_REQUEST_RESET_COM);
  plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_REQUEST_RESET_CONFIG);
  plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_SWITCH_ON);

  cnm->started = true;
  ret = (cnm->sm.state == PLK_NMT_CS_NOT_ACTIVE) ? PLK_ERR_OK : PLK_ERR_NMT_STATE;
  return ret;
}

int plk_cnm_set_pdo_in(PlkCnm* cnm, uint8_t* data, uint16_t size)
{
  if (cnm == NULL || (size > 0 && data == NULL)) {
    return PLK_ERR_INVALID_PARAM;
  }
  cnm->pollInData = data;
  cnm->pollInSize = size;
  return PLK_ERR_OK;
}

int plk_cnm_set_pdo_out(PlkCnm* cnm, uint8_t* data, uint16_t size,
                        PlkCnmPdoOutCb cb, void* ctx)
{
  if (cnm == NULL || (size > 0 && data == NULL)) {
    return PLK_ERR_INVALID_PARAM;
  }
  cnm->pollOutData = data;
  cnm->pollOutSize = size;
  cnm->pdoOutCb = cb;
  cnm->pdoOutCtx = ctx;
  return PLK_ERR_OK;
}

int plk_cnm_app_ready(PlkCnm* cnm)
{
  if (cnm == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }
  return plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_ENTER_READY_TO_OPERATE);
}

/* 处理 IdentRequest：回发 IdentRes 宣告身份与 PDO 尺寸 */
static void cnm_send_ident_response(PlkCnm* cnm, const PlkFrame* req)
{
  PlkFrame resp;
  PlkIdentResponse ident;
  uint8_t mcast[6];
  uint16_t len;

  memset(&ident, 0, sizeof(ident));
  ident.flag2 = 0x18;                            /* PR | RS */
  ident.nmtStatus = (uint8_t)(cnm->sm.state & 0xFF);
  ident.powerlinkProfileVersion = PLK_PROFILE_VERSION;
  ident.mtuLe = PLK_ASYNC_MTU;
  ident.pollInSizeLe = cnm->pollInSize;
  ident.pollOutSizeLe = cnm->pollOutSize;
  ident.deviceTypeLe = 1;
  ident.vendorIdLe = 0x00000001;
  ident.productCodeLe = 0x00000001;
  ident.revisionNumberLe = 0x00000100;
  ident.serialNumberLe = 0x00000001;

  plk_get_mcast_mac(PLK_MSG_ASND, mcast);
  len = plk_build_asnd(&resp, cnm->mac, mcast, cnm->nodeId, req->srcNodeId,
                       PLK_ASND_IDENT_RESPONSE, (const uint8_t*)&ident,
                       sizeof(ident));
  if (cnm->edrv != NULL && cnm->edrv->send != NULL) {
    cnm->edrv->send(cnm->edrv, (const uint8_t*)&resp, len);
  }
}

int plk_cnm_process_rx(PlkCnm* cnm, const uint8_t* raw, uint16_t len)
{
  const PlkFrame* f;

  if (cnm == NULL || raw == NULL) {
    return PLK_ERR_INVALID_PARAM;
  }

  f = (const PlkFrame*)raw;
  if (plk_frame_validate(f, len) != PLK_ERR_OK) {
    return PLK_ERR_PROTOCOL;
  }

  switch (f->messageType) {
    case PLK_MSG_SOC:
      plk_cn_nmt_process(&cnm->sm, PLK_NMT_EVENT_RECEIVE_SOC);
      break;

    case PLK_MSG_ASND:
      if (f->data.asnd.serviceId == PLK_ASND_NMT_COMMAND) {
        uint8_t cmd = f->data.asnd.payload.nmtCommandService.nmtCommandId;
        plk_cn_nmt_process(&cnm->sm, cnm_cmd_to_event(cmd));
      } else if (f->data.asnd.serviceId == PLK_ASND_IDENT_RESPONSE) {
        if (f->dstNodeId == cnm->nodeId ||
            f->dstNodeId == PLK_ADR_BROADCAST ||
            f->dstNodeId == 0) {
          cnm_send_ident_response(cnm, f);
        }
      } else if (f->data.asnd.serviceId == PLK_ASND_SDO) {
        if (f->dstNodeId == cnm->nodeId ||
            f->dstNodeId == PLK_ADR_BROADCAST ||
            f->dstNodeId == 0) {
          plk_sdo_process_rx(f);
        }
      }
      break;

    case PLK_MSG_PREQ:
      if (f->dstNodeId == cnm->nodeId) {
        cnm_handle_preq(cnm, f);
      }
      break;

    default:
      break;
  }

  return PLK_ERR_OK;
}
