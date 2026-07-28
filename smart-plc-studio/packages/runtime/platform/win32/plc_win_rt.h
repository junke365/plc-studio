#ifndef PLC_WIN_RT_H
#define PLC_WIN_RT_H

/**
 * plc_win_rt.h - Windows 实时扩展 API
 *
 * 为 Windows 平台提供硬实时能力的近似实现:
 * - 高精度伺服定时器 (1ms 周期, 基于 TimerQueue)
 * - 实时优先级线程管理
 * - 高精度睡眠
 * - CPU 亲和性绑定
 *
 * 注意: Windows 不是硬实时操作系统。
 * 对于真正的硬实时需求，建议使用:
 * - IntervalZero RTX64 (商业)
 * - INtime (商业)
 * - FreeRTOS Windows 模拟 (开源)
 * - Xenomai 4 (Linux 实时)
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 伺服定时器 ==================== */

/**
 * 创建伺服定时器
 * @param periodUs  周期 (微秒, 最小 1000μs=1ms)
 * @param callback  定时回调函数
 * @param context   回调上下文
 * @return 0=成功
 */
int plc_win_rt_createServo(uint32_t periodUs,
                           void (*callback)(void*),
                           void *context);

/**
 * 启动伺服定时器
 */
int plc_win_rt_startServo(void);

/**
 * 停止伺服定时器
 */
int plc_win_rt_stopServo(void);

/**
 * 销毁伺服定时器
 */
void plc_win_rt_destroyServo(void);

/* ==================== 实时线程 ==================== */

/**
 * 创建实时线程 (挂起状态)
 * @param name      线程名
 * @param priority  优先级 (1-6, 6=最高 TIME_CRITICAL)
 * @param cpuMask   CPU 亲和性掩码 (0=不设置)
 * @param outHandle 输出线程句柄
 * @return 线程 ID, -1=失败
 */
int plc_win_rt_createThread(const char *name, int priority,
                            uint32_t cpuMask, void **outHandle);

/**
 * 恢复线程运行
 */
int plc_win_rt_resumeThread(void *hThread);

/**
 * 高精度睡眠 (微秒)
 * 使用 WaitableTimer 实现, 精度 ~100μs
 */
void plc_win_rt_sleepUs(uint32_t us);

/* ==================== FreeRTOS 兼容层 ==================== */

#if defined(PLC_USE_FREERTOS_ON_WIN)

/* FreeRTOS Windows 模拟器集成:
 * 设置 PLC_USE_FREERTOS_ON_WIN 宏后,
 * PLC 任务可通过 FreeRTOS 任务 API 调度。
 *
 * 使用步骤:
 * 1. 下载 FreeRTOS Windows 移植包
 * 2. 设置 FREERTOS_PATH CMake 变量
 * 3. 包含 FreeRTOS.h 和 task.h
 *
 * 示例 CMake:
 *   add_definitions(-DPLC_USE_FREERTOS_ON_WIN)
 *   include_directories(${FREERTOS_PATH}/include)
 *   include_directories(${FREERTOS_PATH}/portable/MSVC-MingW)
 *   add_subdirectory(${FREERTOS_PATH} freertos)
 */

#ifndef INC_FREERTOS_H
#error "PLC_USE_FREERTOS_ON_WIN 需要 FreeRTOS 头文件"
#endif

/* 将 PLC 任务优先级映射到 FreeRTOS 优先级 */
#define PLC_PRIO_TO_FREERTOS(p)  ((p) * 2)
#define FREERTOS_PRIO_TO_PLC(p)  ((p) / 2)

#endif /* PLC_USE_FREERTOS_ON_WIN */

#ifdef __cplusplus
}
#endif

#endif /* PLC_WIN_RT_H */
