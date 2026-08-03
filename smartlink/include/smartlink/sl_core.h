/**
 * sl_core.h - 智能总线 帧构建/解析辅助接口
 *
 * 提供：多播 MAC 查询、帧头填充、各消息类型构建、CRC32 (IEEE 802.3 FCS)。
 */

#ifndef SL_CORE_H
#define SL_CORE_H

#include "smartlink.h"
#include "frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 根据消息类型获取 智能总线 多播 MAC 地址。
 * @param type  SlMsgType (SOC/PRES/SOA/ASND/AMNI)
 * @param mac   输出 6 字节 MAC
 */
int sl_get_mcast_mac(SlMsgType type, uint8_t mac[6]);

/**
 * 填充 智能总线 帧的以太网头（14 字节）与节点地址字段。
 * EtherType 自动写入 0x88AB（网络字节序）。
 */
void sl_frame_fill_header(SlFrame* frame, const uint8_t dstMac[6],
                           const uint8_t srcMac[6], SlMsgType type,
                           uint8_t dstNode, uint8_t srcNode);

/**
 * 构建 SoC 帧。
 * @param srcMac         源 MAC（MN 的 MAC）
 * @param srcNode        MN 节点 ID
 * @param cycleLen       周期长度 [ns]
 * @param relativeTimeUs 相对时间 [us]（0 表示不填充）
 * @return 帧总长度
 */
uint16_t sl_build_soc(SlFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint32_t cycleLen,
                       uint64_t relativeTimeUs);

/**
 * 构建 PReq 帧。
 * @param dstMac   目标 CN 的 MAC
 * @param dstNode  目标 CN 节点 ID
 * @param payload  输出 PDO 数据
 * @param size     输出 PDO 字节数 (<= 256)
 * @param flag1    帧标志（RD/MS/EA）
 * @return 帧总长度
 */
uint16_t sl_build_preq(SlFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, const uint8_t* payload,
                        uint16_t size, uint8_t flag1);

/**
 * 构建 PRes 帧。
 * @param nmtStatus 节点 NMT 状态码（低位字节）
 * @param flag1    帧标志（RD/EN/MS）
 * @param flag2    帧标志（RS/PR）
 * @return 帧总长度
 */
uint16_t sl_build_pres(SlFrame* frame, const uint8_t srcMac[6],
                        uint8_t srcNode, uint8_t nmtStatus,
                        const uint8_t* payload, uint16_t size,
                        uint8_t flag1, uint8_t flag2);

/**
 * 构建 SoA 帧。
 * @param reqServiceId     异步槽服务 ID
 * @param reqServiceTarget 被授权发送的节点 ID
 * @return 帧总长度
 */
uint16_t sl_build_soa(SlFrame* frame, const uint8_t srcMac[6],
                       uint8_t srcNode, uint8_t nmtStatus,
                       uint8_t reqServiceId, uint8_t reqServiceTarget,
                       uint8_t flag1);

/**
 * 构建 ASnd 帧。
 * @param dstMac  目标 MAC（单播/多播/广播）
 * @param dstNode 目标节点 ID
 * @param serviceId ASnd 服务 ID
 * @param payload 服务载荷
 * @param size    载荷字节数
 * @return 帧总长度
 */
uint16_t sl_build_asnd(SlFrame* frame, const uint8_t srcMac[6],
                        const uint8_t dstMac[6], uint8_t srcNode,
                        uint8_t dstNode, uint8_t serviceId,
                        const uint8_t* payload, uint16_t size);

/**
 * 计算 IEEE 802.3 CRC32（以太网 FCS）。
 */
uint32_t sl_crc32(const uint8_t* data, size_t len);

/**
 * 获取某消息类型的最小帧总长度（含以太网头）。
 */
uint16_t sl_frame_min_len(SlMsgType type);

/**
 * 校验接收帧：EtherType 与消息类型。
 * @return 0 有效；负值无效
 */
int sl_frame_validate(const SlFrame* frame, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* SL_CORE_H */
