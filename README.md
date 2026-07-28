# PLC Studio

一个基于 IEC 61131-3 标准的智能 PLC 集成开发环境，支持多种编程语言、HMI 设计和运动控制仿真。

![PLC Studio 主界面](gui-reference/gui/plc_studio_10/screen.png)

## 项目简介

PLC Studio 是一个现代化的工业自动化开发平台，旨在为工程师提供一个统一的环境来开发、调试和部署 PLC 程序。项目采用前后端分离架构，前端基于 Vue 3 + TypeScript，后端使用 Fastify，运行时核心使用 C 语言编写，支持多种嵌入式平台。

## 核心功能

### IEC 61131-3 编程语言支持

- **梯形图 (LD)** - 继电器逻辑编程
- **功能块图 (FBD)** - 图形化功能块编程
- **结构化文本 (ST)** - 高级文本编程语言
- **指令表 (IL)** - 汇编风格编程
- **顺序功能图 (SFC)** - 顺序控制编程

![编程语言编辑器](gui-reference/gui/plc_studio_5/screen.png)

### HMI 人机界面设计

- 可视化 HMI 编辑器
- 丰富的控件库
- 实时数据绑定
- 多平台渲染支持 (LVGL、SDL2、Linux FB、Windows)

![HMI 设计器](gui-reference/gui/plc_studio_svg_hmi_1/screen.png)

### CNC 运动控制

- 多轴运动控制 (最多 9 轴)
- G-code 解析与执行
- 实时刀具路径仿真
- 运动学模型支持 (关节型、笛卡尔型、SCARA、并联型)

![CNC 控制中心](gui-reference/gui/plc_studio_cnc_1/screen.png)

### 调试与仿真

- 实时变量监控
- 断点调试支持
- 虚拟仿真环境
- 3D 运动可视化

### 串口调试工具

- 串口参数配置（波特率、数据位、停止位、校验位）
- ASCII/HEX 双模式收发
- 串口日志记录与导出
- 波形可视化分析
- 支持 DTR/RTS 信号控制

### 网络调试工具

- **TCP 客户端/服务端** - 连接管理、数据收发、多客户端支持
- **UDP 客户端/服务端** - 数据报收发、绑定管理
- **WebSocket 调试** - 连接测试、消息收发
- **HTTP 测试器** - REST API 调试工具
- **Modbus TCP/RTU** - 工业协议调试

![调试面板](gui-reference/gui/plc_studio_9/screen.png)

### 多物理仿真引擎

- **刚体动力学** - 完整物理世界（重力、碰撞、约束求解、迭代求解器）
- **软体仿真** - 质量-弹簧系统、FEM 线弹性/共旋转模型、碰撞检测（软体-刚体/软体-软体/软体-平面）
- **多体动力学** - RNEA 逆动力学、ABA 正动力学、质量矩阵、科里奥利/重力、几何雅可比
- **手术机器人运动学** - dVRK MTM (7-DOF) / PSM (7-DOF) 正逆运动学、阻尼最小二乘 IK、奇异性检测、工作空间分析
- **CNC 设备仿真** - 13 种机型 G 代码解析与执行
- **工厂场景仿真** - 18 种工业设备三维仿真

### 设备拓扑编辑器

- 分布式 PLC 设备画布拖拽连线
- 支持 PLC / 手术机器人 / 视觉相机 / PX4 无人机 / CNC 机床
- SVG 贝塞尔曲线连线、连接手柄拖拽
- JSON 拓扑导入/导出、localStorage 持久化

### 手术机器人仿真器

- Three.js 3D 实时渲染
- WebSocket 连接 C 运行时后端
- 手动/IK/自动三种控制模式
- 关节滑块控制、TCP 位置显示
- 奇异性与工作空间监测

### 项目分类架构

- **标准项目** - IEC 61131-3 标准 PLC 编程
- **非标准项目** - 扩展集成（PX4 飞控 / OpenCV 视觉 / SOFA 仿真）
- **混合项目** - 标准与非标准混合开发

## 项目架构

```
smart-plc-studio/
├── electron/              # Electron 桌面应用
│   ├── serial/            # 串口管理 (IPC)
│   ├── network/           # TCP/UDP/WebSocket 管理
│   └── main.ts            # Electron 主进程
├── packages/
│   ├── editor/            # Vue 3 前端编辑器
│   │   └── src/
│   │       ├── components/
│   │       │   ├── topology/     # 设备拓扑画布编辑器
│   │       │   ├── simulator/    # 仿真器组件 (CNC/World/Surgical)
│   │       │   ├── tools/        # 调试工具组件
│   │       │   │   ├── uart/     # 串口调试
│   │       │   │   ├── network/  # TCP/UDP/WS 调试
│   │       │   │   ├── modbus/   # Modbus 调试
│   │       │   │   └── motor/    # 电机调试
│   │       │   └── project/      # 项目树管理器
│   │       ├── views/           # 路由页面 (Simulator, SurgicalSim, TopologyEditor, Welcome)
│   │       ├── layouts/         # 布局组件 (ActivityBar, TopNavBar, StatusBar)
│   │       └── stores/          # Pinia 状态管理 (project, editor, ui, debug)
│   ├── server/            # Fastify 后端服务
│   │   └── src/modules/
│   │       ├── tools/     # 调试工具后端模块
│   │       └── hmi/       # HMI 服务模块
│   ├── shared/            # 共享类型定义
│   │   └── src/types/     # project, topology, editor, plc, protocol
│   └── runtime/           # C 运行时 (支持多平台)
│       ├── core/          # 核心运行时
│       ├── hal/           # 硬件抽象层
│       ├── hmi/           # HMI 渲染引擎
│       ├── device/        # 分布式设备拓扑与通信
│       ├── motion/        # CNC 运动控制
│       ├── simulation/    # 多物理仿真引擎
│       │   ├── include/   # 物理/多体/手术机器人/软体/仿真系统
│       │   ├── src/       # 实现 (physics, multibody, surgical, softbody, simulation)
│       │   └── tests/     # 单元测试 (surgical IK, PX4 dynamics)
│       ├── protocol/      # 通信协议 (Modbus, EtherCAT, CANopen)
│       └── platform/      # 平台适配 (ESP32, STM32, Linux, Xenomai, Win32, LiteX)
├── projects/              # 项目分类模板与示例
│   ├── templates/         # 标准项目模板 (ST/LD/FBD)
│   ├── hybrid/            # 混合项目示例
│   └── nonstandard/       # 非标准项目
│       ├── px4/           # PX4 飞控集成
│       ├── opencv5/       # OpenCV5 视觉集成
│       └── sofa/          # SOFA 独立手术仿真器
```

## 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | Vue 3, TypeScript, Vite, Pinia, Naive UI |
| 图形 | Fabric.js (2D), Three.js (3D) |
| 代码编辑 | Monaco Editor |
| 后端 | Fastify, @fastify/websocket, Socket.IO, SQLite |
| 桌面 | Electron, IPC |
| 串口 | serialport |
| 运行时 | C11 (C99+), CMake, MinGW / GCC / Clang |
| 仿真 | 刚体动力学、软体 (FEM/弹簧-质点)、多体 (RNEA/ABA)、DH 运动学 |
| 通信 | Modbus, EtherCAT, CANopen, TCP/UDP, WebSocket |
| 外接系统 | PX4-Autopilot, OpenCV5, SOFA |

## 快速开始

### 环境要求

- Node.js >= 18
- npm >= 9
- CMake >= 3.20 (编译运行时)
- GCC/Clang (Linux) 或 MSVC (Windows)

### 安装与运行

```bash
# 克隆仓库
git clone https://github.com/junke365/plc-studio.git
cd plc-studio/smart-plc-studio

# 安装依赖
npm install

# 启动开发服务器 (前端 + 后端)
npm run dev
```

### 构建

```bash
# 构建所有包
npm run build

# 仅构建前端
npm run build:editor

# 仅构建后端
npm run build:server
```

## 运行时编译

```bash
cd smart-plc-studio/packages/runtime

# Linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 交叉编译 (ESP32)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../platform/esp32/toolchain.cmake
```

## 界面预览

| 主界面 | 梯形图编辑 | FBD 编辑 |
|--------|-----------|----------|
| ![主界面](gui-reference/gui/plc_studio_10/screen.png) | ![梯形图](gui-reference/gui/plc_studio_4/screen.png) | ![FBD](gui-reference/gui/plc_studio_5/screen.png) |

| HMI 设计 | 运动仿真 | 调试面板 |
|----------|---------|----------|
| ![HMI](gui-reference/gui/plc_studio_svg_hmi_2/screen.png) | ![仿真](gui-reference/gui/plc_studio_cnc_1/screen.png) | ![调试](gui-reference/gui/plc_studio_9/screen.png) |

## 文档

详细文档请参阅 [docs/](docs/) 目录：

- [架构设计](docs/architecture.md)
- [用户指南](docs/user-guide.md)
- [运行时开发](docs/runtime-guide.md)
- [API 参考](docs/api-reference.md)

## 开发计划

### ✅ 已完成

- [x] 项目架构搭建
- [x] 编辑器基础框架 (Vue 3 + Pinia + Vue Router)
- [x] ST/LD/FBD 编程语言编辑器
- [x] HMI 设计器（LVGL/SDL2/LinuxFB/Win32/LiteX 多平台渲染）
- [x] 串口调试工具
- [x] TCP/UDP/WebSocket 网络调试工具
- [x] Modbus TCP/RTU 调试工具
- [x] 电机调试工具（伺服、步进、变频器）
- [x] PLC 运行时核心
- [x] 多平台编译支持 (Linux x86/ARM, Xenomai4, STM32, ESP32, LiteX, Win32)
- [x] CNC 运动控制（G 代码解析、多轴插补、13 种机型）
- [x] 调试功能（变量监视、断点、单步执行）
- [x] 运动学配置编辑器
- [x] 多物理仿真引擎
  - [x] 刚体动力学与碰撞检测
  - [x] 软体仿真（质量-弹簧 / FEM）
  - [x] 多体动力学（RNEA / ABA / 雅可比）
  - [x] 手术机器人运动学（dVRK MTM/PSM，FK/IK/奇异/工作空间）
- [x] 分布式设备拓扑编辑器（画布拖拽连线）
- [x] 手术机器人 3D 仿真器（Three.js + WebSocket C 后端）
- [x] 项目分类架构（标准/非标准/混合）
- [x] PX4 飞控集成（6-DOF 动力学仿真 + uORB 订阅回调）
- [x] OpenCV5 视觉集成（C++ 包装层 + DNN/aruco/光流）
- [x] SOFA 独立仿真器（TCP 服务器 + SimulationSystem 包装）

### 🚧 进行中 / 计划中

- [ ] 梯形图编辑器增强（在线仿真调试）
- [ ] FBD 编辑器增强（用户自定义功能块库）
- [ ] 运行时 RTE 多设备分布式联调
- [ ] PX4 硬件在环 (HIL) 支持
- [ ] OpenCV5 视觉流水线编辑器
- [ ] SOFA 仿真器 GUI 面板增强
- [ ] Electron 桌面打包

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License
