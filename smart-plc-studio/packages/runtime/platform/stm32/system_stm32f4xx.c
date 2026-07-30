/**
 * system_stm32f4xx.c — STM32F407 系统时钟初始化
 *
 * 时钟树:
 *   HSE (8MHz) → PLL (x42) → SYSCLK = 168MHz
 *   AHB  = 168MHz
 *   APB1 = 42MHz  (prescaler 4)
 *   APB2 = 84MHz  (prescaler 2)
 *
 * 适用于: STM32F407VG (HSE = 8MHz 晶振)
 * 若 HSE 频率不同，修改 PLL_M 和 PLL_N 参数
 */

#include "stm32f4xx.h"

/* ========== 系统时钟频率变量 ========== */

uint32_t SystemCoreClock = 168000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

/**
 * SystemInit — 配置系统时钟
 *
 * 时钟配置:
 *   1. HSE 8MHz 外部晶振
 *   2. PLL: 8MHz / 8 (M) × 336 (N) / 2 (P) = 168MHz
 *   3. USB/SDIO/RNG: PLLQ = 336 / 7 = 48MHz
 */
void SystemInit(void)
{
  /* 使能 FPU（Cortex-M4F 必需） */
  SCB->CPACR |= (3UL << 20) | (3UL << 22);

  /* 复位 RCC 配置 */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));

  RCC->CFGR = 0x00000000;
  RCC->CR   &= ~(RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEBYP);

  /* 配置电源 */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR |= PWR_CR_VOS_1;

  /* 使能 HSE */
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY));

  /* 配置 Flash 预取/等待周期 (5 WS for 168MHz) */
  FLASH->ACR = FLASH_ACR_LATENCY_5WS |
               FLASH_ACR_PRFTEN |
               FLASH_ACR_ICEN |
               FLASH_ACR_DCEN;

  /* 配置 AHB/APB 分频 */
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;    /* AHB = SYSCLK / 1 = 168MHz */
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;   /* APB1 = AHB / 4 = 42MHz */
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;   /* APB2 = AHB / 2 = 84MHz */

  /* 配置 PLL: HSE(8MHz) / 8 × 336 / 2 = 168MHz */
  RCC->PLLCFGR = (8UL  << RCC_PLLCFGR_PLLM_Pos)  |  /* M = 8  */
                 (336UL << RCC_PLLCFGR_PLLN_Pos)  |  /* N = 336 */
                 (0UL  << RCC_PLLCFGR_PLLP_Pos)  |  /* P = 2  */
                 (7UL  << RCC_PLLCFGR_PLLQ_Pos)  |  /* Q = 7 → 48MHz */
                 RCC_PLLCFGR_PLLSRC_HSE;

  /* 使能 PLL 并等待锁定 */
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY));

  /* 切换到 PLL 作为系统时钟 */
  RCC->CFGR &= ~RCC_CFGR_SW_Msk;
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);

  /* 更新 SystemCoreClock */
  SystemCoreClock = 168000000;

  /* 使能 TIM 的 APB 时钟 */
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
}

/**
 * SystemCoreClockUpdate — 更新 SystemCoreClock 变量
 * 在时钟树发生变化后调用
 */
void SystemCoreClockUpdate(void)
{
  uint32_t pllm, plln, pllp, pllsrc;
  uint32_t hclk;

  switch (RCC->CFGR & RCC_CFGR_SWS_Msk) {
    case RCC_CFGR_SWS_HSI:
      hclk = 16000000;
      break;

    case RCC_CFGR_SWS_HSE:
      hclk = HSE_VALUE;
      break;

    case RCC_CFGR_SWS_PLL:
      pllsrc = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) ? HSE_VALUE : 16000000;
      pllm   = (RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos;
      plln   = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
      pllp   = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos) * 2 + 2;
      hclk   = (pllsrc / pllm) * plln / pllp;
      break;

    default:
      hclk = 16000000;
  }

  /* 应用 AHB 分频 */
  hclk >>= AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos];
  SystemCoreClock = hclk;
}
