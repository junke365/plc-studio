/**
 * main.c — STM32F407 PLC 运行时主程序
 *
 * 硬件配置:
 *   MCU:     STM32F407VG (Cortex-M4F @ 168MHz)
 *   HSE:     8MHz 晶振
 *   LED:     PA5 (心跳指示)
 *   UART1:   PA9(TX)/PA10(RX) — Modbus RTU
 *   调试串口 (USART6): PC6(TX)/PD15(RX) — 调试输出 (115200)
 *   ADC1:    PA0(AI0), PA1(AI1)
 *   TIM3:    PA6(PWM0) — PWM 输出
 *   TIM6:    微秒延时基准
 *   SPI1:    PA5(SCK)/PA6(MISO)/PA7(MOSI)/PA4(CS) — I/O 扩展
 *   5轴 CNC 步进电机:
 *     X: STEP=PB4, DIR=PB5, EN=PB6
 *     Y: STEP=PB7, DIR=PB8, EN=PB9
 *     Z: STEP=PB10, DIR=PB11, EN=PB12
 *     A: STEP=PE0, DIR=PE1, EN=PE2
 *     B: STEP=PE3, DIR=PE4, EN=PE5
 *
 * 构建:
 *   cmake -B build-stm32 -DPLATFORM=stm32 ^
 *     -DSTM32_HAL_DIR=path/to/STM32Cube_FW_F4 ^
 *     -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_gcc.cmake
 *   cmake --build build-stm32
 */

#include <stdio.h>
#include <string.h>
#include "plc_runtime.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ========== STM32 HAL ========== */

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_dac.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_spi.h"
#include "stm32f4xx_hal_rcc.h"

/* ========== HAL 句柄 ========== */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef  hadc1;
DAC_HandleTypeDef  hdac;
TIM_HandleTypeDef  htim3;
TIM_HandleTypeDef  htim6;
SPI_HandleTypeDef  hspi1;

/* ========== 5轴 CNC 步进电机引脚定义 ========== */
/*
 * 每个轴 3 个控制信号: STEP(步进脉冲), DIR(方向), ENABLE(使能)
 * 引脚编码格式: (GPIO端口索引 << 4) | 引脚号
 * 端口索引: GPIOA=0, GPIOB=1, GPIOC=2, GPIOD=3, GPIOE=4
 */

/* X 轴 */
#define AXIS_X_STEP_GPIO  GPIOB
#define AXIS_X_STEP_PIN   GPIO_PIN_4
#define AXIS_X_DIR_GPIO   GPIOB
#define AXIS_X_DIR_PIN    GPIO_PIN_5
#define AXIS_X_EN_GPIO    GPIOB
#define AXIS_X_EN_PIN     GPIO_PIN_6
#define AXIS_X_STEP_ADDR  20   /* (1<<4)|4 */
#define AXIS_X_DIR_ADDR   21   /* (1<<4)|5 */
#define AXIS_X_EN_ADDR    22   /* (1<<4)|6 */

/* Y 轴 */
#define AXIS_Y_STEP_GPIO  GPIOB
#define AXIS_Y_STEP_PIN   GPIO_PIN_7
#define AXIS_Y_DIR_GPIO   GPIOB
#define AXIS_Y_DIR_PIN    GPIO_PIN_8
#define AXIS_Y_EN_GPIO    GPIOB
#define AXIS_Y_EN_PIN     GPIO_PIN_9
#define AXIS_Y_STEP_ADDR  23   /* (1<<4)|7 */
#define AXIS_Y_DIR_ADDR   24   /* (1<<4)|8 */
#define AXIS_Y_EN_ADDR    25   /* (1<<4)|9 */

/* Z 轴 */
#define AXIS_Z_STEP_GPIO  GPIOB
#define AXIS_Z_STEP_PIN   GPIO_PIN_10
#define AXIS_Z_DIR_GPIO   GPIOB
#define AXIS_Z_DIR_PIN    GPIO_PIN_11
#define AXIS_Z_EN_GPIO    GPIOB
#define AXIS_Z_EN_PIN     GPIO_PIN_12
#define AXIS_Z_STEP_ADDR  26   /* (1<<4)|10 */
#define AXIS_Z_DIR_ADDR   27   /* (1<<4)|11 */
#define AXIS_Z_EN_ADDR    28   /* (1<<4)|12 */

/* A 轴 (旋转轴) */
#define AXIS_A_STEP_GPIO  GPIOE
#define AXIS_A_STEP_PIN   GPIO_PIN_0
#define AXIS_A_DIR_GPIO   GPIOE
#define AXIS_A_DIR_PIN    GPIO_PIN_1
#define AXIS_A_EN_GPIO    GPIOE
#define AXIS_A_EN_PIN     GPIO_PIN_2
#define AXIS_A_STEP_ADDR  64   /* (4<<4)|0 */
#define AXIS_A_DIR_ADDR   65   /* (4<<4)|1 */
#define AXIS_A_EN_ADDR    66   /* (4<<4)|2 */

/* B 轴 (旋转轴) */
#define AXIS_B_STEP_GPIO  GPIOE
#define AXIS_B_STEP_PIN   GPIO_PIN_3
#define AXIS_B_DIR_GPIO   GPIOE
#define AXIS_B_DIR_PIN    GPIO_PIN_4
#define AXIS_B_EN_GPIO    GPIOE
#define AXIS_B_EN_PIN     GPIO_PIN_5
#define AXIS_B_STEP_ADDR  67   /* (4<<4)|3 */
#define AXIS_B_DIR_ADDR   68   /* (4<<4)|4 */
#define AXIS_B_EN_ADDR    69   /* (4<<4)|5 */

/* ========== 外设初始化函数声明 ========== */

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_DBG_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM3_PWM_Init(void);
static void MX_TIM6_Init(void);
static void MX_SPI1_Init(void);

/* ========== 系统时钟配置 (168MHz) ========== */

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* HSE 8MHz → PLL → 168MHz */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* AHB=168MHz, APB1=42MHz, APB2=84MHz */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  /* 使能 TIM 时钟 */
  __HAL_RCC_TIM6_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
}

/* ========== GPIO 初始化 ========== */

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* PA5: LED 心跳输出 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* PB0-PB3: 数字量输入（外部传感器） */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PC0-PC1: 数字量输出（继电器） */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* PA4: SPI1 CS 输出 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /* ===== 5轴 CNC 步进电机控制引脚 ===== */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /* X 轴: PB4(STEP), PB5(DIR), PB6(EN) */
  GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);

  /* Y 轴: PB7(STEP), PB8(DIR), PB9(EN) */
  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);

  /* Z 轴: PB10(STEP), PB11(DIR), PB12(EN) */
  GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);

  /* A 轴: PE0(STEP), PE1(DIR), PE2(EN) */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

  /* B 轴: PE3(STEP), PE4(DIR), PE5(EN) */
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);
}

/* ========== UART 初始化 ========== */

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
}

static void MX_USART6_DBG_UART_Init(void)
{
  huart2.Instance = USART6;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
}

/* ========== ADC 初始化 ========== */

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);

  /* 通道0: PA0 */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* ========== DAC 初始化 ========== */

static void MX_DAC_Init(void)
{
  hdac.Instance = DAC;
  HAL_DAC_Init(&hdac);

  DAC_ChannelConfTypeDef sConfig = {0};
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
}

/* ========== TIM3 PWM 初始化 ========== */

static void MX_TIM3_PWM_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 168 - 1;          /* 168MHz / 168 = 1MHz */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000 - 1;            /* 1MHz / 1000 = 1kHz PWM */
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim3);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;                   /* 50% duty */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

/* ========== TIM6 微秒定时器 ========== */

static void MX_TIM6_Init(void)
{
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 168 - 1;          /* 1MHz 计数 */
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 65535;               /* 最大周期 */
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim6);
}

/* ========== SPI1 初始化（I/O 扩展器） ========== */

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  HAL_SPI_Init(&hspi1);
}

/* ========== HAL MSP 初始化回调 ========== */

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef gpio = {0};

  if (huart->Instance == USART1) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF7_USART1;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

  } else if (huart->Instance == USART6) {
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PC6: USART6 TX */
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF8_USART6;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* PD15: USART6 RX */
    gpio.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio);
  }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
  }
}

void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{
  if (hdac->Instance == DAC) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
  }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  if (hspi->Instance == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
  if (htim->Instance == TIM6) {
    __HAL_RCC_TIM6_CLK_ENABLE();
  }
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim)
{
  if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF2_TIM3;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
  }
}

/* ========== printf 重定向到 UART2 ========== */

int _write(int fd, char* ptr, int len)
{
  (void)fd;
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, 100);
  return len;
}

/* ========== PLC 运行时 ========== */

static PlcRuntime g_runtime;

/* ========== FreeRTOS 任务 ========== */

static void plc_scan_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;
  TickType_t lastWake = xTaskGetTickCount();

  printf("[PLC] 扫描任务已启动, 周期=1ms\n");

  for (;;) {
    plc_runtime_scan(rt);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1));
  }
}

static void comm_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t printCount = 0;

  printf("[PLC] 通信任务已启动, 周期=10ms\n");

  for (;;) {
    printCount++;

    /* 每 100 个周期（1秒）输出一次 PLC 状态 */
    if (printCount % 100 == 0) {
      PlcStats stats;
      plc_runtime_get_stats(rt, &stats);

      /* 读取关键变量 */
      plc_bool motor, alarm;
      plc_int runCount, tempValue;
      plc_var_read(&rt->var_table, "Main.Motor", &motor, sizeof(motor));
      plc_var_read(&rt->var_table, "Main.Alarm", &alarm, sizeof(alarm));
      plc_var_read(&rt->var_table, "Main.RunCount", &runCount, sizeof(runCount));
      plc_var_read(&rt->var_table, "Main.TempValue", &tempValue, sizeof(tempValue));

      /* 读取物理输入 */
      plc_bool di0, di1;
      plc_var_read(&rt->var_table, "sensor_di_0", &di0, sizeof(di0));
      plc_var_read(&rt->var_table, "sensor_di_1", &di1, sizeof(di1));

      printf("\n[STAT] up=%lus cyc=%lu cycT=%luus maxT=%luus err=%lu\n",
        (unsigned long)(stats.uptime_ms / 1000),
        (unsigned long)stats.cycle_count,
        (unsigned long)stats.cycle_time_us,
        (unsigned long)stats.max_cycle_time_us,
        (unsigned long)stats.error_count);
      printf("[VAR]  Motor=%d Alarm=%d RunCnt=%d TmpVal=%d DI0=%d DI1=%d\n",
        motor, alarm, runCount, tempValue, di0, di1);
    }

    /* Modbus RTU 轮询处理 */
    plc_comm_modbus_poll(0, &rt->var_table);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
  }
}

static void led_task(void* pvParameters)
{
  (void)pvParameters;
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t toggleCount = 0;

  for (;;) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    toggleCount++;

    /* 每 20 次翻转（10 秒）输出一次任务健康状态 */
    if (toggleCount % 20 == 0) {
      TaskStatus_t taskStatus[8];
      uint32_t totalRunTime;
      uint32_t taskCount = uxTaskGetSystemState(taskStatus, 8, &totalRunTime);
      printf("[WDT] %lu 个任务运行中:\n", (unsigned long)taskCount);
      for (uint32_t i = 0; i < taskCount && i < 8; i++) {
        printf("  %-12s prio=%u stack=%Hu/%Hu\n",
          taskStatus[i].pcTaskName,
          taskStatus[i].uxCurrentPriority,
          taskStatus[i].usStackHighWaterMark,
          taskStatus[i].usStackHighWaterMark /* 实际已用栈 */
        );
      }
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(500));
  }
}

/* ========== main ========== */

int main(void)
{
  /* HAL 库初始化 */
  HAL_Init();

  /* 系统时钟 168MHz */
  SystemClock_Config();

  /* 外设初始化 */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART6_DBG_UART_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM3_PWM_Init();
  MX_TIM6_Init();
  MX_SPI1_Init();

  /* 启动 TIM6（μs 基准） */
  HAL_TIM_Base_Start(&htim6);

  printf("\n========================================\n");
  printf(" Smart PLC Runtime — STM32F407\n");
  printf(" SYSCLK=%luMHz, %luKB Flash, %luKB RAM\n",
         HAL_RCC_GetSysClockFreq() / 1000000UL, 1024UL, 192UL);
  printf("========================================\n\n");

  /* ===== PLC 运行时初始化 ===== */

  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "sensor_di_0", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "数字量输入 0 (PB0)");
  plc_var_register(vt, "sensor_di_1", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "数字量输入 1 (PB1)");
  plc_var_register(vt, "adc_ch_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "模拟量输入 0 (PA0, 12bit)");
  plc_var_register(vt, "relay_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "继电器 0 (PC0)");
  plc_var_register(vt, "relay_1", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "继电器 1 (PC1)");
  plc_var_register(vt, "pwm_0", VAR_TYPE_UINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_uint), "PWM 输出 0 (PA6)");

  printf("[PLC] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O 映射 */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_DI,  "DI_0",   "sensor_di_0", 0x10);
  plc_io_register(io, IO_TYPE_DI,  "DI_1",   "sensor_di_1", 0x11);
  plc_io_register(io, IO_TYPE_AI,  "AI_0",   "adc_ch_0",    0x00);
  plc_io_register(io, IO_TYPE_DO,  "DO_0",   "relay_0",     0x20);
  plc_io_register(io, IO_TYPE_DO,  "DO_1",   "relay_1",     0x21);
  plc_io_register(io, IO_TYPE_PWM, "PWM_0",  "pwm_0",       0x30);

  /* 绑定 I/O ↔ 变量 */
  for (uint16_t i = 0; i < io->channel_count; i++) {
    plc_io_bind(io, i, vt);
    printf("[PLC]   I/O %s → %s\n", io->channels[i].name, io->channels[i].var_name);
  }

  /* 加载并启动运行时 */
  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  printf("[PLC] 运行时已启动 (state=%d)\n", plc_runtime_get_state(&g_runtime));

  /* ===== 创建 FreeRTOS 任务 ===== */

  xTaskCreate(plc_scan_task, "PLC_Scan", 256, &g_runtime,
              configMAX_PRIORITIES - 1, NULL);
  xTaskCreate(comm_task, "Comm", 256, &g_runtime,
              configMAX_PRIORITIES - 2, NULL);
  xTaskCreate(led_task, "LED", 128, NULL, 1, NULL);

  printf("[STM32] FreeRTOS 任务已创建, 启动调度器...\n");

  /* 启动 FreeRTOS 调度器（永不返回） */
  vTaskStartScheduler();

  /* 异常: 调度器不应返回 */
  printf("[FATAL] 调度器异常返回!\n");
  for (;;);
}

/* ========== FreeRTOS 钩子 ========== */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
  (void)xTask;
  printf("[FATAL] 栈溢出! 任务: %s\n", pcTaskName);
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void vApplicationMallocFailedHook(void)
{
  printf("[FATAL] 内存分配失败!\n");
  taskDISABLE_INTERRUPTS();
  for (;;);
}

void vApplicationIdleHook(void)
{
  /* 空闲时进入低功耗（可选） */
}
