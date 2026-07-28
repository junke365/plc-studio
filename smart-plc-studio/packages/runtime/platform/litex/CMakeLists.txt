# ===========================================================================
# LiteX FPGA 平台库
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# LiteX 平台适配库
add_library(plc-platform STATIC
  platform.c
)

target_include_directories(plc-platform
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
)

target_compile_definitions(plc-platform PRIVATE
  PLATFORM_LITEX
)

target_compile_options(plc-platform PRIVATE
  -march=rv32im
  -mabi=ilp32
  -nostdlib
  -ffreestanding
)

# 允许外部 LiteX CSR 头文件路径
if(DEFINED LITEX_CSR_DIR)
  target_include_directories(plc-platform PRIVATE ${LITEX_CSR_DIR})
endif()
