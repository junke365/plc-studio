# ===========================================================================
# Linux x86 平台库
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# x86 Linux 平台适配库
add_library(plc-platform STATIC
  platform.c
)

target_include_directories(plc-platform
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
)

target_compile_definitions(plc-platform PRIVATE
  PLATFORM_LINUX_X86
)
