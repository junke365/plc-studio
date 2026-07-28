/**
 * stm32/platform.c - STM32 平台适配
 *
 * 基于 STM32 HAL 库和 FreeRTOS 实现 PLC 运行时 HAL
 * 适用于 STM32F4/F7 系列 MCU，使用 USART 进行 Modbus RTU 通信
 */

#define PLATFORM_STM32

#include "plc_platform.h"
#include "plc_io.h"
#include "plc_comm.h"

/* STM32 HAL 头文件 */
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_dac.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_spi.h"
#include "stm32f4xx_hal_rcc.h"

/* FreeRTOS 头文件 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphora.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ========== 外部硬件句柄（在 main.c 中初始化） ========== */

extern UART_HandleTypeDef huart1;   /* Modbus RTU 主串口 */
extern UART_HandleTypeDef huart2;   /* 调试串口 */
extern SPI_HandleTypeDef  hspi1;    /* SPI I/O 扩展器 */
extern ADC_HandleTypeDef  hadc1;    /* ADC */
extern DAC_HandleTypeDef  hdac;     /* DAC */
extern TIM_HandleTypeDef  htim6;    /* 微秒延时定时器 */

/* ========== 临界区 ========== */

static SemaphoreHandle_t g_critical_mutex = NULL;
static uint32_t g_systick_us_per_tick = 1;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  /* 创建互斥锁 */
  g_critical_mutex = xSemaphoreCreateMutex();

  /* 配置 SysTick */
  g_systick_us_per_tick = SystemCoreClock / 1000000;

  /* 启动基本定时器（用于微秒延时） */
  HAL_TIM_Base_Start(&htim6);

  plc_platform_log(PLC_LOG_INFO, "STM32 平台初始化完成, FCLK=%luHz",
    (unsigned long)SystemCoreClock);
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  return HAL_GetTick();
}

uint64_t plc_platform_tick_us(void)
{
  /* 使用 DWT CYCCNT 获取高精度时间戳 */
  static int dwt_started = 0;
  if (!dwt_started) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    dwt_started = 1;
  }
  return (uint64_t)(DWT->CYCCNT / g_systick_us_per_tick);
}

void plc_platform_delay_ms(uint32_t ms)
{
  HAL_Delay(ms);
}

void plc_platform_delay_us(uint32_t us)
{
  /*
   * 短延时使用 DWT CYCCNT 忙等
   * 长延时使用 FreeRTOS vTaskDelay
   */
  if (us <= 1000) {
    uint64_t target = plc_platform_tick_us() + us;
    while (plc_platform_tick_us() < target) {
      __NOP();
    }
  } else {
    vTaskDelay(pdMS_TO_TICKS(us / 1000 + 1));
  }
}

/* ========== 临界区 ========== */

void plc_platform_critical_enter(void)
{
  if (g_critical_mutex) {
    xSemaphoreTake(g_critical_mutex, portMAX_DELAY);
  }
  taskENTER_CRITICAL();
}

void plc_platform_critical_exit(void)
{
  taskEXIT_CRITICAL();
  if (g_critical_mutex) {
    xSemaphoreGive(g_critical_mutex);
  }
}

/* ========== 内存管理 ========== */

void* plc_platform_malloc(size_t size)
{
  return pvPortMalloc(size);
}

void plc_platform_free(void* ptr)
{
  vPortFree(ptr);
}

/* ========== 日志 ========== */

static const char* g_log_level_names[] = {
  "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"
};

void plc_platform_log(uint8_t level, const char* fmt, ...)
{
  if (level > PLC_LOG_DEBUG) return;

  char buf[256];
  int offset = 0;

  /* 时间戳 */
  uint32_t tick = HAL_GetTick();
  uint32_t ms = tick % 1000;
  uint32_t sec_total = tick / 1000;
  uint32_t s = sec_total % 60;
  uint32_t m = (sec_total / 60) % 60;
  uint32_t h = (sec_total / 3600) % 24;

  offset += snprintf(buf + offset, sizeof(buf) - offset,
    "[%02lu:%02lu:%02lu.%03lu] [%s] ",
    (unsigned long)h, (unsigned long)m, (unsigned long)s,
    (unsigned long)ms, g_log_level_names[level]);

  va_list args;
  va_start(args, fmt);
  offset += vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
  va_end(args);

  /* 通过调试串口输出 */
  HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)offset, 100);
}

/* ========== I/O 硬件抽象层 ========== */

/*
 * 读取数字输入
 * physical_addr 编码: GPIO端口索引<<4 | 引脚号
 * 例如 PA0 = (0<<4)|0 = 0, PB3 = (1<<4)|3 = 19
 */
static const GPIO_TypeDef* g_gpio_ports[] = {
  GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG
};

static const uint16_t g_gpio_pins[] = {
  GPIO_PIN_0,  GPIO_PIN_1,  GPIO_PIN_2,  GPIO_PIN_3,
  GPIO_PIN_4,  GPIO_PIN_5,  GPIO_PIN_6,  GPIO_PIN_7,
  GPIO_PIN_8,  GPIO_PIN_9,  GPIO_PIN_10, GPIO_PIN_11,
  GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
};

static inline GPIO_TypeDef* di_port(uint32_t addr)
{
  uint32_t idx = (addr >> 4) & 0x0F;
  return (idx < 7) ? (GPIO_TypeDef*)g_gpio_ports[idx] : GPIOA;
}

static inline uint16_t di_pin(uint32_t addr)
{
  return g_gpio_pins[addr & 0x0F];
}

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI: {
      GPIO_PinState state = HAL_GPIO_ReadPin(di_port(physical_addr), di_pin(physical_addr));
      return (state == GPIO_PIN_SET) ? 1 : 0;
    }
    case IO_TYPE_AI: {
      /* ADC 通道从 physical_addr 指定的通道号读取 */
      ADC_ChannelConfTypeDef sConfig = {0};
      sConfig.Channel = physical_addr & 0x0F;
      sConfig.Rank = 1;
      sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
      HAL_ADC_ConfigChannel(&hadc1, &sConfig);

      HAL_ADC_Start(&hadc1);
      HAL_ADC_PollForConversion(&hadc1, 10);
      uint32_t raw = HAL_ADC_GetValue(&hadc1);
      HAL_ADC_Stop(&hadc1);

      return (int32_t)raw;
    }
    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER:
      /* 编码器/计数器暂返回 0 */
      return 0;
    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO:
      HAL_GPIO_WritePin(di_port(physical_addr), di_pin(physical_addr),
        value ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;

    case IO_TYPE_AO: {
      /* DAC 输出: physical_addr = 通道 (0 或 1) */
      uint32_t dac_val = (uint32_t)(value & 0x0FFF); /* 12位 DAC */
      HAL_DAC_Start(&hdac, DAC_CHANNEL_1 + (physical_addr & 0x01));
      HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1 + (physical_addr & 0x01),
        DAC_ALIGN_12B_R, dac_val);
      break;
    }
    case IO_TYPE_PWM: {
      /* PWM 输出通过定时器实现 */
      uint32_t pulse = (uint32_t)(value & 0xFFFF);
      __HAL_TIM_SET_COMPARE(&htim6, TIM_CHANNEL_1, pulse);
      break;
    }
    default:
      break;
  }
}

/* ========== SPI I/O 扩展器接口 ========== */

/**
 * SPI 全双工传输（用于 MCP23S17、PCF8574 等 I/O 扩展器）
 * @param tx 发送缓冲区
 * @param rx 接收缓冲区
 * @param len 传输字节数
 * @return 0=成功
 */
int plc_hal_spi_transfer(const uint8_t* tx, uint8_t* rx, uint32_t len)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); /* 拉低 CS */
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1,
    (uint8_t*)tx, rx, (uint16_t)len, 100);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); /* 拉高 CS */
  return (status == HAL_OK) ? 0 : -1;
}

/* ========== TCP 通信 ========== */

/*
 * STM32 通过以太网 (lwIP) 或外部 WiFi 模块实现 TCP
 * 这里提供基于 lwIP 的实现
 */

/* lwIP 头文件（如果可用） */
#if __has_include("lwip/sockets.h")
  #include "lwip/sockets.h"
  #include "lwip/netdb.h"
  #define HAS_LWIP 1
#else
  #define HAS_LWIP 0
#endif

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
#if HAS_LWIP
  int fd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) return -1;

  /* 设置非阻塞 */
  int flags = 1;
  lwip_ioctl(fd, FIONBIO, &flags);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  /* DNS 解析 */
  struct hostent* he = gethostbyname(host);
  if (!he) {
    lwip_close(fd);
    return -1;
  }
  memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

  int ret = lwip_connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret < 0 && errno != EINPROGRESS) {
    lwip_close(fd);
    return -1;
  }

  /* 等待连接完成 */
  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < timeout_ms) {
    int err = 0;
    socklen_t len = sizeof(err);
    lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err == 0) break;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  /* 恢复阻塞模式 */
  flags = 0;
  lwip_ioctl(fd, FIONBIO, &flags);

  return fd;
#else
  (void)host; (void)port; (void)timeout_ms;
  plc_platform_log(PLC_LOG_ERROR, "lwIP 不可用，TCP 连接未实现");
  return -1;
#endif
}

void plc_hal_tcp_close(int fd)
{
#if HAS_LWIP
  if (fd >= 0) lwip_close(fd);
#endif
}

int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len)
{
#if HAS_LWIP
  uint32_t sent = 0;
  while (sent < len) {
    ssize_t n = lwip_send(fd, data + sent, len - sent, 0);
    if (n < 0) return -1;
    sent += (uint32_t)n;
  }
  return (int)sent;
#else
  (void)fd; (void)data; (void)len;
  return -1;
#endif
}

int plc_hal_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
#if HAS_LWIP
  /* 设置接收超时 */
  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  ssize_t n = lwip_recv(fd, data, max_len, 0);
  return (n >= 0) ? (int)n : -1;
#else
  (void)fd; (void)data; (void)max_len; (void)timeout_ms;
  return -1;
#endif
}

/* ========== 串口通信 ========== */

static UART_HandleTypeDef* g_serial_handles[4] = { &huart1, &huart2, NULL, NULL };

int plc_hal_serial_open(const char* port_name, uint32_t baud_rate,
                        uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
  /* STM32 串口在 CubeMX 中预先配置，这里仅修改波特率 */
  UART_HandleTypeDef* huart = NULL;

  if (strcmp(port_name, "UART1") == 0 || strcmp(port_name, "/dev/ttyS0") == 0) {
    huart = &huart1;
  } else if (strcmp(port_name, "UART2") == 0 || strcmp(port_name, "/dev/ttyS1") == 0) {
    huart = &huart2;
  }

  if (!huart) return -1;

  /* 修改波特率 */
  HAL_UART_DeInit(huart);
  huart->Init.BaudRate = baud_rate;
  huart->Init.WordLength = (data_bits == 9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
  huart->Init.StopBits = (stop_bits == 2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
  huart->Init.Parity = (parity == 1) ? UART_PARITY_EVEN :
                        (parity == 2) ? UART_PARITY_ODD : UART_PARITY_NONE;
  HAL_UART_Init(huart);

  /* 作为返回值：用指针地址的低 32 位作为 fd（STM32 单地址空间） */
  return (int)((uintptr_t)huart & 0xFF);
}

void plc_hal_serial_close(int fd)
{
  /* STM32 串口通常不关闭，保持使能 */
  (void)fd;
}

int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len)
{
  UART_HandleTypeDef* huart = &huart1; /* 默认使用 UART1 */
  HAL_StatusTypeDef status = HAL_UART_Transmit(huart, (uint8_t*)data,
    (uint16_t)len, 1000);
  return (status == HAL_OK) ? (int)len : -1;
}

int plc_hal_serial_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  UART_HandleTypeDef* huart = &huart1;

  uint8_t byte;
  uint32_t received = 0;
  uint32_t start = HAL_GetTick();

  while (received < max_len && (HAL_GetTick() - start) < timeout_ms) {
    if (HAL_UART_Receive(huart, &byte, 1, 1) == HAL_OK) {
      data[received++] = byte;
    }
  }

  return (int)received;
}
