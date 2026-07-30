/**
 * stm32f4xx_it.h — STM32F407 中断服务声明
 */

#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#include "stm32f4xx.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* 应用中断 */
void TIM6_DAC_IRQHandler(void);
void USART1_IRQHandler(void);
void USART6_IRQHandler(void);
void ADC_IRQHandler(void);
void DMA2_Stream0_IRQHandler(void);
void ETH_IRQHandler(void);

#endif /* __STM32F4xx_IT_H */
