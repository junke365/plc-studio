/**
 * mn.h - MN（管理节点/主站）协议栈实例
 *
 * 依据 EPSG DS 301 V1.2.0 第 4.4 ~ 4.6 章。
 * MN 是总线上的唯一主站，负责：
 *   - 启动流程：通信复位、节点发现（IdentRequest/IdentRes）、节点配置
 *   - 周期调度：SoC → 各 CN 的 PReq/PRes → SoA 异步阶段
 *   - 节点监控：跟踪各 CN 的 NMT 状态与连接状态
 *
 * 帧收发经 PlkEdrv 抽象层；节点信息存于 DLL 节点表（plk_dll_*）。
 * 应用接入点：
 *   - plk_mn_set_pdo_out  绑定输出 PDO 缓冲（PReq 载荷，MN→CN）
 *   - plk_mn_set_pdo_in   绑定输入 PDO 缓冲（PRes 载荷，CN→MN）
 *   - plk_mn_set_callbacks 注册节点发现 / 输入 PDO / 应用就绪回调
 */

#ifndef PLK_MN_H
#define PLK_MN_H

#include "plk.h"
#include "frame.h"
#include "dll.h"
#include "edrv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== MN 状态 ========== */

typedef enum
{
  PLK_MN_STATE_OFFLINE = 0,  /* 未初始化 */
  PLK_MN_STATE_BOOTING,      /* 启动中（发现/配置节点） */
  PLK_MN_STATE_OPERATIONAL,  /* 周期运行中 */
  PLK_MN_STATE_STOPPED,      /* 已停止（周期挂起） */
} PlkMnState;

/* ========== 启动阶段 ========== */

typedef enum
{
  PLK_MN_BOOT_IDLE = 0,      /* 未启动 */
  PLK_MN_BOOT_RESET_COM,     /* 广播通信复位 */
  PLK_MN_BOOT_DISCOVER,      /* 节点发现 */
  PLK_MN_BOOT_CONFIG,        /* 配置节点到 Operational */
  PLK_MN_BOOT_DONE,          /* 启动完成 */
  PLK_MN_BOOT_FAILED,        /* 启动失败 */
} PlkMnBootPhase;

/* ========== 回调 ========== */

struct PlkMn;   /* 前向声明：供回调类型引用，避免参数列表内声明作用域受限 */

/**
 * 节点发现回调：收到 CN 的 IdentRes 后调用。
 * @return 0 接受该节点并纳入管理；负值忽略该节点
 */
typedef int (*PlkMnNodeFoundCb)(struct PlkMn* mn,
                                const PlkDllNodeInfo* info, void* ctx);

/**
 * 输入 PDO 回调：收到 CN 的 PRes 载荷后调用。
 * @param nodeId 源节点 ID
 */
typedef int (*PlkMnPdoInCb)(struct PlkMn* mn, uint8_t nodeId,
                            const uint8_t* data, uint16_t size, void* ctx);

/**
 * 应用就绪查询回调：ReadyToOperate 双标志握手中，MN 询问应用层
 * 目标 CN 是否已就绪（对应 CN 侧 plk_cnm_app_ready）。
 * @param ready 输出：CN 应用是否就绪
 * @return 0 查询成功；负值表示暂时无法判断（重试）
 */
typedef int (*PlkMnAppReadyCb)(struct PlkMn* mn, uint8_t nodeId,
                               bool* ready, void* ctx);

/* ========== MN 上下文 ========== */

typedef struct PlkMn
{
  uint8_t  nodeId;               /* 本机节点 ID（默认 0xF0） */
  uint8_t  mac[6];               /* 本机 MAC 地址 */
  PlkEdrv* edrv;                 /* 绑定的以太网驱动 */

  PlkMnState     state;          /* MN 总体状态 */
  PlkMnBootPhase bootPhase;      /* 启动阶段 */

  PlkCycleParam  cycleParam;     /* 周期参数 */
  PlkAsyncParam  asyncParam;     /* 异步阶段参数 */

  uint32_t cycleCount;           /* 已运行周期数 */
  uint16_t bootTimeoutMs;        /* 启动单步超时 */
  uint16_t presTimeoutUs;        /* PRes 等待超时（周期内） */

  uint8_t rxPresNode;            /* 最近收到 PRes 的源节点（周期内等待用） */

  uint8_t* pollOutData;          /* 输出 PDO 缓冲（PReq 载荷，MN→CN） */
  uint16_t pollOutSize;
  uint8_t* pollInData;           /* 输入 PDO 缓冲（PRes 载荷，CN→MN） */
  uint16_t pollInSize;

  PlkMnNodeFoundCb  onNodeFound; /* 节点发现回调 */
  PlkMnPdoInCb      onPdoIn;     /* 输入 PDO 回调 */
  PlkMnAppReadyCb   appReady;    /* 应用就绪查询回调 */
  void* appCtx;                  /* 回调上下文 */
} PlkMn;

/* ========== 接口 ========== */

/**
 * 初始化 MN 栈：绑定节点地址、驱动，初始化 DLL 节点表。
 * @param nodeId 本机节点 ID（通常 PLK_ADR_MN_DEF）
 * @param mac    本机 MAC 地址（6 字节）
 * @param edrv   绑定的以太网驱动（须支持收发、链路查询）
 */
int plk_mn_init(PlkMn* mn, uint8_t nodeId, const uint8_t mac[6], PlkEdrv* edrv);

/**
 * 反初始化 MN 栈。
 */
void plk_mn_exit(PlkMn* mn);

/**
 * 启动流程：通信复位 → 节点发现 → 节点配置到 Operational → 进入周期运行。
 * 内部阻塞推进（经 edrv 收发帧），ReadyToOperate 握手通过 appReady 回调查询。
 * @return 0 启动成功进入 Operational；PLK_ERR_TIMEOUT 发现/配置超时
 */
int plk_mn_start(PlkMn* mn);

/**
 * 停止周期运行（挂起周期通信），节点保留在节点表。
 */
int plk_mn_stop(PlkMn* mn);

/**
 * 执行一个完整周期：SoC → 对每个已连节点发 PReq → 收集 PRes → SoA。
 * 周期时序由调用方掌握（定时器/主循环）。
 * @return PLK_ERR_OK；PLK_ERR_LINK_DOWN 链路断开
 */
int plk_mn_cycle(PlkMn* mn);

/**
 * 处理接收到的以太网帧（edrv 接收回调入口）。
 * 分发 IdentRes / PRes / ASnd 等。
 */
int plk_mn_process_rx(PlkMn* mn, const uint8_t* raw, uint16_t len);

/**
 * 绑定输出 PDO 缓冲（PReq 载荷内容，MN→CN）。
 */
int plk_mn_set_pdo_out(PlkMn* mn, uint8_t* data, uint16_t size);

/**
 * 绑定输入 PDO 缓冲（PRes 载荷内容，CN→MN）。
 */
int plk_mn_set_pdo_in(PlkMn* mn, uint8_t* data, uint16_t size);

/**
 * 注册回调（节点发现 / 输入 PDO / 应用就绪查询）。
 */
int plk_mn_set_callbacks(PlkMn* mn, PlkMnNodeFoundCb nodeFound,
                         PlkMnPdoInCb pdoIn, PlkMnAppReadyCb appReady,
                         void* ctx);

/**
 * 向 DLL 节点表注册一个预先配置的节点（替代自动发现，或追加静态节点）。
 */
int plk_mn_register_node(PlkMn* mn, const PlkDllNodeInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* PLK_MN_H */
