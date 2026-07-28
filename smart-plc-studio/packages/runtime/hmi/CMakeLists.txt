# ===========================================================================
# Smart PLC HMI - 顶层 CMakeLists.txt
#
# 静态库 plc-hmi + 可选平台目标
#
# 选项：
#   BUILD_HMI_WIN32     - Win32 GDI 后端 (Windows)
#   BUILD_HMI_LINUX_FB  - Linux帧缓冲后端
#   BUILD_HMI_LVGL      - LVGL集成后端
#   BUILD_HMI_SDL2      - SDL2后端 (模拟器)
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

set(PLC_HMI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PLC_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    CACHE PATH "核心库头文件路径")

# ========== 编译选项 ==========

option(BUILD_HMI_WIN32    "构建Win32 GDI后端"    OFF)
option(BUILD_HMI_LINUX_FB "构建Linux帧缓冲后端"  OFF)
option(BUILD_HMI_LVGL     "构建LVGL集成后端"      OFF)
option(BUILD_HMI_SDL2     "构建SDL2后端"          OFF)

# ========== 静态库 plc-hmi ==========

add_library(plc-hmi STATIC
  src/plc_hmi.c
  src/plc_hmi_widget.c
  src/plc_hmi_driver.c
  src/plc_hmi_input.c
)

target_include_directories(plc-hmi
  PUBLIC  ${PLC_HMI_ROOT}/include
  PRIVATE ${PLC_CORE_INCLUDE_DIR}
)

target_compile_features(plc-hmi PUBLIC c_std_99)

# ========== 平台子目录 ==========

if(BUILD_HMI_WIN32 AND WIN32)
  add_subdirectory(platforms/windows)
endif()

if(BUILD_HMI_LINUX_FB AND UNIX)
  add_subdirectory(platforms/linux-fb)
endif()

if(BUILD_HMI_LVGL)
  add_subdirectory(platforms/lvgl)
endif()

if(BUILD_HMI_SDL2)
  add_subdirectory(platforms/sdl2)
endif()

# ========== 安装 ==========

install(TARGETS plc-hmi
  ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
  DESTINATION include/plc_hmi
  FILES_MATCHING PATTERN "*.h"
)
