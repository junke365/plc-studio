#ifndef STM32F4XX_H
#define STM32F4XX_H
#include <stdint.h>
#include "core_cm4.h"

#define __I volatile const
#define __O volatile
#define __IO volatile

#define HSE_VALUE           8000000UL
#define HSI_VALUE           16000000UL

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t PLLCFGR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t AHBRSTR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t APB1RSTR;
  volatile uint32_t AHBENR;
  volatile uint32_t APB2ENR;
  volatile uint32_t APB1ENR;
  volatile uint32_t BDCR;
  volatile uint32_t CSR;
  volatile uint32_t AHBLPENR;
  volatile uint32_t APB2LPENR;
  volatile uint32_t APB1LPENR;
  volatile uint32_t DCKCFGR;
} RCC_TypeDef;

typedef struct {
  volatile uint32_t ACR;
  volatile uint32_t KEYR;
  volatile uint32_t OPTKEYR;
  volatile uint32_t SR;
  volatile uint32_t CR;
  volatile uint32_t OPTCR;
} FLASH_TypeDef;

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t CSR;
} PWR_TypeDef;

#define RCC_BASE            0x40023800UL
#define FLASH_R_BASE        0x40023C00UL
#define PWR_BASE            0x40007000UL

#define RCC                 ((RCC_TypeDef*)RCC_BASE)
#define FLASH               ((FLASH_TypeDef*)FLASH_R_BASE)
#define PWR                 ((PWR_TypeDef*)PWR_BASE)

#define RCC_CR_HSION        (1UL << 0)
#define RCC_CR_HSIRDY       (1UL << 1)
#define RCC_CR_HSEON        (1UL << 16)
#define RCC_CR_HSERDY       (1UL << 17)
#define RCC_CR_HSEBYP       (1UL << 18)
#define RCC_CR_CSSON        (1UL << 19)
#define RCC_CR_PLLON        (1UL << 24)
#define RCC_CR_PLLRDY       (1UL << 25)

#define RCC_PLLCFGR_PLLSRC     (1UL << 22)
#define RCC_PLLCFGR_PLLSRC_HSE (1UL << 22)
#define RCC_PLLCFGR_PLLM_Pos   0
#define RCC_PLLCFGR_PLLM_Msk   (0x3FUL << 0)
#define RCC_PLLCFGR_PLLN_Pos   6
#define RCC_PLLCFGR_PLLN_Msk   (0x1FFUL << 6)
#define RCC_PLLCFGR_PLLP_Pos   16
#define RCC_PLLCFGR_PLLP_Msk   (3UL << 16)
#define RCC_PLLCFGR_PLLQ_Pos   24
#define RCC_PLLCFGR_PLLQ_Msk   (0x0FUL << 24)
#define RCC_PLLCFGR_PLLM       RCC_PLLCFGR_PLLM_Msk
#define RCC_PLLCFGR_PLLN       RCC_PLLCFGR_PLLN_Msk
#define RCC_PLLCFGR_PLLP       RCC_PLLCFGR_PLLP_Msk

#define RCC_CFGR_SW_Msk    (3UL << 0)
#define RCC_CFGR_SW_HSI    (0UL << 0)
#define RCC_CFGR_SW_HSE    (1UL << 0)
#define RCC_CFGR_SW_PLL    (2UL << 0)
#define RCC_CFGR_SWS_Msk   (3UL << 2)
#define RCC_CFGR_SWS_HSI   (0UL << 2)
#define RCC_CFGR_SWS_HSE   (1UL << 2)
#define RCC_CFGR_SWS_PLL   (2UL << 2)
#define RCC_CFGR_HPRE_DIV1  (0UL << 4)
#define RCC_CFGR_HPRE_Msk   (0x0FUL << 4)
#define RCC_CFGR_PPRE1_DIV4 (5UL << 10)
#define RCC_CFGR_PPRE2_DIV2 (4UL << 13)
#define RCC_CFGR_HPRE_Pos   4
#define RCC_CFGR_HPRE       0

#define FLASH_ACR_LATENCY_5WS (5UL << 0)
#define FLASH_ACR_PRFTEN      (1UL << 8)
#define FLASH_ACR_ICEN        (1UL << 9)
#define FLASH_ACR_DCEN        (1UL << 10)

#define RCC_APB1ENR_PWREN (1UL << 28)
#define RCC_APB1ENR_TIM6EN (1UL << 4)
#define RCC_APB2ENR_TIM1EN (1UL << 0)

#define PWR_CR_VOS_0 (1UL << 14)
#define PWR_CR_VOS_1 (1UL << 15)

#define __HAL_RCC_TIM6_CLK_ENABLE()   do { RCC->APB1ENR |= RCC_APB1ENR_TIM6EN; } while(0)
#define __HAL_RCC_TIM3_CLK_ENABLE()   do { RCC->APB1ENR |= (1UL<<1); } while(0)
#define __HAL_RCC_GPIOA_CLK_ENABLE()  do { RCC->AHBENR |= (1UL<<0); } while(0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()  do { RCC->AHBENR |= (1UL<<1); } while(0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()  do { RCC->AHBENR |= (1UL<<2); } while(0)
#define __HAL_RCC_USART1_CLK_ENABLE() do { RCC->APB2ENR |= (1UL<<4); } while(0)
#define __HAL_RCC_USART2_CLK_ENABLE() do { RCC->APB1ENR |= (1UL<<17); } while(0)
#define __HAL_RCC_ADC1_CLK_ENABLE()     do { RCC->APB2ENR |= (1UL<<8); } while(0)
#define __HAL_RCC_DAC_CLK_ENABLE()      do { RCC->APB1ENR |= (1UL<<29); } while(0)
#define __HAL_RCC_SPI1_CLK_ENABLE()     do { RCC->APB2ENR |= (1UL<<12); } while(0)
#define __HAL_RCC_GPIOD_CLK_ENABLE()    do { RCC->AHBENR |= (1UL<<3); } while(0)
#define __HAL_RCC_GPIOE_CLK_ENABLE()    do { RCC->AHBENR |= (1UL<<4); } while(0)
#define __HAL_RCC_USART6_CLK_ENABLE()   do { RCC->APB2ENR |= (1UL<<5); } while(0)

#define __HAL_TIM_SET_COMPARE(htim, ch, pulse) ((void)(htim), (void)(ch), (void)(pulse))
#define __HAL_TIM_GET_COUNTER(htim)     0

typedef enum {RESET=0, SET=!RESET} FlagStatus, ITStatus;
typedef enum {DISABLE=0, ENABLE=!DISABLE} FunctionalState;
typedef enum {ERROR=0, SUCCESS=!ERROR} ErrorStatus;

#define HAL_OK       0x00
#define HAL_ERROR    0x01
#define HAL_BUSY     0x02
#define HAL_TIMEOUT  0x03
typedef uint8_t HAL_StatusTypeDef;

#define GPIO_PIN_0  0x01
#define GPIO_PIN_1  0x02
#define GPIO_PIN_2  0x04
#define GPIO_PIN_3  0x08
#define GPIO_PIN_4  0x10
#define GPIO_PIN_5  0x20
#define GPIO_PIN_6  0x40
#define GPIO_PIN_7  0x80
#define GPIO_PIN_8  0x100
#define GPIO_PIN_9  0x200
#define GPIO_PIN_10 0x400
#define GPIO_PIN_11 0x800
#define GPIO_PIN_12 0x1000
#define GPIO_PIN_13 0x2000
#define GPIO_PIN_14 0x4000
#define GPIO_PIN_15 0x8000
#define GPIO_PIN_All 0xFFFF

typedef enum {
  GPIO_MODE_INPUT,
  GPIO_MODE_OUTPUT_PP,
  GPIO_MODE_OUTPUT_OD,
  GPIO_MODE_AF_PP,
  GPIO_MODE_AF_OD,
  GPIO_MODE_ANALOG,
  GPIO_MODE_IT_RISING,
  GPIO_MODE_IT_FALLING,
  GPIO_MODE_IT_RISING_FALLING,
  GPIO_MODE_EVT_RISING,
  GPIO_MODE_EVT_FALLING,
  GPIO_MODE_EVT_RISING_FALLING
} GPIO_Mode;

typedef enum {GPIO_NOPULL=0, GPIO_PULLUP, GPIO_PULLDOWN} GPIO_Pull;

typedef enum {GPIO_SPEED_FREQ_LOW=0, GPIO_SPEED_FREQ_MEDIUM, GPIO_SPEED_FREQ_HIGH, GPIO_SPEED_FREQ_VERY_HIGH} GPIO_Speed;

typedef enum {GPIO_PIN_RESET=0, GPIO_PIN_SET} GPIO_PinState;

typedef struct {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint16_t BSRRL;
  volatile uint16_t BSRRH;
  volatile uint32_t LCKR;
  volatile uint32_t AFR[2];
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef*)0x40020000)
#define GPIOB ((GPIO_TypeDef*)0x40020400)
#define GPIOC ((GPIO_TypeDef*)0x40020800)
#define GPIOD ((GPIO_TypeDef*)0x40020C00)
#define GPIOE ((GPIO_TypeDef*)0x40021000)
#define GPIOF ((GPIO_TypeDef*)0x40021400)
#define GPIOG ((GPIO_TypeDef*)0x40021800)

typedef struct {
  volatile uint16_t SR;
  uint16_t RESERVED0;
  volatile uint16_t DR;
  uint16_t RESERVED1;
  volatile uint16_t DIER;
  uint16_t RESERVED2;
  volatile uint16_t CR1;
  uint16_t RESERVED3;
  volatile uint16_t CR2;
  uint16_t RESERVED4;
  volatile uint32_t CR3;
} TIM_TypeDef;

#define TIM6   ((TIM_TypeDef*)0x40001000)
#define TIM3   ((TIM_TypeDef*)0x40000400)
#define TIM_SR_UIF (1UL << 0)

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t BRR;
  volatile uint32_t GTPR;
  volatile uint32_t RTOR;
  volatile uint32_t RQR;
  volatile uint32_t ISR;
  volatile uint32_t ICR;
  volatile uint32_t RDR;
  volatile uint32_t TDR;
} USART_TypeDef;

#define USART1 ((USART_TypeDef*)0x40011000)
#define USART2 ((USART_TypeDef*)0x40004400)
#define USART3 ((USART_TypeDef*)0x40004800)
#define USART6 ((USART_TypeDef*)0x40011400)

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMPR1;
  volatile uint32_t SMPR2;
  volatile uint32_t JOFR1;
  volatile uint32_t JOFR2;
  volatile uint32_t JOFR3;
  volatile uint32_t JOFR4;
  volatile uint32_t HTR;
  volatile uint32_t LTR;
  volatile uint32_t SQR1;
  volatile uint32_t SQR2;
  volatile uint32_t SQR3;
  volatile uint32_t JSQR;
  volatile uint32_t JDR1;
  volatile uint32_t JDR2;
  volatile uint32_t JDR3;
  volatile uint32_t JDR4;
  volatile uint32_t DR;
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef*)0x40012000)
#define ADC2 ((ADC_TypeDef*)0x40012100)
#define ADC3 ((ADC_TypeDef*)0x40012200)

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t SWTRIGR;
  volatile uint32_t DHR12R1;
  volatile uint32_t DHR12L1;
  volatile uint32_t DHR8R1;
  volatile uint32_t DHR12R2;
  volatile uint32_t DHR12L2;
  volatile uint32_t DHR8R2;
  volatile uint32_t DHR12RD;
  volatile uint32_t DHR12LD;
  volatile uint32_t DHR8RD;
  volatile uint32_t DOR1;
  volatile uint32_t DOR2;
  volatile uint32_t SR;
} DAC_TypeDef;

#define DAC ((DAC_TypeDef*)0x40007400)

typedef struct {
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t CRCPR;
  volatile uint32_t RXCRCR;
  volatile uint32_t TXCRCR;
  volatile uint32_t I2SCFGR;
  volatile uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef*)0x40013000)
#define SPI2 ((SPI_TypeDef*)0x40003800)
#define SPI3 ((SPI_TypeDef*)0x40003C00)

typedef struct {
  volatile uint32_t MACCR;
  volatile uint32_t MACFFR;
  volatile uint32_t MACHTHR;
  volatile uint32_t MACHTLR;
  volatile uint32_t MACMIIAR;
  volatile uint32_t MACMIIDR;
  volatile uint32_t MACFCR;
  volatile uint32_t MACVLANTR;
  uint32_t RESERVED0[2];
  volatile uint32_t MACRWUFFR;
  volatile uint32_t MACPMTCSR;
  uint32_t RESERVED1[2];
  volatile uint32_t MACDBGR;
  volatile uint32_t MACSR;
  volatile uint32_t MACIMR;
  volatile uint32_t MACA0HR;
  volatile uint32_t MACA0LR;
  volatile uint32_t MACA1HR;
  volatile uint32_t MACA1LR;
  volatile uint32_t MACA2HR;
  volatile uint32_t MACA2LR;
  volatile uint32_t MACA3HR;
  volatile uint32_t MACA3LR;
  uint32_t RESERVED2[40];
  volatile uint32_t MMCCR;
  volatile uint32_t MMCRIR;
  volatile uint32_t MMCTIR;
  volatile uint32_t MMCRIMR;
  volatile uint32_t MMCTIMR;
  uint32_t RESERVED3[14];
  volatile uint32_t IEEE_X;
} ETH_TypeDef;

#define ETH ((ETH_TypeDef*)0x40028000)

/* HAL 函数声明移至 stm32f4xx_hal.h */

#endif
