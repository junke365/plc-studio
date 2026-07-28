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

set(PLATFORM "linux-x86" CACHE STRING "目标平台")
set_property(CACHE PLATFORM PROPERTY STRINGS
  linux-arm linux-x86 xenomai4 stm32 esp32)

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

# ===========================================================================
# 通用头文件路径
# ===========================================================================

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/hal/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/protocol/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/hmi/include)

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
# 平台相关配置
# ===========================================================================

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

  # 链接可选模块
  if(BUILD_HAL)
    target_link_libraries(plc-runtime PRIVATE plc-hal)
  endif()
  if(BUILD_PROTOCOL)
    target_link_libraries(plc-runtime PRIVATE plc-protocol)
  endif()
  if(BUILD_HMI)
    target_link_libraries(plc-runtime PRIVATE plc-hmi)
  endif()

  # ARM 交叉编译提示
  if(PLATFORM STREQUAL "linux-arm")
    message(STATUS "ARM Linux 编译:")
    message(STATUS "  如果是交叉编译，请设置:")
    message(STATUS "  -DCMAKE_TOOLCHAIN_FILE=arm-linux-gnueabihf.cmake")
    message(STATUS "  或使用 -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc")
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

  # Xenomai pkg-config
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(XENOMAI QUIET libcobalt)
    if(XENOMAI_FOUND)
      target_include_directories(plc-runtime PRIVATE ${XENOMAI_INCLUDE_DIRS})
      target_link_libraries(plc-runtime PRIVATE ${XENOMAI_LIBRARIES})
    endif()
  endif()

  message(STATUS "Xenomai 4 编译:")
  message(STATUS "  需要 Xenomai 4 SDK 和 Cobalt 内核模块")
  message(STATUS "  链接库: -lrtdm -lcobalt -pthread")

elseif(PLATFORM STREQUAL "stm32")

  # STM32 + FreeRTOS（交叉编译）
  message(STATUS "STM32 交叉编译:")
  message(STATUS "  此平台需要 ARM 工具链和 STM32 HAL")
  message(STATUS "  推荐使用 STM32CubeMX 生成项目，然后集成此运行时")
  message(STATUS "")
  message(STATUS "  CMake 交叉编译示例:")
  message(STATUS "    cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake \\")
  message(STATUS "          -DSTM32F4xx=ON ..")
  message(STATUS "")
  message(STATUS "  ARM 工具链标志:")
  message(STATUS "    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
  message(STATUS("    -specs=nosys.specs -specs=nano.specs")
  message(STATUS "")

  # STM32 需要外部提供 platform 库和 HAL
  # 此处仅提供编译框架
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32/CMakeLists.txt)
    add_subdirectory(platform/stm32)
  else()
    # 占位库，等待实际平台实现
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

  # STM32 特定编译选项
  target_compile_options(plc-runtime PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -ffunction-sections
    -fdata-sections
  )

  target_link_options(plc-runtime PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -specs=nosys.specs
    -specs=nano.specs
    -Wl,--gc-sections
  )

elseif(PLATFORM STREQUAL "esp32")

  # ESP32 + ESP-IDF
  message(STATUS "ESP32 编译:")
  message(STATUS "  此平台需要 ESP-IDF 构建系统")
  message(STATUS "  请在 ESP-IDF 环境中使用 idf.py build")
  message(STATUS "")
  message(STATUS "  集成步骤:")
  message(STATUS "  1. 在 ESP-IDF 项目中添加本目录为组件")
  message(STATUS "  2. 在 CMakeLists.txt 中 set(EXTRA_COMPONENT_DIRS ...)")
  message(STATUS "  3. 使用 idf.py build 编译")

  # ESP-IDF 环境检测
  if(DEFINED ENV{IDF_PATH})
    message(STATUS "  IDF_PATH: $ENV{IDF_PATH}")
  else()
    message(WARNING "未检测到 IDF_PATH，ESP-IDF 环境可能未配置")
  endif()

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

else()

  message(FATAL_ERROR "未知平台: ${PLATFORM}")
  message(STATUS "支持的平台: linux-arm, linux-x86, xenomai4, stm32, esp32")

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

# ===========================================================================
# 构建信息
# ===========================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Smart PLC Runtime ${PROJECT_VERSION}")
message(STATUS "平台: ${PLATFORM}")
message(STATUS "模块: HAL=${BUILD_HAL}  协议=${BUILD_PROTOCOL}  HMI=${BUILD_HMI}")
message(STATUS "HMI平台: Win32=${BUILD_HMI_WIN32}  Linux-FB=${BUILD_HMI_LINUX_FB}  LVGL=${BUILD_HMI_LVGL}  SDL2=${BUILD_HMI_SDL2}")
message(STATUS "C 标准: C${CMAKE_C_STANDARD}")
message(STATUS "========================================")
