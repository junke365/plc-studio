/**
 * stm32f4xx_it.c — STM32F407 中断服务例程
 *
 * 注意:
 *   SysTick_Handler 由 FreeRTOS 提供 (port.c)
 *   PendSV_Handler  由 FreeRTOS 提供
 *   SVC_Handler      由 FreeRTOS 提供
 *
 * 非 FreeRTOS 中断优先级必须高于 configMAX_SYSCALL_INTERRUPT_PRIORITY
 */

#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* 外部 HAL 句柄（在 main.c 或 CubeMX 生成代码中定义） */
extern ADC_HandleTypeDef hadc1;

/* ========== 系统异常 ========== */

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void MemManage_Handler(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void BusFault_Handler(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void UsageFault_Handler(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void DebugMon_Handler(void)
{
}

/* ========== 外设中断 ========== */

void TIM6_DAC_IRQHandler(void)
{
  if (TIM6->SR & TIM_SR_UIF) {
    TIM6->SR = (uint16_t)~TIM_SR_UIF;
  }
}

void USART1_IRQHandler(void)
{
}

void USART6_IRQHandler(void)
{
}

void ADC_IRQHandler(void)
{
  HAL_ADC_IRQHandler(&hadc1);
}

void DMA2_Stream0_IRQHandler(void)
{
}

void ETH_IRQHandler(void)
{
}
