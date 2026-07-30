#ifndef SEMPHR_H
#define SEMPHR_H
#include "FreeRTOS.h"
typedef void* SemaphoreHandle_t;
#define xSemaphoreCreateMutex() ((SemaphoreHandle_t)1)
#define xSemaphoreTake(s, t) pdPASS
#define xSemaphoreGive(s) pdPASS
#endif
