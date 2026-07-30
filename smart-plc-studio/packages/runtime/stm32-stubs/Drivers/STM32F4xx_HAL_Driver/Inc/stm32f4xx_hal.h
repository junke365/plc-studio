#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H
#include "stm32f4xx.h"

extern uint32_t SystemCoreClock;
void SystemInit(void);

HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
void HAL_IncTick(void);

HAL_StatusTypeDef HAL_RCC_OscConfig(void*);
HAL_StatusTypeDef HAL_RCC_ClockConfig(void*, uint32_t);
uint32_t HAL_RCC_GetSysClockFreq(void);

void HAL_GPIO_Init(GPIO_TypeDef*, void*);
void HAL_GPIO_DeInit(GPIO_TypeDef*, uint32_t);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef*, uint16_t);
void HAL_GPIO_WritePin(GPIO_TypeDef*, uint16_t, GPIO_PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef*, uint16_t);

typedef struct {
  USART_TypeDef* Instance;
  struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
  } Init;
} UART_HandleTypeDef;

typedef struct {
  ADC_TypeDef* Instance;
  struct {
    uint32_t ClockPrescaler;
    uint32_t Resolution;
    uint32_t ScanConvMode;
    uint32_t ContinuousConvMode;
    uint32_t DiscontinuousConvMode;
    uint32_t ExternalTrigConvEdge;
    uint32_t DataAlign;
    uint32_t NbrOfConversion;
    uint32_t DMAContinuousRequests;
    uint32_t EOCSelection;
  } Init;
} ADC_HandleTypeDef;

typedef struct {
  DAC_TypeDef* Instance;
} DAC_HandleTypeDef;

typedef struct {
  TIM_TypeDef* Instance;
  struct {
    uint32_t Prescaler;
    uint32_t CounterMode;
    uint32_t Period;
    uint32_t ClockDivision;
    uint32_t AutoReloadPreload;
  } Init;
} TIM_HandleTypeDef;

typedef struct {
  SPI_TypeDef* Instance;
  struct {
    uint32_t Mode;
    uint32_t Direction;
    uint32_t DataSize;
    uint32_t CLKPolarity;
    uint32_t CLKPhase;
    uint32_t NSS;
    uint32_t BaudRatePrescaler;
    uint32_t FirstBit;
    uint32_t TIMode;
    uint32_t CRCCalculation;
    uint32_t CRCPolynomial;
  } Init;
} SPI_HandleTypeDef;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;
extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t*, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef*, uint8_t*, uint16_t, uint32_t);
int HAL_UART_Receive_IT(UART_HandleTypeDef*, uint8_t*, uint16_t);

HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*, uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef*, void*);
void HAL_ADC_IRQHandler(ADC_HandleTypeDef*);

HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef*);
HAL_StatusTypeDef HAL_DAC_DeInit(DAC_HandleTypeDef*);
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef*, uint32_t);
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef*, void*, uint32_t);
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef*, uint32_t, uint32_t, uint32_t);

HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_DeInit(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef*);

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_PWM_DeInit(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*, uint32_t);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*, uint32_t);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef*, void*, uint32_t);

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef*);
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef*);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef*, uint8_t*, uint8_t*, uint16_t, uint32_t);

void HAL_UART_MspInit(UART_HandleTypeDef*);
void HAL_ADC_MspInit(ADC_HandleTypeDef*);
void HAL_DAC_MspInit(DAC_HandleTypeDef*);
void HAL_SPI_MspInit(SPI_HandleTypeDef*);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef*);
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef*);

#define ADC_CHANNEL_0             0
#define ADC_SAMPLETIME_84CYCLES   0
#define ADC_RESOLUTION_12B        0
#define ADC_CLOCK_SYNC_PCLK_DIV4 0
#define ADC_EOC_SINGLE_CONV       0
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0
#define ADC_DATAALIGN_RIGHT       0

#define DAC_CHANNEL_1             0
#define DAC_TRIGGER_NONE          0
#define DAC_OUTPUTBUFFER_ENABLE   0
#define DAC_ALIGN_12B_R           0

#define UART_WORDLENGTH_8B        0
#define UART_WORDLENGTH_9B        0
#define UART_STOPBITS_1           0
#define UART_STOPBITS_2           0
#define UART_PARITY_NONE          0
#define UART_PARITY_EVEN          0
#define UART_PARITY_ODD           0
#define UART_HWCONTROL_NONE       0
#define UART_OVERSAMPLING_16      0
#define UART_MODE_TX_RX           0

#define SPI_MODE_MASTER           0
#define SPI_DIRECTION_2LINES      0
#define SPI_DATASIZE_8BIT         0
#define SPI_POLARITY_LOW          0
#define SPI_PHASE_1EDGE           0
#define SPI_NSS_SOFT              0
#define SPI_BAUDRATEPRESCALER_16  0
#define SPI_FIRSTBIT_MSB          0
#define SPI_TIMODE_DISABLE        0
#define SPI_CRCCALCULATION_DISABLE 0

#define TIM_COUNTERMODE_UP        0
#define TIM_CLOCKDIVISION_DIV1    0
#define TIM_AUTORELOAD_PRELOAD_ENABLE 0
#define TIM_OCMODE_PWM1           0
#define TIM_OCPOLARITY_HIGH       0
#define TIM_OCFAST_DISABLE        0
#define TIM_CHANNEL_1             0

#define GPIO_AF7_USART1           7
#define GPIO_AF7_USART2           7
#define GPIO_AF8_USART6           8
#define GPIO_AF5_SPI1             5
#define GPIO_AF2_TIM3             2

#define RCC_PLLSOURCE_HSE         0
#define RCC_PLLP_DIV2             0
#define RCC_SYSCLKSOURCE_PLLCLK   0
#define RCC_SYSCLK_DIV1           0
#define RCC_HCLK_DIV4             0
#define RCC_HCLK_DIV2             0
#define FLASH_LATENCY_5           0

#define RCC_OSCILLATORTYPE_HSE    0
#define RCC_PLL_ON                0
#define RCC_CLOCKTYPE_HCLK        0
#define RCC_CLOCKTYPE_SYSCLK      0
#define RCC_CLOCKTYPE_PCLK1       0
#define RCC_CLOCKTYPE_PCLK2       0

typedef struct {
  uint32_t OscillatorType;
  uint32_t HSEState;
  struct {
    uint32_t PLLState;
    uint32_t PLLSource;
    uint32_t PLLM;
    uint32_t PLLN;
    uint32_t PLLP;
    uint32_t PLLQ;
  } PLL;
} RCC_OscInitTypeDef;

typedef struct {
  uint32_t ClockType;
  uint32_t SYSCLKSource;
  uint32_t AHBCLKDivider;
  uint32_t APB1CLKDivider;
  uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

typedef struct {
  uint32_t Channel;
  uint32_t Rank;
  uint32_t SamplingTime;
  uint32_t Offset;
} ADC_ChannelConfTypeDef;

typedef struct {
  uint32_t DAC_Trigger;
  uint32_t DAC_OutputBuffer;
} DAC_ChannelConfTypeDef;

typedef struct {
  uint32_t OCMode;
  uint32_t Pulse;
  uint32_t OCPolarity;
  uint32_t OCFastMode;
} TIM_OC_InitTypeDef;

typedef struct {
  uint32_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
  uint32_t Alternate;
} GPIO_InitTypeDef;

#define RCC_HSE_ON                0
#define RCC_PLLSOURCE_HSE         0
#define RCC_PLLP_DIV2             0
#define DISABLE                    0
#define ENABLE                     1

#endif
