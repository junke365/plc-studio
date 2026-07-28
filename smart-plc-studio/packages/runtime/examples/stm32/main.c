/**
 * stm32/main.c - STM32 + FreeRTOS PLC 运行时示例
 *
 * 演示在 STM32 + FreeRTOS 嵌入式平台上运行 PLC 运行时：
 * - HAL 初始化、GPIO、UART 初始化
 * - FreeRTOS 任务中执行 PLC 扫描
 * - printf 重定向到 UART2 调试输出
 * - 裸机风格的嵌入式 PLC 运行
 */

#include <stdio.h>
#include <string.h>
#include "plc_runtime.h"

/* ========== STM32 HAL 头文件（示例） ========== */

/* 实际项目中取消注释并使用正确的型号头文件 */
/* #include "stm32f4xx_hal.h" */

/* ========== HAL 函数桩（需替换为实际 HAL 调用） ========== */

static void SystemClock_Config(void)
{
  /* TODO: 配置系统时钟
   * 例如 STM32F407: HSE 8MHz -> PLL -> 168MHz SYSCLK
   *
   * RCC_OscInitTypeDef RCC_OscInitStruct = {0};
   * RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
   * RCC_OscInitStruct.HSEState = RCC_HSE_ON;
   * RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
   * ...
   */
}

static void MX_GPIO_Init(void)
{
  /* TODO: GPIO 初始化
   * GPIOA: LED 输出 (PA5)
   * GPIOB: 数字量输入 (PB0-PB3)
   * GPIOC: 数字量输出 (PC0-PC1)
   *
   * __HAL_RCC_GPIOA_CLK_ENABLE();
   * __HAL_RCC_GPIOB_CLK_ENABLE();
   * ...
   */
}

static void MX_USART2_UART_Init(void)
{
  /* TODO: UART2 初始化（115200 baud, 用于调试输出）
   * USART2: PA2(TX), PA3(RX)
   *
   * huart2.Instance = USART2;
   * huart2.Init.BaudRate = 115200;
   * huart2.Init.WordLength = UART_WORDLENGTH_8B;
   * huart2.Init.StopBits = UART_STOPBITS_1;
   * huart2.Init.Parity = UART_PARITY_NONE;
   * ...
   */
}

static void MX_ADC1_Init(void)
{
  /* TODO: ADC1 初始化（模拟量输入）
   * ADC1: PA0(AI0), PA1(AI1)
   */
}

static void MX_TIM3_PWM_Init(void)
{
  /* TODO: TIM3 PWM 初始化（模拟量输出）
   * TIM3_CH1: PA6(AO0) - PWM 输出
   */
}

/* ========== printf 重定向 ========== */

/* 重定向 printf 到 UART2（GCC + ARM 工具链） */
#ifdef __GNUC__
int _write(int fd, char* ptr, int len)
{
  (void)fd;
  int i;
  for (i = 0; i < len; i++) {
    /* TODO: 替换为实际的 HAL_UART_Transmit 调用
     * HAL_UART_Transmit(&huart2, (uint8_t*)&ptr[i], 1, HAL_MAX_DELAY);
     */
    (void)ptr[i];
  }
  return len;
}
#endif

/* ========== 全局状态 ========== */

static PlcRuntime g_runtime;

/* ========== 任务回调 ========== */

static void plc_task_callback(void* ctx)
{
  PlcRuntime* rt = (PlcRuntime*)ctx;
  plc_runtime_scan(rt);
}

static void comm_task_callback(void* ctx)
{
  (void)ctx;
  /* Modbus RTU 通信处理 */
}

/* ========== FreeRTOS 任务 ========== */

static void plc_scan_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  printf("[STM32] PLC 扫描任务已启动\n");

  for (;;) {
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);
    vTaskDelay(pdMS_TO_TICKS(1)); /* 1ms 周期 */
  }
}

static void comm_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  printf("[STM32] 通信任务已启动\n");

  for (;;) {
    /* 处理 Modbus RTU 请求 */
    vTaskDelay(pdMS_TO_TICKS(10)); /* 10ms 周期 */
  }
}

/* ========== LED 状态指示 ========== */

static void led_task(void* pvParameters)
{
  (void)pvParameters;

  for (;;) {
    /* TODO: HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); */
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/* ========== main 函数 ========== */

int main(void)
{
  printf("\n========================================\n");
  printf("Smart PLC Runtime - STM32 + FreeRTOS\n");
  printf("========================================\n");

  /* HAL 初始化 */
  HAL_Init();
  SystemClock_Config();

  /* 外设初始化 */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_PWM_Init();

  printf("[STM32] 系统时钟已配置\n");
  printf("[STM32] GPIO/UART/ADC/PWM 已初始化\n");

  /* PLC 运行时初始化 */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "adc_channel_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "ADC 通道 0 (12bit)");
  plc_var_register(vt, "pwm_output_0", VAR_TYPE_UINT, VAR_ATTR_OUTPUT,
                   sizeof(plc_uint), "PWM 输出通道 0");
  plc_var_register(vt, "relay_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "继电器 0");
  plc_var_register(vt, "relay_1", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "继电器 1");

  printf("[STM32] 已注册 %u 个变量\n", plc_var_count(vt));

  /* 配置 I/O 通道 */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_AI, "ADC_0", "adc_channel_0", 0x00);
  plc_io_register(io, IO_TYPE_PWM, "PWM_0", "pwm_output_0", 0x10);
  plc_io_register(io, IO_TYPE_DO,  "DO_0",  "relay_0",      0x20);
  plc_io_register(io, IO_TYPE_DO,  "DO_1",  "relay_1",      0x21);

  plc_io_bind(io, 0, vt); /* ADC_0 -> adc_channel_0 */
  plc_io_bind(io, 1, vt); /* PWM_0 -> pwm_output_0 */
  plc_io_bind(io, 2, vt); /* DO_0  -> relay_0 */
  plc_io_bind(io, 3, vt); /* DO_1  -> relay_1 */

  printf("[STM32] I/O: 1 AI + 1 PWM + 2 DO\n");

  /* 创建 FreeRTOS 任务 */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  1, 200, plc_task_callback, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  10, 100, comm_task_callback, &g_runtime);

  /* 加载并启动 */
  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  /* 创建 FreeRTOS 任务 */
  xTaskCreate(plc_scan_task, "PLC_Scan", PLC_STACK_SIZE / sizeof(StackType_t),
              &g_runtime, configMAX_PRIORITIES - 1, NULL);
  xTaskCreate(comm_task, "Comm", PLC_STACK_SIZE / sizeof(StackType_t),
              &g_runtime, configMAX_PRIORITIES - 2, NULL);
  xTaskCreate(led_task, "LED", 128, NULL, 1, NULL);

  printf("[STM32] FreeRTOS 任务已创建\n");
  printf("[STM32] 启动调度器...\n");

  /* 启动 FreeRTOS 调度器（不返回） */
  vTaskStartScheduler();

  /* 不应到达此处 */
  for (;;) {
  }
}

/* ========== FreeRTOS 钩子函数 ========== */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
  (void)xTask;
  printf("[STM32] 栈溢出! 任务: %s\n", pcTaskName);
  for (;;) {
  }
}

void vApplicationMallocFailedHook(void)
{
  printf("[STM32] 内存分配失败!\n");
  for (;;) {
  }
}
