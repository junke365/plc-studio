/**
 * esp32/main.c - ESP32 ESP-IDF PLC 运行时示例
 *
 * 演示在 ESP32 + FreeRTOS (ESP-IDF) 平台上运行 PLC 运行时：
 * - app_main() 入口
 * - GPIO/ADC/PWM I/O 配置
 * - WiFi 初始化用于 Modbus TCP
 * - UART 初始化用于 Modbus RTU
 * - FreeRTOS 任务执行 PLC 扫描
 * - 堆内存统计
 */

#include <stdio.h>
#include <string.h>
#include "plc_runtime.h"

/* ESP-IDF 头文件 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

static const char* TAG = "PLC";

/* ========== I/O 引脚配置 ========== */

#define GPIO_INPUT_0     4    /* 数字量输入 0 */
#define GPIO_INPUT_1     5    /* 数字量输入 1 */
#define GPIO_OUTPUT_0    18   /* 数字量输出 0 */
#define GPIO_OUTPUT_1    19   /* 数字量输出 1 */
#define ADC_CHANNEL_0    ADC_CHANNEL_0  /* 模拟量输入 0 (GPIO36) */
#define PWM_OUTPUT_0     21   /* PWM 输出 0 */

/* UART 配置（Modbus RTU） */
#define UART_PORT        UART_NUM_1
#define UART_TX_PIN      17
#define UART_RX_PIN      16
#define UART_BAUD_RATE   115200

/* WiFi 配置 */
#define WIFI_SSID        "your_wifi_ssid"
#define WIFI_PASS        "your_wifi_password"
#define MODBUS_TCP_PORT  502

/* ========== WiFi 初始化桩 ========== */

static void wifi_init_stub(void)
{
  ESP_LOGI(TAG, "WiFi 初始化 (用于 Modbus TCP)");
  ESP_LOGI(TAG, "  SSID: %s", WIFI_SSID);
  ESP_LOGI(TAG, "  端口: %u", MODBUS_TCP_PORT);

  /* TODO: 实际 WiFi 初始化
   * esp_netif_init();
   * esp_event_loop_create_default();
   * esp_netif_create_default_wifi_sta();
   * wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   * esp_wifi_init(&cfg);
   * esp_wifi_set_mode(WIFI_MODE_STA);
   * esp_wifi_start();
   * esp_wifi_connect();
   */

  ESP_LOGI(TAG, "WiFi 初始化桩完成（需替换为实际实现）");
}

/* ========== UART 初始化桩 ========== */

static void uart_init_stub(void)
{
  ESP_LOGI(TAG, "UART 初始化 (Modbus RTU)");
  ESP_LOGI(TAG, "  端口: UART%d", UART_PORT);
  ESP_LOGI(TAG, "  TX: GPIO%d, RX: GPIO%d", UART_TX_PIN, UART_RX_PIN);
  ESP_LOGI(TAG, "  波特率: %d", UART_BAUD_RATE);

  /* TODO: 实际 UART 初始化
   * uart_config_t uart_config = {
   *   .baud_rate = UART_BAUD_RATE,
   *   .data_bits = UART_DATA_8_BITS,
   *   .parity = UART_PARITY_DISABLE,
   *   .stop_bits = UART_STOP_BITS_1,
   *   .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
   * };
   * uart_param_config(UART_PORT, &uart_config);
   * uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, -1, -1);
   * uart_driver_install(UART_PORT, 256, 256, 0, NULL, 0);
   */
}

/* ========== 全局状态 ========== */

static PlcRuntime g_runtime;

/* ========== 任务回调 ========== */

static void plc_task_callback(void* ctx)
{
  PlcRuntime* rt = (PlcRuntime*)ctx;
  plc_runtime_scan(rt);
}

/* ========== FreeRTOS 任务 ========== */

static void plc_scan_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  ESP_LOGI(TAG, "PLC 扫描任务启动");

  for (;;) {
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);
    vTaskDelay(pdMS_TO_TICKS(1)); /* 1ms 周期 */
  }
}

/* ========== 堆内存统计 ========== */

static void print_heap_stats(void)
{
  /* 总堆大小 */
  size_t heap_total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t heap_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
  size_t heap_used = heap_total - heap_free;

  ESP_LOGI(TAG, "堆内存统计:");
  ESP_LOGI(TAG, "  总大小: %u KB", (unsigned)(heap_total / 1024));
  ESP_LOGI(TAG, "  已使用: %u KB (%.1f%%)",
           (unsigned)(heap_used / 1024),
           (float)heap_used / (float)heap_total * 100.0f);
  ESP_LOGI(TAG, "  剩余:   %u KB", (unsigned)(heap_free / 1024));
  ESP_LOGI(TAG, "  最小值: %u KB", (unsigned)(heap_min_free / 1024));

  /* PSRAM（如果存在） */
  size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (psram_total > 0) {
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "  PSRAM: %u KB (剩余 %u KB)",
             (unsigned)(psram_total / 1024),
             (unsigned)(psram_free / 1024));
  }
}

/* ========== app_main ========== */

void app_main(void)
{
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "Smart PLC Runtime - ESP32 示例");
  ESP_LOGI(TAG, "========================================");

  /* 外设初始化 */
  uart_init_stub();
  wifi_init_stub();

  /* PLC 运行时初始化 */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "gpio_input_0", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "GPIO 输入 0");
  plc_var_register(vt, "gpio_output_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "GPIO 输出 0");
  plc_var_register(vt, "adc_value_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "ADC 值 (12bit)");
  plc_var_register(vt, "pwm_duty_0", VAR_TYPE_UINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_uint), "PWM 占空比 (0-1023)");

  ESP_LOGI(TAG, "已注册 %u 个变量", plc_var_count(vt));

  /* 配置 I/O 通道 */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_DI, "DI_0", "gpio_input_0", GPIO_INPUT_0);
  plc_io_register(io, IO_TYPE_DO, "DO_0", "gpio_output_0", GPIO_OUTPUT_0);
  plc_io_register(io, IO_TYPE_AI, "AI_0", "adc_value_0", ADC_CHANNEL_0);
  plc_io_register(io, IO_TYPE_PWM, "PWM_0", "pwm_duty_0", PWM_OUTPUT_0);

  plc_io_bind(io, 0, vt);
  plc_io_bind(io, 1, vt);
  plc_io_bind(io, 2, vt);
  plc_io_bind(io, 3, vt);

  ESP_LOGI(TAG, "I/O: 1 DI + 1 DO + 1 AI + 1 PWM");

  /* 创建任务 */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  1, 200, plc_task_callback, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  10, 100, NULL, &g_runtime);

  ESP_LOGI(TAG, "任务: MainTask(1ms), CommTask(10ms)");

  /* 加载并启动 */
  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  /* 创建 FreeRTOS 任务 */
  xTaskCreatePinnedToCore(
    plc_scan_task,
    "plc_scan",
    PLC_STACK_SIZE / sizeof(StackType_t),
    &g_runtime,
    configMAX_PRIORITIES - 1,
    NULL,
    1  /* 核心 1（核心 0 运行 WiFi/蓝牙） */
  );

  ESP_LOGI(TAG, "PLC 运行时已启动");

  /* 打印初始堆内存统计 */
  print_heap_stats();

  /* 主循环：定时打印统计 */
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000)); /* 每 5 秒 */

    PlcStats stats;
    plc_runtime_get_stats(&g_runtime, &stats);

    ESP_LOGI(TAG, "运行: %u s | 周期: %u us | 最大: %u us | 错误: %u",
             stats.uptime_ms / 1000,
             stats.cycle_time_us,
             stats.max_cycle_time_us,
             stats.error_count);

    print_heap_stats();
  }
}
