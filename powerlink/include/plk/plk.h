/**
 * plk.h - 手写 openPOWERLINK 协议栈主头文件
 *
 * 基于 Ethernet POWERLINK 协议规范 (EPSG DS 301 V1.2.0 / V1.3.0)
 * 轻量级实现：MN（主站）+ CN（从站）共用同一协议核心
 *
 * 帧格式参考开源参考栈 openPOWERLINK_V2.7.2 (BSD 许可)
 * 本实现为全新手写代码，仅复用公开的帧格式定义
 */

#ifndef PLK_H
#define PLK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 协议版本 ========== */

#define PLK_VERSION_MAJOR           2
#define PLK_VERSION_MINOR           0
#define PLK_VERSION_FULL            0x00020702
#define PLK_PROFILE_VERSION         0x12    /* 主版本 1, 子版本 2 */

/* ========== 基础协议常量 (EPSG DS 301) ========== */

#define PLK_ETHERTYPE               0x88AB  /* POWERLINK EtherType (网络字节序 0xAB88) */
#define PLK_ETH_FRAME_MIN           60      /* 最小以太网帧（不含 CRC） */
#define PLK_ETH_FRAME_MAX           1514    /* 最大以太网帧（不含 CRC） */
#define PLK_ETH_MTU                 1500
#define PLK_ISOCHR_MAX_PAYLOAD      1490    /* PReq/PRes 最大有效载荷 */
#define PLK_ASYNC_MTU               1500    /* 异步帧最大有效载荷 */
#define PLK_MIN_ASYNC_MTU           300
#define PLK_MAX_NODE_ID             254
#define PLK_MAX_RS                  7       /* 最大待发送请求数 */

/* POWERLINK 节点地址 */
#define PLK_ADR_BROADCAST           0xFF
#define PLK_ADR_MN_DEF              0xF0    /* MN 默认节点地址 */
#define PLK_ADR_DIAG_DEF            0xFD
#define PLK_ADR_DUMMY               0xFC
#define PLK_ADR_INVALID             0x00
#define PLK_ADR_RT1_DEF             0xFE

/* 同步触发地址 */
#define PLK_ADR_SYNC_ON_SOC         0x00
#define PLK_ADR_SYNC_ON_SOA         0xFF

/* 多播 MAC 地址 (01:11:1E:00:00:0x) */
#define PLK_MCAST_SOC               0x01111E000001ULL
#define PLK_MCAST_PRES              0x01111E000002ULL
#define PLK_MCAST_SOA               0x01111E000003ULL
#define PLK_MCAST_ASND              0x01111E000004ULL
#define PLK_MCAST_AMNI              0x01111E000005ULL

/* 以太网帧内 POWERLINK 字段偏移 */
#define PLK_FRAME_OFFSET_DST_MAC    0
#define PLK_FRAME_OFFSET_SRC_MAC    6
#define PLK_FRAME_OFFSET_ETH_TYPE   12
#define PLK_FRAME_OFFSET_MSG_TYPE   14
#define PLK_FRAME_OFFSET_DST_NODE   15
#define PLK_FRAME_OFFSET_SRC_NODE   16
#define PLK_FRAME_OFFSET_PDO_PAYLOAD 24

/* 最小帧尺寸 */
#define PLK_MINSIZE_SOC             36
#define PLK_MINSIZE_PREQ            60
#define PLK_MINSIZE_PRES            60
#define PLK_MINSIZE_SOA             54
#define PLK_MINSIZE_IDENTRES        176
#define PLK_MINSIZE_STATUSRES       72
#define PLK_MINSIZE_SYNCRES         44
#define PLK_MINSIZE_NMTCMD          20

/* SDO/UDP 端口 */
#define PLK_SDO_UDP_PORT            3819

/* ========== 帧标志位 ========== */

#define PLK_FRAME_FLAG1_RD          0x01    /* ready (PReq/PRes) */
#define PLK_FRAME_FLAG1_ER          0x02    /* exception reset (SoA) */
#define PLK_FRAME_FLAG1_EA          0x04    /* exception acknowledge (PReq/SoA) */
#define PLK_FRAME_FLAG1_EC          0x08    /* exception clear (StatusRes) */
#define PLK_FRAME_FLAG1_EN          0x10    /* exception new (PRes/StatusRes) */
#define PLK_FRAME_FLAG1_MS          0x20    /* multiplexed slot (PReq) */
#define PLK_FRAME_FLAG1_PS          0x40    /* prescaled slot (SoC) */
#define PLK_FRAME_FLAG1_MC          0x80    /* multiplexed cycle completed (SoC) */

#define PLK_FRAME_FLAG2_RS_MASK     0x07
#define PLK_FRAME_FLAG2_PR_SHIFT    3
#define PLK_FRAME_FLAG3_MR          0x01    /* MN redundancy active (SoA) */

/* ========== 消息类型 ========== */

typedef enum {
  PLK_MSG_NON_POWERLINK = 0x00,
  PLK_MSG_SOC           = 0x01,
  PLK_MSG_PREQ          = 0x03,
  PLK_MSG_PRES          = 0x04,
  PLK_MSG_SOA           = 0x05,
  PLK_MSG_ASND          = 0x06,
  PLK_MSG_AMNI          = 0x07,
  PLK_MSG_AINV          = 0x0D,
} PlkMsgType;

/* ========== ASnd 服务 ID ========== */

typedef enum {
  PLK_ASND_NONE            = 0x00,
  PLK_ASND_IDENT_RESPONSE  = 0x01,
  PLK_ASND_STATUS_RESPONSE = 0x02,
  PLK_ASND_NMT_REQUEST     = 0x03,
  PLK_ASND_NMT_COMMAND     = 0x04,
  PLK_ASND_SDO             = 0x05,
  PLK_ASND_SYNC_RESPONSE   = 0x06,
} PlkAsndServiceId;

/* ========== 异步请求服务 ID (SoA 用) ========== */

typedef enum {
  PLK_REQ_SERVICE_NONE          = 0x00,
  PLK_REQ_SERVICE_IDENT         = 0x01,
  PLK_REQ_SERVICE_STATUS        = 0x02,
  PLK_REQ_SERVICE_NMT_REQUEST   = 0x03,
  PLK_REQ_SERVICE_SYNC          = 0x06,
  PLK_REQ_SERVICE_UNSPECIFIED   = 0xFF,
} PlkReqServiceId;

/* ========== 异步请求优先级 ========== */

typedef enum {
  PLK_ASYNC_PRIO_NMT      = 0x07,
  PLK_ASYNC_PRIO_GENERIC  = 0x03,
  PLK_ASYNC_PRIO_STD      = 0x00,
} PlkAsyncPriority;

/* ========== 错误码 ========== */

typedef enum {
  PLK_ERR_OK             = 0,
  PLK_ERR_INVALID_PARAM  = -1,
  PLK_ERR_NOT_INITIALIZED = -2,
  PLK_ERR_NO_MEMORY      = -3,
  PLK_ERR_UNSUPPORTED    = -4,
  PLK_ERR_BUSY           = -5,
  PLK_ERR_TIMEOUT        = -6,
  PLK_ERR_NO_DATA        = -7,
  PLK_ERR_LINK_DOWN      = -8,
  PLK_ERR_PROTOCOL       = -9,
  PLK_ERR_OD_INDEX       = -10,   /* 对象字典索引不存在 */
  PLK_ERR_OD_SUBINDEX    = -11,   /* 子索引不存在 */
  PLK_ERR_OD_ACCESS      = -12,   /* 非法访问权限 */
  PLK_ERR_OD_SIZE        = -13,   /* 数据长度不匹配 */
  PLK_ERR_SDO_ABORT      = -14,   /* SDO abort 码 */
  PLK_ERR_NMT_STATE      = -15,   /* NMT 状态不允许 */
  PLK_ERR_NODE_NOT_FOUND = -16,   /* 节点未找到 */
} PlkError;

/* ========== 网络字节序辅助 ========== */

static inline uint16_t plk_swap16(uint16_t v)
{
  return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t plk_swap32(uint32_t v)
{
  return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
         ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000);
}

static inline uint64_t plk_swap64(uint64_t v)
{
  return ((uint64_t)plk_swap32((uint32_t)v) << 32) | plk_swap32((uint32_t)(v >> 32));
}

/* POWERLINK 帧内字段均为小端序，EtherType 为大端序 */
#define plk_hton16(v) plk_swap16(v)
#define plk_hton32(v) plk_swap32(v)
#define plk_hton64(v) plk_swap64(v)
#define plk_ntoh16(v) plk_swap16(v)
#define plk_ntoh32(v) plk_swap32(v)
#define plk_ntoh64(v) plk_swap64(v)

#ifdef __cplusplus
}
#endif

#endif /* PLK_H */
