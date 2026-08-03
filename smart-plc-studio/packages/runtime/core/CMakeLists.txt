# ===========================================================================
# PLC 核心库 CMakeLists.txt
#
# 编译 core/src/ 下的所有源文件为静态库 plc-core
# 头文件目录: core/include/
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# 收集所有源文件
file(GLOB PLC_CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 如果 src/ 为空，创建占位源文件
if(NOT PLC_CORE_SOURCES)
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/plc_core_placeholder.c"
    "/* 占位文件 - 等待核心模块实现 */\n"
  )
  set(PLC_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plc_core_placeholder.c"
  )
  message(STATUS "core/src/ 为空，已创建占位文件")
endif()

# 创建静态库
add_library(plc-core STATIC ${PLC_CORE_SOURCES})

# 头文件搜索路径（PUBLIC：消费者也会继承）
target_include_directories(plc-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/plc_runtime>
)

# 编译选项
target_compile_options(plc-core PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# STM32 交叉编译：传递平台宏（紧凑内存配置等依赖 PLATFORM_STM32）
if(PLATFORM STREQUAL "stm32")
  target_compile_definitions(plc-core PUBLIC
    PLATFORM_STM32
    STM32F407xx
  )
endif()

# 库信息
message(STATUS "核心库: plc-core")
message(STATUS "  源文件: ${PLC_CORE_SOURCES}")
message(STATUS "  头文件: ${CMAKE_CURRENT_SOURCE_DIR}/include/")
