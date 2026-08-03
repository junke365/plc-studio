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

/* 故障诊断: 现场保存到固定地址 0x2001C000 (SRAM2, 当前未使用) */
#define FAULT_DUMP_BASE 0x2001C000u

void HardFault_Dump(void);

__attribute__((naked)) void HardFault_Handler(void)
{
  /* naked: 在函数入口处立即保存现场，避免编译器 push 干扰帧定位
   * 布局: [0]=MSP, [1]=PSP, [2]=CFSR, [3]=BFAR, [4..11]=异常帧
   * (R0,R1,R2,R3,R12,LR,PC,xPSR), [7] 覆写为魔法数 */
  __asm volatile(
    "ldr r0, =0x2001C000\n"
    "mrs r1, MSP\n"
    "str r1, [r0, #0]\n"
    "mrs r1, PSP\n"
    "str r1, [r0, #4]\n"
    "ldr r1, =0xE000ED28\n" /* SCB->CFSR */
    "ldr r2, [r1]\n"
    "str r2, [r0, #8]\n"
    "ldr r1, =0xE000ED38\n" /* SCB->BFAR */
    "ldr r2, [r1]\n"
    "str r2, [r0, #12]\n"
    "b HardFault_Dump\n"
  );
}

void HardFault_Dump(void)
{
  uint32_t* dump = (uint32_t*)FAULT_DUMP_BASE;
  uint32_t msp = dump[0];
  uint32_t* frame = (uint32_t*)msp;
  /* 复制异常压栈帧 (R0-R3, R12, LR, PC, xPSR) 到 dump[8..15] */
  for (int i = 0; i < 8; i++)
    dump[8 + i] = frame[i];
  /* 标记现场有效 */
  dump[7] = 0xFA57A71Fu;
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
