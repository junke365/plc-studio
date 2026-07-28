# ===========================================================================
# LiteX FPGA HMI 平台 CMakeLists.txt
#
# 构建 LiteX FPGA 平台上的 HMI 帧缓冲驱动
# 需要 RISC-V 交叉编译器: riscv32-unknown-elf-gcc
# 需要 LiteX 生成的 CSR 头文件（csr.h, mem.h）
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU")
  message(WARNING "LiteX 平台推荐使用 RISC-V GCC: riscv32-unknown-elf-gcc")
endif()

add_library(plc-hmi-litex STATIC
  hmi_litex.c
)

target_include_directories(plc-hmi-litex PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
)

target_link_libraries(plc-hmi-litex
  PRIVATE plc-hmi
)

target_compile_definitions(plc-hmi-litex PRIVATE
  PLC_USE_LITEX
)

# 允许外部指定帧缓冲基址
if(DEFINED PLC_LITEX_FB_BASE)
  target_compile_definitions(plc-hmi-litex PRIVATE
    PLC_LITEX_FB_BASE=${PLC_LITEX_FB_BASE}
  )
endif()
