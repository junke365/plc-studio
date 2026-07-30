#ifndef TASK_H
#define TASK_H
#include "FreeRTOS.h"

void vTaskStartScheduler(void);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement);
TickType_t xTaskGetTickCount(void);
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
eTaskState eTaskGetState(TaskHandle_t xTask);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
#endif
