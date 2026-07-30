#ifndef PORTMACRO_H
#define PORTMACRO_H
#include <stdint.h>
#define portCHAR char
#define portFLOAT float
#define portDOUBLE double
#define portLONG long
#define portSHORT short
#define portSTACK_TYPE uint32_t
#define portBASE_TYPE long
typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY (TickType_t)0xFFFFFFFF

#define taskENTER_CRITICAL()   do { __disable_irq(); } while(0)
#define taskEXIT_CRITICAL()    do { __enable_irq(); } while(0)

void* pvPortMalloc(size_t xWantedSize);
void vPortFree(void* pv);
#endif
