/**
 * esp32/platform.c - ESP32 平台适配
 *
 * 基于 ESP-IDF 和 FreeRTOS 实现 PLC 运行时 HAL
 * 使用 UART 进行 Modbus RTU，WiFi/TCP 进行 Modbus TCP
 * 适用于 ESP32/ESP32-S2/ESP32-S3 系列芯片
 */

#include "plc_platform.h"
#include "plc_io.h"
#include "plc_comm.h"

/* ESP-IDF 头文件 */
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "driver/uart_vfs.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

/* FreeRTOS 头文件 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/* 网络头文件 */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "esp_system.h"

static const char* TAG = "PLC_HAL";

/* ========== 临界区 ========== */

static SemaphoreHandle_t g_critical_mutex = NULL;
static portMUX_TYPE g_spinlock = portMUX_INITIALIZER_UNLOCKED;

/* ========== 平台初始化 ========== */

void plc_platform_init(void)
{
  /* 创建互斥锁 */
  g_critical_mutex = xSemaphoreCreateMutex();

  /* 配置 ADC */
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);

  plc_platform_log(PLC_LOG_INFO, "ESP32 平台初始化完成, CPU=%luMHz",
    (unsigned long)esp_clk_cpu_freq() / 1000000);
}

/* ========== 时间函数 ========== */

uint32_t plc_platform_tick_ms(void)
{
  return (uint32_t)(esp_timer_get_time() / 1000);
}

uint64_t plc_platform_tick_us(void)
{
  return (uint64_t)esp_timer_get_time();
}

void plc_platform_delay_ms(uint32_t ms)
{
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void plc_platform_delay_us(uint32_t us)
{
  if (us <= 1000) {
    /* 短延时使用 esp_timer 忙等 */
    uint64_t target = esp_timer_get_time() + us;
    while ((uint64_t)esp_timer_get_time() < target) {
      ets_delay_us(1);
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
  portENTER_CRITICAL(&g_spinlock);
}

void plc_platform_critical_exit(void)
{
  portEXIT_CRITICAL(&g_spinlock);
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

  int64_t now_us = esp_timer_get_time();
  uint32_t total_ms = (uint32_t)(now_us / 1000);
  uint32_t ms = total_ms % 1000;
  uint32_t s = (total_ms / 1000) % 60;
  uint32_t m = (total_ms / 60000) % 60;
  uint32_t h = (total_ms / 3600000) % 24;

  offset += snprintf(buf + offset, sizeof(buf) - offset,
    "[%02lu:%02lu:%02lu.%03lu] [%s] ",
    (unsigned long)h, (unsigned long)m, (unsigned long)s,
    (unsigned long)ms, g_log_level_names[level]);

  va_list args;
  va_start(args, fmt);
  offset += vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
  va_end(args);

  /* 使用 ESP-IDF 日志系统或直接输出到 UART0 */
  ESP_LOGI(TAG, "%s", buf);
}

/* ========== I/O 硬件抽象层 ========== */

int32_t plc_hal_read_input(uint32_t physical_addr, IoType type)
{
  switch (type) {
    case IO_TYPE_DI: {
      /* 数字输入: physical_addr = GPIO 编号 */
      gpio_num_t gpio = (gpio_num_t)(physical_addr & 0x3F);
      return gpio_get_level(gpio) ? 1 : 0;
    }
    case IO_TYPE_AI: {
      /* 模拟输入: physical_addr = ADC 通道 (0-7 for ADC1, 8-15 for ADC2) */
      uint32_t ch = physical_addr & 0x0F;
      if (ch < 8) {
        adc1_channel_t adc_ch = (adc1_channel_t)ch;
        return (int32_t)adc1_get_raw(adc_ch);
      } else {
        /* ADC2 通道（WiFi 使用时不可用） */
        adc2_channel_t adc_ch = (adc2_channel_t)(ch - 8);
        int raw = 0;
        if (adc2_get_raw(adc_ch, ADC_WIDTH_BIT_12, &raw) == ESP_OK) {
          return (int32_t)raw;
        }
        return 0;
      }
    }
    case IO_TYPE_ENCODER:
    case IO_TYPE_COUNTER:
      return 0;
    default:
      return 0;
  }
}

void plc_hal_write_output(uint32_t physical_addr, IoType type, int32_t value)
{
  switch (type) {
    case IO_TYPE_DO: {
      gpio_num_t gpio = (gpio_num_t)(physical_addr & 0x3F);
      gpio_set_level(gpio, value ? 1 : 0);
      break;
    }
    case IO_TYPE_AO: {
      /* DAC 输出: physical_addr = DAC 通道 (0 或 1, 仅 ESP32 原版) */
      dac_output_enable((dac_channel_t)(physical_addr & 0x01));
      dac_output_voltage((dac_channel_t)(physical_addr & 0x01),
        (uint32_t)(value & 0xFF)); /* 8位 DAC */
      break;
    }
    case IO_TYPE_PWM: {
      /* PWM 输出通过 LEDC 实现 */
      uint32_t duty = (uint32_t)(value & 0x1FFF); /* 13位分辨率 */
      ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)(physical_addr & 0x07), duty);
      ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)(physical_addr & 0x07));
      break;
    }
    default:
      break;
  }
}

/* ========== SPI I/O 扩展器接口 ========== */

static spi_device_handle_t g_spi_dev = NULL;

void plc_hal_spi_init(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs)
{
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = mosi,
    .miso_io_num = miso,
    .sclk_io_num = sclk,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 64
  };

  spi_device_interface_config_t dev_cfg = {
    .clock_speed_hz = 1000000,
    .mode = 0,
    .spics_io_num = cs,
    .queue_size = 1
  };

  spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  spi_bus_add_device(SPI2_HOST, &dev_cfg, &g_spi_dev);
}

int plc_hal_spi_transfer(const uint8_t* tx, uint8_t* rx, uint32_t len)
{
  if (!g_spi_dev) return -1;

  spi_transaction_t t = {
    .length = len * 8,
    .tx_buffer = tx,
    .rx_buffer = rx
  };

  esp_err_t ret = spi_device_transmit(g_spi_dev, &t);
  return (ret == ESP_OK) ? 0 : -1;
}

/* ========== TCP 通信 ========== */

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  char port_str[8];
  snprintf(port_str, sizeof(port_str), "%u", port);

  int err = getaddrinfo(host, port_str, &hints, &res);
  if (err != 0 || !res) return -1;

  int fd = socket(res->ai_family, res->ai_socktype, 0);
  if (fd < 0) {
    freeaddrinfo(res);
    return -1;
  }

  /* 设置非阻塞 */
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  err = connect(fd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  if (err < 0 && errno != EINPROGRESS) {
    close(fd);
    return -1;
  }

  /* 等待连接完成 */
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLOUT;
  int ret = poll(&pfd, 1, (int)timeout_ms);

  if (ret <= 0) {
    close(fd);
    return -1;
  }

  int sock_err = 0;
  socklen_t len = sizeof(sock_err);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &len);
  if (sock_err) {
    close(fd);
    return -1;
  }

  /* 恢复阻塞模式 */
  fcntl(fd, F_SETFL, flags);

  int opt = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

  return fd;
}

void plc_hal_tcp_close(int fd)
{
  if (fd >= 0) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
  }
}

int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len)
{
  uint32_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, data + sent, len - sent, 0);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    sent += (uint32_t)n;
  }
  return (int)sent;
}

int plc_hal_tcp_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, (int)timeout_ms);
  if (ret <= 0) return (ret == 0) ? 0 : -1;

  ssize_t n = recv(fd, data, max_len, 0);
  return (n >= 0) ? (int)n : -1;
}

/* ========== 串口通信 ========== */

int plc_hal_serial_open(const char* port_name, uint32_t baud_rate,
                        uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
  /* 根据端口名称选择 UART 编号 */
  uart_port_t uart_num = UART_NUM_1;
  if (strcmp(port_name, "UART0") == 0 || strcmp(port_name, "/dev/ttyS0") == 0) {
    uart_num = UART_NUM_0;
  } else if (strcmp(port_name, "UART2") == 0 || strcmp(port_name, "/dev/ttyS2") == 0) {
    uart_num = UART_NUM_2;
  }

  /* UART 配置 */
  uart_config_t uart_cfg = {
    .baud_rate = (int)baud_rate,
    .data_bits = (data_bits == 9) ? UART_DATA_9_BITS : UART_DATA_8_BITS,
    .stop_bits = (stop_bits == 2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1,
    .parity = (parity == 1) ? UART_PARITY_EVEN :
              (parity == 2) ? UART_PARITY_ODD : UART_PARITY_DISABLE,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    .source_clk = UART_SCLK_DEFAULT,
  };

  /* 安装驱动 */
  esp_err_t err = uart_driver_install(uart_num, 1024, 256, 0, NULL, 0);
  if (err != ESP_OK) return -1;

  err = uart_param_config(uart_num, &uart_cfg);
  if (err != ESP_OK) return -1;

  /* GPIO 映射 (默认 UART1: TX=GPIO10, RX=GPIO9) */
  if (uart_num == UART_NUM_1) {
    uart_set_pin(uart_num, GPIO_NUM_10, GPIO_NUM_9,
      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  }

  return (int)uart_num;
}

void plc_hal_serial_close(int fd)
{
  if (fd >= 0) {
    uart_driver_delete((uart_port_t)fd);
  }
}

int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len)
{
  int written = uart_write_bytes((uart_port_t)fd, data, len);
  return (written >= 0) ? written : -1;
}

int plc_hal_serial_recv(int fd, uint8_t* data, uint32_t max_len, uint32_t timeout_ms)
{
  int len = uart_read_bytes((uart_port_t)fd, data, max_len,
    pdMS_TO_TICKS(timeout_ms));
  return (len >= 0) ? len : -1;
}
