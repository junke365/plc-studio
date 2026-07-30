/**
 * start_stm32f407xx.s — STM32F407VG 启动文件
 *
 * 功能:
 *   1. 定义中断向量表（放在 .isr_vector 段）
 *   2. 设置栈顶指针 __initial_sp
 *   3. 调用 SystemInit() → 跳转到 main()
 *
 * MCU: STM32F407VG (Cortex-M4F, 1MB Flash, 192KB RAM)
 */

.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.global g_pfnVectors
.global Default_Handler

/* 栈顶地址（链接脚本提供） */
.word _estack

/* ========== 中断向量表 ========== */
.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0                         /* Reserved */
  .word 0                         /* Reserved */
  .word 0                         /* Reserved */
  .word 0                         /* Reserved */
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0                         /* Reserved */
  .word PendSV_Handler
  .word SysTick_Handler

  /* 外部中断 (STM32F407VG) */
  .word WWDG_IRQHandler           /* 0: 窗口看门狗 */
  .word PVD_IRQHandler            /* 1: PVD */
  .word TAMP_STAMP_IRQHandler     /* 2: RTC 侵入/时间戳 */
  .word RTC_WKUP_IRQHandler       /* 3: RTC 唤醒 */
  .word FLASH_IRQHandler          /* 4: Flash */
  .word RCC_IRQHandler            /* 5: RCC */
  .word EXTI0_IRQHandler          /* 6: EXTI0 */
  .word EXTI1_IRQHandler          /* 7: EXTI1 */
  .word EXTI2_IRQHandler          /* 8: EXTI2 */
  .word EXTI3_IRQHandler          /* 9: EXTI3 */
  .word EXTI4_IRQHandler          /* 10: EXTI4 */
  .word DMA1_Stream0_IRQHandler   /* 11: DMA1 Stream0 */
  .word DMA1_Stream1_IRQHandler   /* 12: DMA1 Stream1 */
  .word DMA1_Stream2_IRQHandler   /* 13: DMA1 Stream2 */
  .word DMA1_Stream3_IRQHandler   /* 14: DMA1 Stream3 */
  .word DMA1_Stream4_IRQHandler   /* 15: DMA1 Stream4 */
  .word DMA1_Stream5_IRQHandler   /* 16: DMA1 Stream5 */
  .word DMA1_Stream6_IRQHandler   /* 17: DMA1 Stream6 */
  .word ADC_IRQHandler            /* 18: ADC1/2/3 */
  .word CAN1_TX_IRQHandler        /* 19: CAN1 TX */
  .word CAN1_RX0_IRQHandler       /* 20: CAN1 RX0 */
  .word CAN1_RX1_IRQHandler       /* 21: CAN1 RX1 */
  .word CAN1_SCE_IRQHandler       /* 22: CAN1 SCE */
  .word EXTI9_5_IRQHandler        /* 23: EXTI9-5 */
  .word TIM1_BRK_TIM9_IRQHandler  /* 24: TIM1 BRK / TIM9 */
  .word TIM1_UP_TIM10_IRQHandler  /* 25: TIM1 UP / TIM10 */
  .word TIM1_TRG_COM_TIM11_IRQHandler /* 26: TIM1 TRG/COM / TIM11 */
  .word TIM1_CC_IRQHandler        /* 27: TIM1 CC */
  .word TIM2_IRQHandler           /* 28: TIM2 */
  .word TIM3_IRQHandler           /* 29: TIM3 */
  .word TIM4_IRQHandler           /* 30: TIM4 */
  .word I2C1_EV_IRQHandler        /* 31: I2C1 EV */
  .word I2C1_ER_IRQHandler        /* 32: I2C1 ER */
  .word I2C2_EV_IRQHandler        /* 33: I2C2 EV */
  .word I2C2_ER_IRQHandler        /* 34: I2C2 ER */
  .word SPI1_IRQHandler           /* 35: SPI1 */
  .word SPI2_IRQHandler           /* 36: SPI2 */
  .word USART1_IRQHandler         /* 37: USART1 */
  .word USART2_IRQHandler         /* 38: USART2 */
  .word USART3_IRQHandler         /* 39: USART3 */
  .word EXTI15_10_IRQHandler      /* 40: EXTI15-10 */
  .word RTC_Alarm_IRQHandler      /* 41: RTC Alarm */
  .word OTG_FS_WKUP_IRQHandler    /* 42: OTG FS WKUP */
  .word TIM8_BRK_TIM12_IRQHandler /* 43: TIM8 BRK / TIM12 */
  .word TIM8_UP_TIM13_IRQHandler  /* 44: TIM8 UP / TIM13 */
  .word TIM8_TRG_COM_TIM14_IRQHandler /* 45: TIM8 TRG/COM / TIM14 */
  .word TIM8_CC_IRQHandler        /* 46: TIM8 CC */
  .word DMA1_Stream7_IRQHandler   /* 47: DMA1 Stream7 */
  .word FSMC_IRQHandler           /* 48: FSMC */
  .word SDIO_IRQHandler           /* 49: SDIO */
  .word TIM5_IRQHandler           /* 50: TIM5 */
  .word SPI3_IRQHandler           /* 51: SPI3 */
  .word UART4_IRQHandler          /* 52: UART4 */
  .word UART5_IRQHandler          /* 53: UART5 */
  .word TIM6_DAC_IRQHandler       /* 54: TIM6 / DAC */
  .word TIM7_IRQHandler           /* 55: TIM7 */
  .word DMA2_Stream0_IRQHandler   /* 56: DMA2 Stream0 */
  .word DMA2_Stream1_IRQHandler   /* 57: DMA2 Stream1 */
  .word DMA2_Stream2_IRQHandler   /* 58: DMA2 Stream2 */
  .word DMA2_Stream3_IRQHandler   /* 59: DMA2 Stream3 */
  .word DMA2_Stream4_IRQHandler   /* 60: DMA2 Stream4 */
  .word ETH_IRQHandler            /* 61: Ethernet */
  .word ETH_WKUP_IRQHandler       /* 62: Ethernet WKUP */
  .word CAN2_TX_IRQHandler        /* 63: CAN2 TX */
  .word CAN2_RX0_IRQHandler       /* 64: CAN2 RX0 */
  .word CAN2_RX1_IRQHandler       /* 65: CAN2 RX1 */
  .word CAN2_SCE_IRQHandler       /* 66: CAN2 SCE */
  .word OTG_FS_IRQHandler         /* 67: OTG FS */
  .word DMA2_Stream5_IRQHandler   /* 68: DMA2 Stream5 */
  .word DMA2_Stream6_IRQHandler   /* 69: DMA2 Stream6 */
  .word DMA2_Stream7_IRQHandler   /* 70: DMA2 Stream7 */
  .word USART6_IRQHandler         /* 71: USART6 */
  .word I2C3_EV_IRQHandler        /* 72: I2C3 EV */
  .word I2C3_ER_IRQHandler        /* 73: I2C3 ER */
  .word OTG_HS_EP1_OUT_IRQHandler /* 74: OTG HS EP1 OUT */
  .word OTG_HS_EP1_IN_IRQHandler  /* 75: OTG HS EP1 IN */
  .word OTG_HS_WKUP_IRQHandler    /* 76: OTG HS WKUP */
  .word OTG_HS_IRQHandler         /* 77: OTG HS */
  .word DCMI_IRQHandler           /* 78: DCMI */
  .word CRYP_IRQHandler           /* 79: CRYP */
  .word HASH_RNG_IRQHandler       /* 80: Hash / RNG */
  .word FPU_IRQHandler            /* 81: FPU */

/* ========== 复位处理 ========== */
.section .text.Reset_Handler, "ax", %progbits
.type Reset_Handler, %function
Reset_Handler:
  /* 设置栈指针 */
  ldr r0, =_estack
  msr msp, r0

  /* 清除 BSS 段 */
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
  b LoopFillBss

FillBss:
  str r2, [r0]
  adds r0, r0, #4

LoopFillBss:
  cmp r0, r1
  bcc FillBss

  /* 复制 .data 段从 Flash 到 SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  b LoopCopyData

CopyData:
  ldr r3, [r2]
  str r3, [r0]
  adds r0, r0, #4
  adds r2, r2, #4

LoopCopyData:
  cmp r0, r1
  bcc CopyData

  /* 调用 SystemInit（系统时钟配置） */
  bl SystemInit

  /* 调用 C++ 静态初始化（如果使用） */
  bl __libc_init_array

  /* 跳转到 main */
  bl main

  /* 永不返回 */
  bl exit

/* ========== 默认中断处理（弱定义） ========== */
.macro weak_handler name
  .thumb_func
  .weak \name
  .type \name, %function
\name:
  b .
.endm

weak_handler Default_Handler
weak_handler NMI_Handler
weak_handler HardFault_Handler
weak_handler MemManage_Handler
weak_handler BusFault_Handler
weak_handler UsageFault_Handler
weak_handler SVC_Handler
weak_handler DebugMon_Handler
weak_handler PendSV_Handler
weak_handler SysTick_Handler

/* 外部中断默认处理 */
weak_handler WWDG_IRQHandler
weak_handler PVD_IRQHandler
weak_handler TAMP_STAMP_IRQHandler
weak_handler RTC_WKUP_IRQHandler
weak_handler FLASH_IRQHandler
weak_handler RCC_IRQHandler
weak_handler EXTI0_IRQHandler
weak_handler EXTI1_IRQHandler
weak_handler EXTI2_IRQHandler
weak_handler EXTI3_IRQHandler
weak_handler EXTI4_IRQHandler
weak_handler DMA1_Stream0_IRQHandler
weak_handler DMA1_Stream1_IRQHandler
weak_handler DMA1_Stream2_IRQHandler
weak_handler DMA1_Stream3_IRQHandler
weak_handler DMA1_Stream4_IRQHandler
weak_handler DMA1_Stream5_IRQHandler
weak_handler DMA1_Stream6_IRQHandler
weak_handler ADC_IRQHandler
weak_handler CAN1_TX_IRQHandler
weak_handler CAN1_RX0_IRQHandler
weak_handler CAN1_RX1_IRQHandler
weak_handler CAN1_SCE_IRQHandler
weak_handler EXTI9_5_IRQHandler
weak_handler TIM1_BRK_TIM9_IRQHandler
weak_handler TIM1_UP_TIM10_IRQHandler
weak_handler TIM1_TRG_COM_TIM11_IRQHandler
weak_handler TIM1_CC_IRQHandler
weak_handler TIM2_IRQHandler
weak_handler TIM3_IRQHandler
weak_handler TIM4_IRQHandler
weak_handler I2C1_EV_IRQHandler
weak_handler I2C1_ER_IRQHandler
weak_handler I2C2_EV_IRQHandler
weak_handler I2C2_ER_IRQHandler
weak_handler SPI1_IRQHandler
weak_handler SPI2_IRQHandler
weak_handler USART1_IRQHandler
weak_handler USART2_IRQHandler
weak_handler USART3_IRQHandler
weak_handler EXTI15_10_IRQHandler
weak_handler RTC_Alarm_IRQHandler
weak_handler OTG_FS_WKUP_IRQHandler
weak_handler TIM8_BRK_TIM12_IRQHandler
weak_handler TIM8_UP_TIM13_IRQHandler
weak_handler TIM8_TRG_COM_TIM14_IRQHandler
weak_handler TIM8_CC_IRQHandler
weak_handler DMA1_Stream7_IRQHandler
weak_handler FSMC_IRQHandler
weak_handler SDIO_IRQHandler
weak_handler TIM5_IRQHandler
weak_handler SPI3_IRQHandler
weak_handler UART4_IRQHandler
weak_handler UART5_IRQHandler
weak_handler TIM6_DAC_IRQHandler
weak_handler TIM7_IRQHandler
weak_handler DMA2_Stream0_IRQHandler
weak_handler DMA2_Stream1_IRQHandler
weak_handler DMA2_Stream2_IRQHandler
weak_handler DMA2_Stream3_IRQHandler
weak_handler DMA2_Stream4_IRQHandler
weak_handler ETH_IRQHandler
weak_handler ETH_WKUP_IRQHandler
weak_handler CAN2_TX_IRQHandler
weak_handler CAN2_RX0_IRQHandler
weak_handler CAN2_RX1_IRQHandler
weak_handler CAN2_SCE_IRQHandler
weak_handler OTG_FS_IRQHandler
weak_handler DMA2_Stream5_IRQHandler
weak_handler DMA2_Stream6_IRQHandler
weak_handler DMA2_Stream7_IRQHandler
weak_handler USART6_IRQHandler
weak_handler I2C3_EV_IRQHandler
weak_handler I2C3_ER_IRQHandler
weak_handler OTG_HS_EP1_OUT_IRQHandler
weak_handler OTG_HS_EP1_IN_IRQHandler
weak_handler OTG_HS_WKUP_IRQHandler
weak_handler OTG_HS_IRQHandler
weak_handler DCMI_IRQHandler
weak_handler CRYP_IRQHandler
weak_handler HASH_RNG_IRQHandler
weak_handler FPU_IRQHandler

.end
