/**
 * cnm.h - CN（受控节点）协议栈实例
 *
 * 将 NMT 状态机、SDO 服务器、PDO 交换封装为可接入以太网驱动的完整 CN 栈。
 * 配合 SlEdrv 驱动使用：注册接收回调后，自动处理 SoC / NMT 命令 / SDO 请求 /
 * PReq 轮询，并在收到 PReq 时通过 PRes 回复输入 PDO。
 *
 * 应用层接入点：
 *   - sl_cnm_set_pdo_in   绑定输入 PDO 缓冲（PRes 载荷，CN→MN）
 *   - sl_cnm_set_pdo_out  绑定输出 PDO 缓冲（PReq 载荷，MN→CN），可回调通知
 *   - sl_cnm_app_ready    应用就绪握手（ReadyToOperate 双标志）
 */

#ifndef SL_CNM_H
#define SL_CNM_H

#include "smartlink.h"
#include "frame.h"
#include "nmt.h"
#include "od.h"
#include "edrv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 输出 PDO 更新回调：MN 的 PReq 载荷到达后调用。
 * @param data  输出 PDO 数据（已拷贝到 pollOutData）
 * @param size  实际字节数
 * @param ctx   注册回调时传入的上下文
 */
typedef void (*SlCnmPdoOutCb)(const uint8_t* data, uint16_t size, void* ctx);

/**
 * CN 协议栈上下文。
 */
typedef struct SlCnm
{
  uint8_t  nodeId;               /* 本机节点 ID */
  uint8_t  mac[6];               /* 本机 MAC 地址 */
  SlEdrv* edrv;                 /* 绑定的以太网驱动 */

  SlCnmStateMachine sm;         /* NMT 状态机 */
  SlOd od;                      /* 本地对象字典（SDO 服务器访问目标） */

  uint8_t*  pollInData;          /* 输入 PDO 缓冲（CN→MN，PRes 载荷） */
  uint16_t  pollInSize;
  uint8_t*  pollOutData;         /* 输出 PDO 缓冲（MN→CN，PReq 载荷） */
  uint16_t  pollOutSize;

  SlCnmPdoOutCb pdoOutCb;       /* 输出 PDO 更新回调 */
  void* pdoOutCtx;

  bool started;                  /* 是否已上电启动 */
} SlCnm;

/**
 * 初始化 CN 栈：绑定节点地址、驱动，初始化 NMT 状态机、对象字典与 SDO 服务器。
 * @param nodeId 本机节点 ID
 * @param mac    本机 MAC 地址（6 字节）
 * @param edrv   绑定的以太网驱动（须已注册回调能力）
 */
int sl_cnm_init(SlCnm* cnm, uint8_t nodeId, const uint8_t mac[6], SlEdrv* edrv);

/**
 * 反初始化 CN 栈。
 */
void sl_cnm_exit(SlCnm* cnm);

/**
 * 上电启动：按事件序列将状态机迁移到 NotActive，等待 MN 的 SoC。
 */
int sl_cnm_start(SlCnm* cnm);

/**
 * 绑定输入 PDO 缓冲（PRes 载荷内容，CN→MN）。
 */
int sl_cnm_set_pdo_in(SlCnm* cnm, uint8_t* data, uint16_t size);

/**
 * 绑定输出 PDO 缓冲（PReq 载荷内容，MN→CN）。
 * @param cb  可选：收到新输出 PDO 时回调；可传 NULL
 */
int sl_cnm_set_pdo_out(SlCnm* cnm, uint8_t* data, uint16_t size,
                        SlCnmPdoOutCb cb, void* ctx);

/**
 * 应用就绪握手：应用完成初始化后调用（ReadyToOperate 双标志之一）。
 */
int sl_cnm_app_ready(SlCnm* cnm);

/**
 * 处理接收到的以太网帧（edrv 接收回调入口）。
 * @param raw  完整以太网帧（不含 CRC）
 * @param len  帧长度
 */
int sl_cnm_process_rx(SlCnm* cnm, const uint8_t* raw, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* SL_CNM_H */
