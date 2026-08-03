#ifndef PLC_DEBUG_PROTOCOL_H
#define PLC_DEBUG_PROTOCOL_H

#include <stdint.h>

/* 调试协议 - UART 帧格式 */
#define DBG_FRAME_HEADER0   0xDB
#define DBG_FRAME_HEADER1   0xDC
#define DBG_FRAME_HEADER_LEN 2
#define DBG_FRAME_LEN_FIELD_LEN 1
#define DBG_FRAME_CMD_LEN   1
#define DBG_FRAME_MAX_PAYLOAD 128
#define DBG_FRAME_MAX_LEN   (DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN + DBG_FRAME_CMD_LEN + DBG_FRAME_MAX_PAYLOAD)

/* 命令码 PC → STM32 */
#define DBG_CMD_PING        0x01
#define DBG_CMD_GET_STATUS  0x02
#define DBG_CMD_READ_VAR    0x03
#define DBG_CMD_WRITE_VAR   0x04
#define DBG_CMD_SET_BP      0x05
#define DBG_CMD_REMOVE_BP   0x06
#define DBG_CMD_STEP        0x07
#define DBG_CMD_RUN         0x08
#define DBG_CMD_PAUSE       0x09
#define DBG_CMD_GET_BPS     0x0A

/* 命令码 STM32 → PC */
#define DBG_RSP_PONG        0x81
#define DBG_RSP_STATUS      0x82
#define DBG_RSP_VAR_VALUE   0x83
#define DBG_RSP_VAR_WRITTEN 0x84
#define DBG_RSP_BP_HIT      0x85
#define DBG_RSP_STEPPED     0x86
#define DBG_RSP_LOG         0x87
#define DBG_RSP_ERROR       0x88

/* 构建发送帧（写入缓冲区，返回总长度）*/
static inline uint32_t dbg_build_frame(uint8_t* buf, uint32_t buf_size,
                                       uint8_t cmd, const uint8_t* payload, uint32_t payload_len)
{
  uint32_t frame_len = DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN +
                       DBG_FRAME_CMD_LEN + payload_len;
  if (buf_size < frame_len)
    return 0;
  buf[0] = DBG_FRAME_HEADER0;
  buf[1] = DBG_FRAME_HEADER1;
  buf[2] = (uint8_t)payload_len;
  buf[3] = cmd;
  for (uint32_t i = 0; i < payload_len; i++)
    buf[DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN + DBG_FRAME_CMD_LEN + i] = payload[i];
  return frame_len;
}

/* 解析接收帧（返回负载长度，-1 表示无效帧）*/
static inline int32_t dbg_parse_frame(const uint8_t* buf, uint32_t buf_len,
                                      uint8_t* out_cmd, const uint8_t** out_payload)
{
  if (buf_len < DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN + DBG_FRAME_CMD_LEN)
    return -1;
  if (buf[0] != DBG_FRAME_HEADER0 || buf[1] != DBG_FRAME_HEADER1)
    return -1;
  uint32_t payload_len = buf[2];
  if (buf_len < DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN + DBG_FRAME_CMD_LEN + payload_len)
    return -1;
  *out_cmd = buf[3];
  *out_payload = &buf[DBG_FRAME_HEADER_LEN + DBG_FRAME_LEN_FIELD_LEN + DBG_FRAME_CMD_LEN];
  return (int32_t)payload_len;
}

#endif /* PLC_DEBUG_PROTOCOL_H */