# 运行时开发指南

## 概述

PLC Studio 运行时使用 C11 标准编写，支持多种嵌入式平台。本文档介绍如何开发和扩展运行时功能。

## 构建系统

### 依赖

- CMake >= 3.20
- GCC/Clang (Linux)
- MSVC (Windows)
- ARM GCC (交叉编译)

### 构建步骤

```bash
cd smart-plc-studio/packages/runtime

# 创建构建目录
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| CMAKE_BUILD_TYPE | Debug | 构建类型 |
| PLC_ENABLE_HMI | ON | 启用 HMI 支持 |
| PLC_ENABLE_PROTOCOL | ON | 启用通信协议 |
| PLC_PLATFORM | linux-x86 | 目标平台 |

## 模块开发

### 1. Core 模块

Core 模块负责 PLC 扫描周期和任务调度。

**关键结构：**

```c
// 任务控制块
typedef struct {
    uint32_t id;
    uint32_t period_ms;
    uint32_t priority;
    void (*execute)(void);
} PLC_Task;

// 变量表
typedef struct {
    char name[64];
    PLC_Type type;
    void *value;
    uint32_t flags;
} PLC_Variable;
```

**扫描周期：**

```c
void plc_scan_cycle(void) {
    plc_task_read_inputs();    // 读取输入
    plc_task_execute_program(); // 执行用户程序
    plc_task_write_outputs();   // 写入输出
    plc_task_update_hmi();      // 更新 HMI
}
```

### 2. HAL 模块

硬件抽象层提供统一的硬件访问接口。

**接口定义：**

```c
// GPIO
int plc_gpio_init(uint32_t pin, PLC_GPIO_Mode mode);
int plc_gpio_write(uint32_t pin, int value);
int plc_gpio_read(uint32_t pin);

// ADC
int plc_adc_init(uint32_t channel, uint32_t resolution);
int plc_adc_read(uint32_t channel);

// PWM
int plc_pwm_init(uint32_t channel, uint32_t frequency);
int plc_pwm_set_duty(uint32_t channel, float duty);

// 通信
int plc_uart_init(uint32_t baudrate);
int plc_spi_init(uint32_t mode, uint32_t speed);
int plc_i2c_init(uint32_t address);
int plc_can_init(uint32_t bitrate);
```

### 3. HMI 模块

HMI 模块负责界面渲染和用户交互。

**渲染流程：**

```c
void plc_hmi_render(void) {
    plc_hmi_update_data();    // 更新数据绑定
    plc_hmi_draw_widgets();   // 绘制控件
    plc_hmi_handle_input();   // 处理输入
}
```

**支持的平台：**

| 平台 | 文件 | 说明 |
|------|------|------|
| LVGL | hmi_lvgl.c | 嵌入式 GUI 库 |
| SDL2 | hmi_sdl2.c | 跨平台图形库 |
| Linux FB | hmi_fb.c | Linux Framebuffer |
| Windows | hmi_win32.c | Win32 API |

### 4. Protocol 模块

通信协议模块支持工业现场总线。

**Modbus 示例：**

```c
// Modbus TCP 服务器
PLC_Modbus_Server server;
plc_modbus_tcp_init(&server, 502);

// 寄存器映射
plc_modbus_register_holding(&server, 0, 100, holding_regs);
plc_modbus_register_input(&server, 0, 50, input_regs);
plc_modbus_register_coil(&server, 0, 32, coils);
```

## 平台移植

### 移植步骤

1. 在 `platform/` 目录创建新平台文件夹
2. 实现 HAL 接口
3. 实现 HMI 平台驱动
4. 添加 CMake 工具链文件
5. 测试和验证

### ESP32 移植示例

```c
// platform/esp32/platform.c
#include "plc_platform.h"
#include "driver/gpio.h"
#include "driver/adc.h"

void plc_platform_init(void) {
    // 初始化 GPIO
    gpio_install_isr_service(0);
    
    // 初始化 ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
}

uint32_t plc_platform_get_tick(void) {
    return esp_timer_get_time() / 1000;
}
```

### STM32 移植示例

```c
// platform/stm32/platform.c
#include "plc_platform.h"
#include "stm32f4xx_hal.h"

void plc_platform_init(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USART1_UART_Init();
}

uint32_t plc_platform_get_tick(void) {
    return HAL_GetTick();
}
```

## 调试

### 日志

```c
#include "plc_debug.h"

PLC_LOG_INFO("System initialized");
PLC_LOG_WARNING("Watchdog timeout");
PLC_LOG_ERROR("Memory allocation failed");
```

### 断言

```c
#include "plc_assert.h"

PLC_ASSERT(task != NULL);
PLC_ASSERT_RETURN(task != NULL, PLC_ERR_INVALID_PARAM);
```

### 性能分析

```c
PLC_PERF_START("scan_cycle");
plc_scan_cycle();
PLC_PERF_END("scan_cycle");
```

## 测试

### 单元测试

```bash
cd smart-plc-studio/packages/runtime
mkdir build-test && cd build-test
cmake .. -DPLC_ENABLE_TESTS=ON
make
./plc_test
```

### 集成测试

使用 QEMU 或硬件在环 (HIL) 测试。

## API 参考

详细的 API 文档请参阅头文件：

- `plc_runtime.h` - 运行时核心
- `plc_io.h` - 输入输出
- `plc_task.h` - 任务调度
- `plc_var.h` - 变量管理
- `plc_hmi.h` - HMI 接口
- `plc_modbus.h` - Modbus 协议
- `plc_ethercat.h` - EtherCAT 协议
