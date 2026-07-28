# Smart PLC Runtime

Smart PLC Runtime 是 Smart PLC Studio 的运行时引擎，负责在目标平台上执行 PLC 程序。

## 架构概述

```
┌─────────────────────────────────────────┐
│           PLC 应用程序 (IEC 61131-3)      │
├─────────────────────────────────────────┤
│           生成代码 (generated/)           │
│  generated_init() + generated_main()     │
├─────────────────────────────────────────┤
│           PLC 运行时核心 (core/)          │
│  ┌──────────┬──────────┬──────────┐     │
│  │ 变量管理  │ 任务调度  │ I/O 管理 │     │
│  │ plc_var  │ plc_task │ plc_io   │     │
│  └──────────┴──────────┴──────────┘     │
│  ┌──────────┬──────────┐                │
│  │ 通信管理  │ 调试接口  │                │
│  │ plc_comm │ plc_debug│                │
│  └──────────┴──────────┘                │
├─────────────────────────────────────────┤
│           平台抽象层 (plc_platform.h)     │
│  时间函数 | 内存管理 | 临界区 | 日志输出   │
├─────────────────────────────────────────┤
│           平台实现 (platform/)            │
│  linux-arm | linux-x86 | xenomai4        │
│  stm32 | esp32                           │
└─────────────────────────────────────────┘
```

## 目录结构

```
runtime/
├── core/                  # 运行时核心库
│   ├── include/           # 公共头文件
│   │   ├── plc_platform.h # 平台抽象层
│   │   ├── plc_var.h      # 变量管理
│   │   ├── plc_io.h       # I/O 管理
│   │   ├── plc_task.h     # 任务调度
│   │   ├── plc_comm.h     # 通信管理
│   │   ├── plc_debug.h    # 调试接口
│   │   └── plc_runtime.h  # 运行时主入口
│   └── src/               # 核心实现
├── platform/              # 平台特定实现
│   ├── linux-arm/         # ARM Linux
│   ├── linux-x86/         # x86 Linux
│   ├── xenomai4/          # Xenomai 4 实时
│   ├── stm32/             # STM32 + FreeRTOS
│   └── esp32/             # ESP32 + ESP-IDF
├── examples/              # 平台示例程序
│   ├── linux-arm/main.c
│   ├── linux-x86/main.c
│   ├── xenomai4/main.c
│   ├── stm32/main.c
│   └── esp32/main.c
├── generated/             # 编译器生成的代码（输出目录）
├── CMakeLists.txt         # 顶层构建文件
└── README.md              # 本文件
```

## 支持的平台

| 平台      | 说明                              | 编译方式       |
| --------- | --------------------------------- | -------------- |
| linux-arm | ARM Linux (树莓派、嵌入式 ARM 等) | 本地或交叉编译 |
| linux-x86 | x86 Linux (PC、工控机等)          | 本地编译       |
| xenomai4  | Xenomai 4 实时 Linux              | 需 Xenomai SDK |
| stm32     | STM32 + FreeRTOS (裸机)           | ARM 交叉编译   |
| esp32     | ESP32 + FreeRTOS (ESP-IDF)        | ESP-IDF 工具链 |

## 快速开始

### x86 Linux (默认)

```bash
mkdir build && cd build
cmake -DPLATFORM=linux-x86 ..
make
./plc-runtime
```

### ARM Linux 交叉编译

```bash
mkdir build && cd build
cmake -DPLATFORM=linux-arm \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      ..
make
```

### Xenomai 4

```bash
mkdir build && cd build
cmake -DPLATFORM=xenomai4 ..
make
# 注意：需要 root 权限和 Xenomai 内核模块
sudo ./plc-runtime
```

## 核心 API

### 运行时生命周期

```c
PlcRuntime rt;
plc_runtime_init(&rt);          /* 初始化 */
plc_runtime_load(&rt);          /* 加载生成代码 */
plc_runtime_start(&rt);         /* 启动扫描 */
plc_runtime_scan(&rt);          /* 执行扫描周期 */
plc_runtime_stop(&rt);          /* 停止扫描 */
```

### 变量管理

```c
PlcVarTable* vt = plc_runtime_get_var_table(&rt);
plc_var_register(vt, "my_var", VAR_TYPE_INT, VAR_ATTR_GLOBAL, sizeof(plc_int), "注释");
plc_var_read(vt, "my_var", &value, sizeof(value));
plc_var_write(vt, "my_var", &new_value, sizeof(new_value));
```

### I/O 配置

```c
PlcIoConfig* io = plc_runtime_get_io_config(&rt);
plc_io_register(io, IO_TYPE_DI, "DI_0", "my_bool_var", 0x00);
plc_io_bind(io, 0, vt);
```

### 任务调度

```c
PlcTaskScheduler* sched = plc_runtime_get_scheduler(&rt);
plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC, 10, 200, callback, ctx);
plc_task_schedule(sched);
```

## 构建要求

- CMake >= 3.12
- C11 编译器 (GCC, Clang, 或目标平台工具链)
- Linux 平台：pthreads, librt
- Xenomai 平台：Xenomai 4 SDK
- STM32 平台：ARM GCC 工具链, STM32 HAL
- ESP32 平台：ESP-IDF v5.x+
