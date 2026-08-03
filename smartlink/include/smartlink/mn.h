/**
 * mn.h - MN（管理节点/主站）协议栈实例
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.4 ~ 4.6 章。
 * MN 是总线上的唯一主站，负责：
 *   - 启动流程：通信复位、节点发现（IdentRequest/IdentRes）、节点配置
 *   - 周期调度：SoC → 各 CN 的 PReq/PRes → SoA 异步阶段
 *   - 节点监控：跟踪各 CN 的 NMT 状态与连接状态
 *
 * 帧收发经 SlEdrv 抽象层；节点信息存于 DLL 节点表（sl_dll_*）。
 * 应用接入点：
 *   - sl_mn_set_pdo_out  绑定输出 PDO 缓冲（PReq 载荷，MN→CN）
 *   - sl_mn_set_pdo_in   绑定输入 PDO 缓冲（PRes 载荷，CN→MN）
 *   - sl_mn_set_callbacks 注册节点发现 / 输入 PDO / 应用就绪回调
 */

#ifndef SL_MN_H
#define SL_MN_H

#include "smartlink.h"
#include "frame.h"
#include "dll.h"
#include "edrv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== MN 状态 ========== */

typedef enum
{
  SL_MN_STATE_OFFLINE = 0,  /* 未初始化 */
  SL_MN_STATE_BOOTING,      /* 启动中（发现/配置节点） */
  SL_MN_STATE_OPERATIONAL,  /* 周期运行中 */
  SL_MN_STATE_STOPPED,      /* 已停止（周期挂起） */
} SlMnState;

/* ========== 启动阶段 ========== */

typedef enum
{
  SL_MN_BOOT_IDLE = 0,      /* 未启动 */
  SL_MN_BOOT_RESET_COM,     /* 广播通信复位 */
  SL_MN_BOOT_DISCOVER,      /* 节点发现 */
  SL_MN_BOOT_CONFIG,        /* 配置节点到 Operational */
  SL_MN_BOOT_DONE,          /* 启动完成 */
  SL_MN_BOOT_FAILED,        /* 启动失败 */
} SlMnBootPhase;

/* ========== 回调 ========== */

struct SlMn;   /* 前向声明：供回调类型引用，避免参数列表内声明作用域受限 */

/**
 * 节点发现回调：收到 CN 的 IdentRes 后调用。
 * @return 0 接受该节点并纳入管理；负值忽略该节点
 */
typedef int (*SlMnNodeFoundCb)(struct SlMn* mn,
                                const SlDllNodeInfo* info, void* ctx);

/**
 * 输入 PDO 回调：收到 CN 的 PRes 载荷后调用。
 * @param nodeId 源节点 ID
 */
typedef int (*SlMnPdoInCb)(struct SlMn* mn, uint8_t nodeId,
                            const uint8_t* data, uint16_t size, void* ctx);

/**
 * 应用就绪查询回调：ReadyToOperate 双标志握手中，MN 询问应用层
 * 目标 CN 是否已就绪（对应 CN 侧 sl_cnm_app_ready）。
 * @param ready 输出：CN 应用是否就绪
 * @return 0 查询成功；负值表示暂时无法判断（重试）
 */
typedef int (*SlMnAppReadyCb)(struct SlMn* mn, uint8_t nodeId,
                               bool* ready, void* ctx);

/* ========== MN 上下文 ========== */

typedef struct SlMn
{
  uint8_t  nodeId;               /* 本机节点 ID（默认 0xF0） */
  uint8_t  mac[6];               /* 本机 MAC 地址 */
  SlEdrv* edrv;                 /* 绑定的以太网驱动 */

  SlMnState     state;          /* MN 总体状态 */
  SlMnBootPhase bootPhase;      /* 启动阶段 */

  SlCycleParam  cycleParam;     /* 周期参数 */
  SlAsyncParam  asyncParam;     /* 异步阶段参数 */

  uint32_t cycleCount;           /* 已运行周期数 */
  uint16_t bootTimeoutMs;        /* 启动单步超时 */
  uint16_t presTimeoutUs;        /* PRes 等待超时（周期内） */

  uint8_t rxPresNode;            /* 最近收到 PRes 的源节点（周期内等待用） */

  uint8_t* pollOutData;          /* 输出 PDO 缓冲（PReq 载荷，MN→CN） */
  uint16_t pollOutSize;
  uint8_t* pollInData;           /* 输入 PDO 缓冲（PRes 载荷，CN→MN） */
  uint16_t pollInSize;

  SlMnNodeFoundCb  onNodeFound; /* 节点发现回调 */
  SlMnPdoInCb      onPdoIn;     /* 输入 PDO 回调 */
  SlMnAppReadyCb   appReady;    /* 应用就绪查询回调 */
  void* appCtx;                  /* 回调上下文 */
} SlMn;

/* ========== 接口 ========== */

/**
 * 初始化 MN 栈：绑定节点地址、驱动，初始化 DLL 节点表。
 * @param nodeId 本机节点 ID（通常 SL_ADR_MN_DEF）
 * @param mac    本机 MAC 地址（6 字节）
 * @param edrv   绑定的以太网驱动（须支持收发、链路查询）
 */
int sl_mn_init(SlMn* mn, uint8_t nodeId, const uint8_t mac[6], SlEdrv* edrv);

/**
 * 反初始化 MN 栈。
 */
void sl_mn_exit(SlMn* mn);

/**
 * 启动流程：通信复位 → 节点发现 → 节点配置到 Operational → 进入周期运行。
 * 内部阻塞推进（经 edrv 收发帧），ReadyToOperate 握手通过 appReady 回调查询。
 * @return 0 启动成功进入 Operational；SL_ERR_TIMEOUT 发现/配置超时
 */
int sl_mn_start(SlMn* mn);

/**
 * 停止周期运行（挂起周期通信），节点保留在节点表。
 */
int sl_mn_stop(SlMn* mn);

/**
 * 执行一个完整周期：SoC → 对每个已连节点发 PReq → 收集 PRes → SoA。
 * 周期时序由调用方掌握（定时器/主循环）。
 * @return SL_ERR_OK；SL_ERR_LINK_DOWN 链路断开
 */
int sl_mn_cycle(SlMn* mn);

/**
 * 处理接收到的以太网帧（edrv 接收回调入口）。
 * 分发 IdentRes / PRes / ASnd 等。
 */
int sl_mn_process_rx(SlMn* mn, const uint8_t* raw, uint16_t len);

/**
 * 绑定输出 PDO 缓冲（PReq 载荷内容，MN→CN）。
 */
int sl_mn_set_pdo_out(SlMn* mn, uint8_t* data, uint16_t size);

/**
 * 绑定输入 PDO 缓冲（PRes 载荷内容，CN→MN）。
 */
int sl_mn_set_pdo_in(SlMn* mn, uint8_t* data, uint16_t size);

/**
 * 注册回调（节点发现 / 输入 PDO / 应用就绪查询）。
 */
int sl_mn_set_callbacks(SlMn* mn, SlMnNodeFoundCb nodeFound,
                         SlMnPdoInCb pdoIn, SlMnAppReadyCb appReady,
                         void* ctx);

/**
 * 向 DLL 节点表注册一个预先配置的节点（替代自动发现，或追加静态节点）。
 */
int sl_mn_register_node(SlMn* mn, const SlDllNodeInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* SL_MN_H */
