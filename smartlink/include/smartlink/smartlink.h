/**
 * smartlink.h - 手写智能总线协议栈主头文件
 *
 * 基于 Ethernet 智能总线 协议规范 (EPSG DS 301 V1.2.0 / V1.3.0)
 * 轻量级实现：MN（主站）+ CN（从站）共用同一协议核心
 *
 * 帧格式遵循 智能总线 协议规范
 * 本实现为全新手写代码，仅复用公开的帧格式定义
 */

#ifndef SL_H
#define SL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 协议版本 ========== */

#define SL_VERSION_MAJOR           2
#define SL_VERSION_MINOR           0
#define SL_VERSION_FULL            0x00020702
#define SL_PROFILE_VERSION         0x12    /* 主版本 1, 子版本 2 */

/* ========== 基础协议常量 (EPSG DS 301) ========== */

#define SL_ETHERTYPE               0x88AB  /* 智能总线 EtherType (网络字节序 0xAB88) */
#define SL_ETH_FRAME_MIN           60      /* 最小以太网帧（不含 CRC） */
#define SL_ETH_FRAME_MAX           1514    /* 最大以太网帧（不含 CRC） */
#define SL_ETH_MTU                 1500
#define SL_ISOCHR_MAX_PAYLOAD      1490    /* PReq/PRes 最大有效载荷 */
#define SL_ASYNC_MTU               1500    /* 异步帧最大有效载荷 */
#define SL_MIN_ASYNC_MTU           300
#define SL_MAX_NODE_ID             254
#define SL_MAX_RS                  7       /* 最大待发送请求数 */

/* 智能总线 节点地址 */
#define SL_ADR_BROADCAST           0xFF
#define SL_ADR_MN_DEF              0xF0    /* MN 默认节点地址 */
#define SL_ADR_DIAG_DEF            0xFD
#define SL_ADR_DUMMY               0xFC
#define SL_ADR_INVALID             0x00
#define SL_ADR_RT1_DEF             0xFE

/* 同步触发地址 */
#define SL_ADR_SYNC_ON_SOC         0x00
#define SL_ADR_SYNC_ON_SOA         0xFF

/* 多播 MAC 地址 (01:11:1E:00:00:0x) */
#define SL_MCAST_SOC               0x01111E000001ULL
#define SL_MCAST_PRES              0x01111E000002ULL
#define SL_MCAST_SOA               0x01111E000003ULL
#define SL_MCAST_ASND              0x01111E000004ULL
#define SL_MCAST_AMNI              0x01111E000005ULL

/* 以太网帧内 智能总线 字段偏移 */
#define SL_FRAME_OFFSET_DST_MAC    0
#define SL_FRAME_OFFSET_SRC_MAC    6
#define SL_FRAME_OFFSET_ETH_TYPE   12
#define SL_FRAME_OFFSET_MSG_TYPE   14
#define SL_FRAME_OFFSET_DST_NODE   15
#define SL_FRAME_OFFSET_SRC_NODE   16
#define SL_FRAME_OFFSET_PDO_PAYLOAD 24

/* 最小帧尺寸 */
#define SL_MINSIZE_SOC             36
#define SL_MINSIZE_PREQ            60
#define SL_MINSIZE_PRES            60
#define SL_MINSIZE_SOA             54
#define SL_MINSIZE_IDENTRES        176
#define SL_MINSIZE_STATUSRES       72
#define SL_MINSIZE_SYNCRES         44
#define SL_MINSIZE_NMTCMD          20

/* SDO/UDP 端口 */
#define SL_SDO_UDP_PORT            3819

/* ========== 帧标志位 ========== */

#define SL_FRAME_FLAG1_RD          0x01    /* ready (PReq/PRes) */
#define SL_FRAME_FLAG1_ER          0x02    /* exception reset (SoA) */
#define SL_FRAME_FLAG1_EA          0x04    /* exception acknowledge (PReq/SoA) */
#define SL_FRAME_FLAG1_EC          0x08    /* exception clear (StatusRes) */
#define SL_FRAME_FLAG1_EN          0x10    /* exception new (PRes/StatusRes) */
#define SL_FRAME_FLAG1_MS          0x20    /* multiplexed slot (PReq) */
#define SL_FRAME_FLAG1_PS          0x40    /* prescaled slot (SoC) */
#define SL_FRAME_FLAG1_MC          0x80    /* multiplexed cycle completed (SoC) */

#define SL_FRAME_FLAG2_RS_MASK     0x07
#define SL_FRAME_FLAG2_PR_SHIFT    3
#define SL_FRAME_FLAG3_MR          0x01    /* MN redundancy active (SoA) */

/* ========== 消息类型 ========== */

typedef enum {
  SL_MSG_NON_SMARTLINK = 0x00,
  SL_MSG_SOC           = 0x01,
  SL_MSG_PREQ          = 0x03,
  SL_MSG_PRES          = 0x04,
  SL_MSG_SOA           = 0x05,
  SL_MSG_ASND          = 0x06,
  SL_MSG_AMNI          = 0x07,
  SL_MSG_AINV          = 0x0D,
} SlMsgType;

/* ========== ASnd 服务 ID ========== */

typedef enum {
  SL_ASND_NONE            = 0x00,
  SL_ASND_IDENT_RESPONSE  = 0x01,
  SL_ASND_STATUS_RESPONSE = 0x02,
  SL_ASND_NMT_REQUEST     = 0x03,
  SL_ASND_NMT_COMMAND     = 0x04,
  SL_ASND_SDO             = 0x05,
  SL_ASND_SYNC_RESPONSE   = 0x06,
} SlAsndServiceId;

/* ========== 异步请求服务 ID (SoA 用) ========== */

typedef enum {
  SL_REQ_SERVICE_NONE          = 0x00,
  SL_REQ_SERVICE_IDENT         = 0x01,
  SL_REQ_SERVICE_STATUS        = 0x02,
  SL_REQ_SERVICE_NMT_REQUEST   = 0x03,
  SL_REQ_SERVICE_SYNC          = 0x06,
  SL_REQ_SERVICE_UNSPECIFIED   = 0xFF,
} SlReqServiceId;

/* ========== 异步请求优先级 ========== */

typedef enum {
  SL_ASYNC_PRIO_NMT      = 0x07,
  SL_ASYNC_PRIO_GENERIC  = 0x03,
  SL_ASYNC_PRIO_STD      = 0x00,
} SlAsyncPriority;

/* ========== 错误码 ========== */

typedef enum {
  SL_ERR_OK             = 0,
  SL_ERR_INVALID_PARAM  = -1,
  SL_ERR_NOT_INITIALIZED = -2,
  SL_ERR_NO_MEMORY      = -3,
  SL_ERR_UNSUPPORTED    = -4,
  SL_ERR_BUSY           = -5,
  SL_ERR_TIMEOUT        = -6,
  SL_ERR_NO_DATA        = -7,
  SL_ERR_LINK_DOWN      = -8,
  SL_ERR_PROTOCOL       = -9,
  SL_ERR_OD_INDEX       = -10,   /* 对象字典索引不存在 */
  SL_ERR_OD_SUBINDEX    = -11,   /* 子索引不存在 */
  SL_ERR_OD_ACCESS      = -12,   /* 非法访问权限 */
  SL_ERR_OD_SIZE        = -13,   /* 数据长度不匹配 */
  SL_ERR_SDO_ABORT      = -14,   /* SDO abort 码 */
  SL_ERR_NMT_STATE      = -15,   /* NMT 状态不允许 */
  SL_ERR_NODE_NOT_FOUND = -16,   /* 节点未找到 */
} SlError;

/* ========== 网络字节序辅助 ========== */

static inline uint16_t sl_swap16(uint16_t v)
{
  return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t sl_swap32(uint32_t v)
{
  return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
         ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000);
}

static inline uint64_t sl_swap64(uint64_t v)
{
  return ((uint64_t)sl_swap32((uint32_t)v) << 32) | sl_swap32((uint32_t)(v >> 32));
}

/* 智能总线 帧内字段均为小端序，EtherType 为大端序 */
#define sl_hton16(v) sl_swap16(v)
#define sl_hton32(v) sl_swap32(v)
#define sl_hton64(v) sl_swap64(v)
#define sl_ntoh16(v) sl_swap16(v)
#define sl_ntoh32(v) sl_swap32(v)
#define sl_ntoh64(v) sl_swap64(v)

#ifdef __cplusplus
}
#endif

#endif /* SL_H */
