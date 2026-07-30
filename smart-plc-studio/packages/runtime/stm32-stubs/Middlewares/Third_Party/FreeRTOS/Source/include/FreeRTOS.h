#ifndef FREERTOS_H
#define FREERTOS_H
#include <stdint.h>
#include <stddef.h>
#include "portmacro.h"

typedef unsigned long TickType_t;
typedef unsigned long UBaseType_t;
typedef long BaseType_t;

#define pdMS_TO_TICKS(x) ((TickType_t)(x))
/* portMAX_DELAY defined in portmacro.h */
#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1
#define pdFAIL 0

#define configMAX_PRIORITIES 5
#define taskDISABLE_INTERRUPTS() __disable_irq()
#define taskENABLE_INTERRUPTS() __enable_irq()

#define portTICK_PERIOD_MS 1

void vTaskStartScheduler(void);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement);
TickType_t xTaskGetTickCount(void);

typedef void* TaskHandle_t;
BaseType_t xTaskCreate(void(*)(void*), const char*, unsigned, void*, unsigned, TaskHandle_t*);

/* 任务状态类型 */
typedef enum {
  eRunning = 0, eReady, eBlocked, eSuspended, eDeleted, eInvalid
} eTaskState;

/* 任务状态信息结构 */
typedef struct {
  TaskHandle_t xHandle;
  const char* pcTaskName;
  UBaseType_t xTaskNumber;
  eTaskState eCurrentState;
  UBaseType_t uxCurrentPriority;
  UBaseType_t uxBasePriority;
  uint32_t ulRunTimeCounter;
  StackType_t* pxStackBase;
  uint32_t usStackHighWaterMark;
} TaskStatus_t;

UBaseType_t uxTaskGetSystemState(TaskStatus_t* pxTaskStatusArray,
                                  UBaseType_t uxArraySize,
                                  uint32_t* pulTotalRunTime);
#endif
