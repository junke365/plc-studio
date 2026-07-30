/**
 * FreeRTOSConfig.h — FreeRTOS 配置 (STM32F407, Cortex-M4F)
 *
 * 配置要点:
 *   - 时钟: SysTick 1ms (168MHz / 8 = 21MHz 计数时钟)
 *   - 堆大小: 32KB (SRAM1 堆区)
 *   - 最大优先级: 5 (足够 PLC 任务 + 通信任务 + LED)
 *   - 支持: 互斥量、递归互斥量、时间片轮转
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f4xx_hal.h"

/* ========== 基础配置 ========== */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SYNC      1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ((uint32_t)168000000)
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    (5)
#define configMINIMAL_STACK_SIZE                ((unsigned short)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)(32 * 1024))
#define configMAX_TASK_NAME_LEN                 (16)
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1

/* ========== 内存管理 ========== */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ========== 协程（不用） ========== */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         (2)

/* ========== 软件定时器 ========== */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (2)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE)

/* ========== 断言 ========== */
#define configASSERT(x)                         if ((x) == 0) { taskDISABLE_INTERRUPTS(); for (;;); }

/* ========== 中断嵌套 ========== */
#define configPRIO_BITS                         4
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5
#define configKERNEL_INTERRUPT_PRIORITY         (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5 << (8 - configPRIO_BITS))
#define configMAX_API_CALL_INTERRUPT_PRIORITY   (5 << (8 - configPRIO_BITS))

/* ========== 可选功能 ========== */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xSemaphoreGetMutexHolder        1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1

/* ========== 跟踪/调试 ========== */
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#define configCHECK_FOR_STACK_OVERFLOW          2

/* ========== 钩子函数 ========== */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ========== SysTick 中断优先级 ========== */
#define configTICK_PRIORITY                     (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)

#endif /* FREERTOS_CONFIG_H */
