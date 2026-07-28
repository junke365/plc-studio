# Smart PLC HMI 运行时

## 架构概览

```
hmi/
├── include/              # 头文件
│   ├── plc_hmi.h         # 主接口（已存在）
│   ├── plc_hmi_driver.h  # 显示驱动接口
│   ├── plc_hmi_widget.h  # 控件系统
│   ├── plc_hmi_input.h   # 输入抽象层
│   └── plc_hmi_font.h    # 位图字体
├── src/                  # 核心实现
│   ├── plc_hmi.c         # HMI引擎：帧缓冲、FPS、更新循环、屏幕导航
│   ├── plc_hmi_widget.c  # 控件：创建/销毁/渲染/命中测试/属性/绑定
│   ├── plc_hmi_driver.c  # 显示驱动注册和管理
│   └── plc_hmi_input.c   # 输入事件环形缓冲区
├── platforms/            # 平台驱动
│   ├── windows/          # Win32 GDI (WM_PAINT + DIB Section)
│   ├── linux-fb/         # Linux /dev/fb0 + 触摸输入
│   ├── lvgl/             # LVGL集成 (控件映射 + 工业暗色主题)
│   └── sdl2/             # SDL2模拟器 (含变量模拟 + FPS覆盖层)
├── generated/
│   └── plc_hmi_demo.c   # 演示界面：电机控制、仪表盘、趋势图
└── CMakeLists.txt        # 顶层构建
```

## 支持的控件类型

| 控件          | 类型     | 描述                               |
| ------------- | -------- | ---------------------------------- |
| LABEL         | 文本标签 | 可缩放文字                         |
| BUTTON        | 按钮     | 带点击回调，支持正常/按下/禁用状态 |
| SWITCH        | 开关     | 二态切换                           |
| SLIDER        | 滑块     | 水平/垂直，连续值                  |
| GAUGE         | 仪表盘   | 半圆仪表，带指针和红区             |
| VALUE_DISPLAY | 数值显示 | 格式化数值 + 单位                  |
| BAR           | 柱状条   | 水平/垂直填充                      |
| PROGRESS_BAR  | 进度条   | 百分比进度                         |
| TREND_CHART   | 趋势图   | 64点环形历史数据                   |
| RECTANGLE     | 矩形     | 填充/边框                          |
| CIRCLE        | 圆形     | 填充/边框                          |
| GROUP         | 组容器   | 子控件分组                         |

## PLC变量绑定

```c
// 变量索引 1 → 滑块值属性，双向绑定
plc_hmi_binding_add(slider_id, "value", "1", PLC_HMI_BIND_BIDIR);

// 变量索引 2 → 标签文本，只读
plc_hmi_binding_add(label_id, "text", "2", PLC_HMI_BIND_READ);
```

变量表为 `int32_t` 数组，变量名使用数字索引字符串（"0", "1", ...）。

## 构建示例

```bash
# SDL2 模拟器
cmake -B build -DBUILD_HMI_SDL2=ON
cmake --build build --target plc-hmi-sdl2

# Win32
cmake -B build -DBUILD_HMI_WIN32=ON
cmake --build build --target plc-hmi-win32

# Linux 帧缓冲
cmake -B build -DBUILD_HMI_LINUX_FB=ON
cmake --build build --target plc-hmi-fb
```
