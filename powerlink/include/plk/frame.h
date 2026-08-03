/**
 * frame.h - POWERLINK 帧结构定义
 *
 * 帧布局依据 EPSG DS 301 V1.2.0 第 4.6.1.1 章"Basic Frame Format"
 * 所有帧字段均为小端序（EtherType 除外，为大端序）。
 * 帧偏移：dst_mac(0) src_mac(6) eth_type(12) msg_type(14) dst_node(15) src_node(16)，
 * 协议数据从 offset 17 开始。
 *
 * 帧格式遵循 POWERLINK 协议规范，本文件为全新手写实现。
 */

#ifndef PLK_FRAME_H
#define PLK_FRAME_H

#include "plk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 字节对齐支持 ========== */

/* GCC 中 `} __attribute__((packed)) Name;` 只作用于 typedef 名称而不打包结构体，
 * 因此对 GCC/Clang 同样使用 #pragma pack(push, 1) 强制 1 字节对齐，
 * 并保留 __attribute__((packed)) 作为兜底。 */
#if defined(_MSC_VER)
  #pragma pack(push, 1)
  #define PLK_PACKED
#elif defined(__GNUC__)
  #pragma pack(push, 1)
  #define PLK_PACKED __attribute__((packed))
#else
  #pragma pack(push, 1)
  #define PLK_PACKED
#endif

/* ========== POWERLINK 版本 ========== */

#define PLK_VERSION_SUB   0x0F    /* 子版本掩码 */
#define PLK_VERSION_MAIN  0xF0    /* 主版本掩码 */

/* 帧标志位（PLK_FRAME_FLAG1 系列等）已在 plk.h 定义 */

/* ========== SDO 命令层头尺寸 ========== */

#define PLK_SDO_CMDL_HDR_FIXED_SIZE           8   /* 固定头大小 */
#define PLK_SDO_CMDL_HDR_VAR_SIZE             4   /* 可变头大小 */
#define PLK_SDO_CMDL_HDR_WRITEBYINDEX_SIZE    4   /* WriteByIndex 头 (index+subindex+reserved) */
#define PLK_SDO_CMDL_HDR_READBYINDEX_SIZE     4   /* ReadByIndex 头 */
#define PLK_SDO_CMDL_HDR_WRITEMULTBYINDEX_SIZE 8  /* WriteMultByIndex 子头 */

/* ========== SDO 命令层标志 ========== */

#define PLK_SDO_CMDL_FLAG_RESPONSE    0x80
#define PLK_SDO_CMDL_FLAG_ABORT       0x40
#define PLK_SDO_CMDL_FLAG_EXPEDITED   0x00
#define PLK_SDO_CMDL_FLAG_SEGMINIT    0x10
#define PLK_SDO_CMDL_FLAG_SEGMENTED   0x20
#define PLK_SDO_CMDL_FLAG_SEGMCOMPL   0x30
#define PLK_SDO_CMDL_FLAG_SEGM_MASK   0x30
#define PLK_SDO_CMDL_FLAG_PADSIZE_MASK 0x03

/* ========== NMT 命令数据标志 ========== */

#define PLK_NMT_CMD_DATA_FLAG_DELAY   0x01   /* NMT GoToStandby 含 MNSwitchOverDelay */

/* ========== 错误条目类型 ========== */

#define PLK_ERR_ENTRYTYPE_STATUS       0x8000
#define PLK_ERR_ENTRYTYPE_HISTORY      0x0000
#define PLK_ERR_ENTRYTYPE_EMCY         0x4000
#define PLK_ERR_ENTRYTYPE_MODE_ACTIVE  0x1000
#define PLK_ERR_ENTRYTYPE_MODE_CLEARED 0x2000
#define PLK_ERR_ENTRYTYPE_MODE_OCCURRED 0x3000
#define PLK_ERR_ENTRYTYPE_MODE_MASK    0x3000
#define PLK_ERR_ENTRYTYPE_PROF_VENDOR  0x0001
#define PLK_ERR_ENTRYTYPE_PROF_PLK     0x0002
#define PLK_ERR_ENTRYTYPE_PROF_MASK    0x0FFF

/* ========== SyncControl / SyncStatus 位 ========== */

#define PLK_SYNC_PRES_TIME_FIRST_VALID        0x00000001
#define PLK_SYNC_PRES_TIME_SECOND_VALID       0x00000002
#define PLK_SYNC_SYNC_MN_DELAY_FIRST_VALID    0x00000004
#define PLK_SYNC_SYNC_MN_DELAY_SECOND_VALID   0x00000008
#define PLK_SYNC_PRES_FALL_BACK_TIMEOUT_VALID 0x00000010
#define PLK_SYNC_DEST_MAC_ADDRESS_VALID       0x00000020
#define PLK_SYNC_PRES_MODE_RESET              0x40000000
#define PLK_SYNC_PRES_MODE_SET                0x80000000

/* ========== 帧结构 ========== */

/**
 * 网络时间戳：秒 + 纳秒（小端）
 */
typedef struct
{
  uint32_t sec;    /* 秒 */
  uint32_t nsec;   /* 纳秒 */
} PLK_PACKED PlkNetTime;

/**
 * 周期起始帧 SoC (Start of Cycle)
 * MN 在每个周期开始向所有节点多播发送。
 */
typedef struct
{
  uint8_t      reserved1;       /* 保留 (offset 17) */
  uint8_t      flag1;           /* MC / PS 标志 (offset 18) */
  uint8_t      flag2;           /* 保留 (offset 19) */
  PlkNetTime   netTimeLe;       /* 可选：网络起始时间 (offset 20) */
  uint64_t     relativeTimeLe;  /* 可选：相对时间 us (offset 28) */
} PLK_PACKED PlkSocFrame;

/**
 * 轮询请求帧 PReq (Poll Request)
 * MN 周期性地向单个 CN 单播发送。
 */
typedef struct
{
  uint8_t   reserved1;       /* 保留 (offset 17) */
  uint8_t   flag1;           /* MS / EA / RD 标志 (offset 18) */
  uint8_t   flag2;           /* 保留 (offset 19) */
  uint8_t   pdoVersion;      /* PDO 版本 (offset 20) */
  uint8_t   reserved2;       /* 保留 (offset 21) */
  uint16_t  sizeLe;          /* 有效载荷字节数 (offset 22) */
  uint8_t   aPayload[256];   /* 载荷 (offset 24) */
} PLK_PACKED PlkPreqFrame;

/**
 * 轮询响应帧 PRes (Poll Response)
 * CN 收到 PReq 后多播发送，MN 的 PRes 则单播。
 */
typedef struct
{
  uint8_t   nmtStatus;       /* NMT 状态 (offset 17) */
  uint8_t   flag1;           /* MS / EN / RD 标志 (offset 18) */
  uint8_t   flag2;           /* PR / RS 标志 (offset 19) */
  uint8_t   pdoVersion;      /* PDO 版本 (offset 20) */
  uint8_t   reserved2;       /* 保留 (offset 21) */
  uint16_t  sizeLe;          /* 有效载荷字节数 (offset 22) */
  uint8_t   aPayload[256];   /* 载荷 (offset 24) */
} PLK_PACKED PlkPresFrame;

/**
 * 同步请求 SyncReq
 * PollResponse Chaining 模式下 SoA 的特殊形式 (EPSG DS 302-C)。
 */
typedef struct
{
  uint32_t syncControlLe;         /* Sync 控制位 (offset 24) */
  uint32_t presTimeFirstLe;       /* PRes 响应时间 [ns] (offset 28) */
  uint32_t presTimeSecondLe;      /* 环冗余第二方向响应时间 (offset 32) */
  uint32_t syncMnDelayFirstLe;    /* MN-CN 传播延迟 [ns] (offset 36) */
  uint32_t syncMnDelaySecondLe;   /* 环冗余第二方向延迟 (offset 40) */
  uint32_t presFallBackTimeoutLe; /* 周期监控回退超时 (offset 44) */
  uint8_t  aDestMacAddress[6];    /* 目标节点 MAC (offset 48) */
} PLK_PACKED PlkSyncRequest;

/**
 * SoA 载荷联合
 */
typedef union
{
  PlkSyncRequest syncRequest;    /* PollResponse Chaining 模式使用 (offset 24) */
} PlkSoaPayload;

/**
 * 异步周期起始帧 SoA (Start of Asynchronous)
 * 宣告异步阶段及被授权发送的节点。
 */
typedef struct
{
  uint8_t       nmtStatus;         /* MN 的 NMT 状态 (offset 17) */
  uint8_t       flag1;             /* EA / ER 标志 (offset 18) */
  uint8_t       flag2;             /* 保留 (offset 19) */
  uint8_t       reqServiceId;      /* 本异步槽对应的服务 ID (offset 20) */
  uint8_t       reqServiceTarget;  /* 允许发送的节点地址 (offset 21) */
  uint8_t       powerlinkVersion;  /* MN 的 POWERLINK 版本 (offset 22) */
  uint8_t       flag3;             /* MN 冗余激活标志 (offset 23) */
  PlkSoaPayload payload;           /* SoA 载荷 (offset 24) */
} PLK_PACKED PlkSoaFrame;

/**
 * 错误历史条目
 */
typedef struct
{
  uint16_t   entryType;     /* 条目类型 (offset 0) */
  uint16_t   errorCode;     /* 错误码 (offset 2) */
  PlkNetTime timeStamp;     /* 时间戳 (offset 4) */
  uint8_t    aAddInfo[8];   /* 附加错误信息 (offset 12) */
} PLK_PACKED PlkErrHistoryEntry;

/**
 * 状态响应 StatusRes
 * CN 对 MN 状态查询 (StatusRequest) 的应答。
 */
typedef struct
{
  uint8_t            flag1;               /* EN / EC 标志 (offset 18) */
  uint8_t            flag2;               /* PR / RS 标志 (offset 19) */
  uint8_t            nmtStatus;           /* CN 的 NMT 状态 (offset 20) */
  uint8_t            reserved1[3];        /* 保留 (offset 21) */
  uint64_t           staticErrorLe;       /* 静态错误位图 (offset 24) */
  PlkErrHistoryEntry aErrorHistoryEntry[13]; /* 错误历史列表 (offset 32) */
} PLK_PACKED PlkStatusResponse;

/**
 * 识别响应 IdentRes
 * CN 对 IdentRequest 的应答，宣告身份与特性。
 */
typedef struct
{
  uint8_t  flag1;                     /* 保留 (offset 18) */
  uint8_t  flag2;                     /* PR / RS 标志 (offset 19) */
  uint8_t  nmtStatus;                 /* CN 的 NMT 状态 (offset 20) */
  uint8_t  identResponseFlags;        /* 识别响应标志 (offset 21) */
  uint8_t  powerlinkProfileVersion;   /* CN 的协议版本 (offset 22) */
  uint8_t  reserved1;                 /* 保留 (offset 23) */
  uint32_t featureFlagsLe;            /* 特性标志 (offset 24) */
  uint16_t mtuLe;                     /* 最大异步帧尺寸 (offset 28) */
  uint16_t pollInSizeLe;              /* 轮询输入尺寸 (offset 30) */
  uint16_t pollOutSizeLe;             /* 轮询输出尺寸 (offset 32) */
  uint32_t responseTimeLe;            /* 最大响应时间 [ns] (offset 34) */
  uint16_t reserved2;                 /* 保留 (offset 38) */
  uint32_t deviceTypeLe;              /* 设备类型 (offset 40) */
  uint32_t vendorIdLe;                /* 厂商 ID (offset 44) */
  uint32_t productCodeLe;             /* 产品码 (offset 48) */
  uint32_t revisionNumberLe;          /* 修订号 (offset 52) */
  uint32_t serialNumberLe;            /* 序列号 (offset 56) */
  uint64_t vendorSpecificExt1Le;      /* 厂商扩展 1 (offset 60) */
  uint32_t verifyConfigurationDateLe; /* 配置校验日期 (offset 68) */
  uint32_t verifyConfigurationTimeLe; /* 配置校验时间 (offset 72) */
  uint32_t applicationSwDateLe;       /* 应用软件日期 (offset 76) */
  uint32_t applicationSwTimeLe;       /* 应用软件时间 (offset 80) */
  uint32_t ipAddressLe;               /* IP 地址 (offset 84) */
  uint32_t subnetMaskLe;              /* 子网掩码 (offset 88) */
  uint32_t defaultGatewayLe;          /* 默认网关 (offset 92) */
  uint8_t  sHostName[32];             /* DNS 主机名 (offset 96) */
  uint8_t  aVendorSpecificExt2[48];   /* 厂商扩展 2 (offset 128) */
} PLK_PACKED PlkIdentResponse;

/**
 * NMT 命令服务 (ASnd)
 * MN 使用 NMT 状态命令控制 CN 状态机。
 */
typedef struct
{
  uint8_t nmtCommandId;        /* NMT 命令 ID (offset 18) */
  uint8_t reserved1;           /* 保留 (offset 19) */
  uint8_t aNmtCommandData[32]; /* 命令数据 (offset 20) */
} PLK_PACKED PlkNmtCommandService;

/**
 * 同步响应 SyncRes
 * PollResponse Chaining 模式下 ASnd 帧的一种形式 (EPSG DS 302-C)。
 */
typedef struct
{
  uint16_t reserved;           /* 保留 (offset 18) */
  uint32_t syncStatusLe;       /* Sync 状态位 (offset 20) */
  uint32_t latencyLe;          /* PRes 延迟 [ns] (offset 24) */
  uint32_t syncNodeNumberLe;   /* 最近接收的节点号 (offset 28) */
  uint32_t syncDelayLe;        /* 收发时间差 [ns] (offset 32) */
  uint32_t presTimeFirstLe;    /* PRes 响应时间 1 (offset 36) */
  uint32_t presTimeSecondLe;   /* PRes 响应时间 2 (offset 40) */
} PLK_PACKED PlkSyncResponse;

/**
 * SDO 命令层协议 (SDO Command Layer)
 * 固定部分 8 字节 + 变量头 + 命令数据。
 */
typedef struct
{
  uint8_t  reserved1;       /* 保留 (offset 0) */
  uint8_t  transactionId;   /* 事务 ID (offset 1) */
  uint8_t  flags;           /* Rsp/Abort/Seg 标志 (offset 2) */
  uint8_t  commandId;       /* 命令 ID (offset 3) */
  uint16_t segmentSizeLe;   /* 分段大小 (offset 4) */
  uint16_t reserved2;       /* 保留 (offset 6) */
  uint8_t  aCommandData[8]; /* 命令数据占位 (offset 8) */
} PLK_PACKED PlkAsySdoCom;

/**
 * WriteMultParam 请求 / ReadMultParam 响应子头 (V1.3.0)
 */
typedef struct
{
  uint32_t byteOffsetNext;   /* 下一个数据集偏移，0 表示最后 (offset 0) */
  uint16_t index;            /* 对象字典索引 (offset 4) */
  uint8_t  subIndex;         /* 子索引 (offset 6) */
  uint8_t  info;             /* 保留 + 填充位，多位读时 MSB 为子中止标志 (offset 7) */
  uint8_t  aCommandData[4];  /* 载荷或子中止码 (offset 8) */
} PLK_PACKED PlkAsySdoComMultWriteReqReadResp;

/**
 * WriteMultParam 响应子头
 */
typedef struct
{
  uint16_t index;            /* 对象字典索引 (offset 0) */
  uint8_t  subIndex;         /* 子索引 (offset 2) */
  uint8_t  abortFlag;        /* 1 位 MSB：0 成功 1 中止 (offset 3) */
  uint32_t subAbortCode;     /* 子中止原因码 (offset 4) */
} PLK_PACKED PlkAsySdoComWriteMultResp;

/**
 * ReadMultParam 请求子头
 */
typedef struct
{
  uint16_t index;            /* 对象字典索引 (offset 0) */
  uint8_t  subIndex;         /* 子索引 (offset 2) */
  uint8_t  reserved;         /* 对齐保留 (offset 3) */
} PLK_PACKED PlkAsySdoComReadMultReq;

/**
 * SDO 序列层头 (SDO Sequence Layer)
 * 提供可靠、有序、不丢失不重复的连接。
 */
typedef struct
{
  uint8_t     recvSeqNumCon;   /* 最近正确接收帧的序列号 (offset 0) */
  uint8_t     sendSeqNumCon;   /* 本端序列号，每帧 +1 (offset 1) */
  uint8_t     aReserved[2];    /* 保留 (offset 2) */
  PlkAsySdoCom sdoSeqPayload;  /* SDO 载荷 (offset 4) */
} PLK_PACKED PlkAsySdoSeq;

/**
 * NMT 请求服务
 * CN 收到 SoA 的 NMTRequestInvite 后发起。
 */
typedef struct
{
  uint8_t nmtCommandId;        /* NMT 命令 ID (offset 18) */
  uint8_t targetNodeId;        /* 目标节点 ID (offset 19) */
  uint8_t aNmtCommandData[32]; /* 命令数据 (offset 20) */
} PLK_PACKED PlkNmtRequestService;

/**
 * ASnd 载荷联合
 */
typedef union
{
  PlkStatusResponse     statusResponse;    /* StatusRequest 应答 */
  PlkIdentResponse      identResponse;     /* IdentRequest 应答 */
  PlkNmtCommandService  nmtCommandService; /* MN 的 NMT 命令 */
  PlkNmtRequestService  nmtRequestService; /* CN 的 NMT 请求 */
  PlkAsySdoSeq          sdoSequenceFrame;  /* SDO 传输 (ASnd 方式) */
  PlkSyncResponse       syncResponse;      /* SyncRes */
  uint8_t               aPayload[256];
} PlkAsndPayload;

/**
 * 异步发送帧 ASnd (Asynchronous Send)
 */
typedef struct
{
  uint8_t       serviceId;   /* 异步槽服务 ID (offset 17) */
  PlkAsndPayload payload;    /* 服务数据 (offset 18) */
} PLK_PACKED PlkAsndFrame;

/**
 * 帧数据联合：各类 POWERLINK 消息
 */
typedef union
{
  PlkSocFrame  soc;    /* SoC (多播) */
  PlkPreqFrame preq;   /* PReq (单播) */
  PlkPresFrame pres;   /* PRes (多播) */
  PlkSoaFrame  soa;    /* SoA (多播) */
  PlkAsndFrame asnd;   /* ASnd (多播) */
} PlkFrameData;

/**
 * POWERLINK 基本帧
 * 以太网封装：14 字节以太网头 + 协议数据 + 4 字节 CRC32。
 */
typedef struct
{
  uint8_t      aDstMac[6];    /* 目标 MAC (offset 0) */
  uint8_t      aSrcMac[6];    /* 源 MAC (offset 6) */
  uint16_t     etherType;     /* 以太网类型（大端序）(offset 12) */
  uint8_t      messageType;   /* 消息类型 (PlkMsgType 值, offset 14) */
  uint8_t      dstNodeId;     /* 目标节点 ID (offset 15) */
  uint8_t      srcNodeId;     /* 源节点 ID (offset 16) */
  PlkFrameData data;          /* 帧数据 (offset 17) */
} PLK_PACKED PlkFrame;

/* 恢复字节对齐 */
#if defined(_MSC_VER)
  #pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* PLK_FRAME_H */
