# ===========================================================================
# Smart PLC Runtime - 顶层 CMakeLists.txt
#
# 支持平台：
#   - linux-arm    (ARM Linux)
#   - linux-x86    (x86 Linux, 默认)
#   - xenomai4     (Xenomai 4 实时)
#   - stm32        (STM32 + FreeRTOS, 需交叉编译)
#   - esp32        (ESP32 + ESP-IDF, 需 ESP-IDF 工具链)
# ===========================================================================

cmake_minimum_required(VERSION 3.12)
project(smart-plc-runtime VERSION 1.0.0 LANGUAGES C)

# C11 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ===========================================================================
# 平台选择
# ===========================================================================

# 自动检测 Windows 平台
if(WIN32 OR MINGW OR MSVC)
  set(PLATFORM "win32" CACHE STRING "目标平台")
else()
  set(PLATFORM "linux-x86" CACHE STRING "目标平台")
endif()
set_property(CACHE PLATFORM PROPERTY STRINGS
  linux-arm linux-x86 xenomai4 stm32 esp32 litex win32)

message(STATUS "目标平台: ${PLATFORM}")

# ===========================================================================
# 模块开关
# ===========================================================================

option(BUILD_HAL          "构建硬件抽象层 (HAL)"          ON)
option(BUILD_PROTOCOL     "构建工业通信协议"              ON)
option(BUILD_HMI          "构建 HMI 运行时"               ON)
option(BUILD_HMI_WIN32    "HMI Win32 平台 (.exe)"         OFF)
option(BUILD_HMI_LINUX_FB "HMI Linux Framebuffer 平台"    OFF)
option(BUILD_HMI_LVGL     "HMI LVGL 嵌入式平台"           OFF)
option(BUILD_HMI_SDL2     "HMI SDL2 模拟器平台"           OFF)

# 根据目标平台自动启用对应 HMI
if(PLATFORM STREQUAL "linux-x86" OR PLATFORM STREQUAL "linux-arm")
  set(BUILD_HMI_LINUX_FB ON CACHE BOOL "" FORCE)
  set(BUILD_HMI_SDL2 ON CACHE BOOL "" FORCE)
endif()
if(PLATFORM STREQUAL "litex")
  set(BUILD_HMI_LITEX ON CACHE BOOL "" FORCE)
endif()

# ===========================================================================
# 通用头文件路径
# ===========================================================================

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/hal/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/protocol/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/hmi/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/motion/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/simulation/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/device/include)

# ===========================================================================
# 核心库
# ===========================================================================

add_subdirectory(core)

# ===========================================================================
# 硬件抽象层
# ===========================================================================

if(BUILD_HAL)
  add_subdirectory(hal)
endif()

# ===========================================================================
# 协议库
# ===========================================================================

if(BUILD_PROTOCOL)
  add_subdirectory(protocol)
endif()

# ===========================================================================
# HMI 运行时
# ===========================================================================

if(BUILD_HMI)
  # Windows 平台默认启用 Win32 HMI
  if(WIN32 OR PLATFORM STREQUAL "win32")
    set(BUILD_HMI_WIN32 ON CACHE BOOL "" FORCE)
  endif()
  add_subdirectory(hmi)
endif()

# ===========================================================================
# 运动控制 (CNC)
# ===========================================================================

option(BUILD_MOTION "构建 CNC 运动控制模块" ON)
option(BUILD_MOTION_EXAMPLE "构建 CNC 示例" OFF)

if(BUILD_MOTION)
  add_subdirectory(motion)
endif()

# ===========================================================================
# 多物理仿真 (手术机器人)
# ===========================================================================

option(BUILD_SIMULATION "构建多物理仿真模块" ON)
option(BUILD_SIMULATION_EXAMPLE "构建仿真示例" OFF)

if(BUILD_SIMULATION)
  add_subdirectory(simulation)
endif()

# ===========================================================================
# 设备拓扑 (分布式 PLC)
# ===========================================================================

option(BUILD_DEVICE "构建分布式设备拓扑模块" ON)

if(BUILD_DEVICE)
  add_subdirectory(device)
endif()

# ===========================================================================
# 平台相关配置
# ===========================================================================
# 每个平台创建 plc-runtime 可执行文件:
# - 链接 plc-core 和 plc-platform
# - 可选: plc-hal, plc-protocol, plc-hmi, plc-motion

if(PLATFORM STREQUAL "linux-arm" OR PLATFORM STREQUAL "linux-x86")

  # Linux 平台
  find_package(Threads REQUIRED)

  add_subdirectory(platform/${PLATFORM})

  add_executable(plc-runtime
    examples/${PLATFORM}/main.c
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
    PRIVATE Threads::Threads
    PRIVATE rt
  )

  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

elseif(PLATFORM STREQUAL "xenomai4")

  # Xenomai 4 实时平台
  add_subdirectory(platform/xenomai4)

  add_executable(plc-runtime
    examples/xenomai4/main.c
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
    PRIVATE rtdm
    PRIVATE cobalt
    PRIVATE pthread
  )

  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

elseif(PLATFORM STREQUAL "stm32")

  # STM32 + FreeRTOS（交叉编译）
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32/CMakeLists.txt)
    add_subdirectory(platform/stm32)
  else()
    add_library(plc-platform STATIC IMPORTED)
    message(WARNING "platform/stm32/ 尚无 CMakeLists.txt，跳过平台库")
  endif()

  add_executable(plc-runtime
    examples/stm32/main.c
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
  )

  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

elseif(PLATFORM STREQUAL "litex")

  # LiteX FPGA (RISC-V SoC)
  add_subdirectory(platform/litex)

  add_executable(plc-runtime
    examples/litex/main.c
  )

  target_include_directories(plc-runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/litex
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
  )

  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

elseif(PLATFORM STREQUAL "esp32")

  # ESP32 + ESP-IDF
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/esp32/CMakeLists.txt)
    add_subdirectory(platform/esp32)
  else()
    add_library(plc-platform STATIC IMPORTED)
    message(WARNING "platform/esp32/ 尚无 CMakeLists.txt，跳过平台库")
  endif()

  add_executable(plc-runtime
    examples/esp32/main.c
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
  )

  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

elseif(PLATFORM STREQUAL "win32")

  # Windows 实时平台
  message(STATUS "Win32 编译:")

  add_subdirectory(platform/win32)

  add_executable(plc-runtime
    examples/win32/main.c
  )

  target_link_libraries(plc-runtime
    PRIVATE plc-core
    PRIVATE plc-platform
  )

  # Win32 平台头文件 (plc_win_rt.h)
  target_include_directories(plc-runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/win32
  )

  # Win32 特定链接
  target_link_libraries(plc-runtime PRIVATE winmm ws2_32)

  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()
  if(BUILD_MOTION)
    target_link_libraries(plc-runtime PRIVATE plc-motion)
    target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
  endif()

  if(BUILD_SIMULATION)
    target_link_libraries(plc-runtime PRIVATE plc-simulation)
    target_compile_definitions(plc-runtime PRIVATE BUILD_SIMULATION=1)
  endif()

  if(BUILD_DEVICE)
    target_link_libraries(plc-runtime PRIVATE plc-device)
    target_compile_definitions(plc-runtime PRIVATE BUILD_DEVICE=1)
  endif()

  # 控制台子系统 (仅 MSVC)
  if(MSVC)
    set_target_properties(plc-runtime PROPERTIES
      LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
  endif()

else()

  message(FATAL_ERROR "未知平台: ${PLATFORM}")
  message(STATUS "支持的平台: linux-arm, linux-x86, xenomai4, stm32, esp32, litex, win32")

endif()

# ===========================================================================
# 安装目标
# ===========================================================================

install(TARGETS plc-runtime
  RUNTIME DESTINATION bin
)

install(DIRECTORY core/include/
  DESTINATION include/plc_runtime
  FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY hal/include/
  DESTINATION include/plc_hal
  FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY protocol/include/
  DESTINATION include/plc_protocol
  FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY hmi/include/
  DESTINATION include/plc_hmi
  FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY motion/include/
  DESTINATION include/plc_motion
  FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY simulation/include/
  DESTINATION include/plc_simulation
  FILES_MATCHING PATTERN "*.h"
)

# ===========================================================================
# 构建信息
# ===========================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Smart PLC Runtime ${PROJECT_VERSION}")
message(STATUS "平台: ${PLATFORM}")
message(STATUS "模块: HAL=${BUILD_HAL}  协议=${BUILD_PROTOCOL}  HMI=${BUILD_HMI}  运动控制=${BUILD_MOTION}  仿真=${BUILD_SIMULATION}  设备=${BUILD_DEVICE}")
message(STATUS "HMI平台: Win32=${BUILD_HMI_WIN32}  Linux-FB=${BUILD_HMI_LINUX_FB}  LVGL=${BUILD_HMI_LVGL}  SDL2=${BUILD_HMI_SDL2}  LiteX=${BUILD_HMI_LITEX}")
message(STATUS "运动控制: 示例=${BUILD_MOTION_EXAMPLE}")
message(STATUS "C 标准: C${CMAKE_C_STANDARD}")
message(STATUS "========================================")
