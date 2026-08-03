# =============================================================================
# STM32F407 平台库 — PLC 运行时平台适配
#
# 依赖:
#   - ARM GCC 工具链 (arm-none-eabi-gcc)
#   - STM32F4xx HAL 驱动 (通过 STM32_HAL_DIR 指定)
#   - FreeRTOS 源码  (通过 FREERTOS_DIR 指定)
# =============================================================================

cmake_minimum_required(VERSION 3.12)

# 顶层 project 仅启用 C 语言，此处补充汇编语言支持 (startup_stm32f407xx.s)
enable_language(ASM)

# =============================================================================
# 外部依赖路径
# =============================================================================

# STM32F4xx HAL 驱动路径
if(NOT DEFINED STM32_HAL_DIR)
  set(STM32_HAL_DIR "C:/Users/junjun/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3"
    CACHE PATH "STM32F4xx HAL 驱动根目录")
endif()

# FreeRTOS 源码路径
if(NOT DEFINED FREERTOS_DIR)
  set(FREERTOS_DIR "C:/Users/junjun/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Middlewares/Third_Party/FreeRTOS"
    CACHE PATH "FreeRTOS 源码根目录")
endif()

message(STATUS "STM32 HAL: ${STM32_HAL_DIR}")
message(STATUS "FreeRTOS:  ${FREERTOS_DIR}")

# =============================================================================
# HAL 源文件
# =============================================================================

set(HAL_DRIVER_DIR "${STM32_HAL_DIR}/Drivers/STM32F4xx_HAL_Driver")
set(HAL_SRC_DIR    "${HAL_DRIVER_DIR}/Src")
set(CMSIS_DIR      "${STM32_HAL_DIR}/Drivers/CMSIS")

file(GLOB HAL_SOURCES
  "${HAL_SRC_DIR}/stm32f4xx_hal.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_rcc.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_gpio.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_cortex.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_uart.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_adc.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_adc_ex.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_dac.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_dac_ex.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_spi.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_tim.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_tim_ex.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_flash.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_pwr.c"
  "${HAL_SRC_DIR}/stm32f4xx_hal_dma.c"
)

# =============================================================================
# FreeRTOS 源文件
# =============================================================================

set(FREERTOS_SRC_DIR "${FREERTOS_DIR}/Source")
set(FREERTOS_PORT_DIR "${FREERTOS_SRC_DIR}/portable/GCC/ARM_CM4F")

file(GLOB FREERTOS_SOURCES
  "${FREERTOS_SRC_DIR}/*.c"
  "${FREERTOS_SRC_DIR}/portable/MemMang/heap_4.c"
  "${FREERTOS_PORT_DIR}/*.c"
)

# 无 FreeRTOS 源码时使用协作式调度器存根
if(NOT FREERTOS_SOURCES)
  file(GLOB FREERTOS_SOURCES "${FREERTOS_SRC_DIR}/tasks_stub.c")
endif()

# =============================================================================
# PLC 平台库
# =============================================================================

# 启动必需文件 (startup/syscall/system/中断) 不能放在静态库中:
# 静态库成员仅在被引用时提取, 而 Reset_Handler/中断向量表无外部引用,
# 会导致固件缺失启动代码。这些文件直接编译进 plc-runtime 可执行目标。
set(PLATFORM_STARTUP_SOURCES
  syscall_stubs.c
  system_stm32f4xx.c
  stm32f4xx_it.c
  startup_stm32f407xx.s
)

add_library(plc-platform STATIC
  # PLC 平台适配
  platform.c
)

target_include_directories(plc-platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAL_DRIVER_DIR}/Inc
    ${HAL_DRIVER_DIR}/Inc/Legacy
    ${CMSIS_DIR}/Include
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Include
    ${FREERTOS_SRC_DIR}/include
    ${FREERTOS_PORT_DIR}
)

target_compile_definitions(plc-platform PRIVATE
  PLATFORM_STM32
  STM32F407xx
  USE_HAL_DRIVER
  HSE_VALUE=8000000
)

# =============================================================================
# 链接脚本
# =============================================================================

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTx_FLASH.ld")
target_link_options(plc-platform INTERFACE
  -T ${LINKER_SCRIPT}
)

# =============================================================================
# 编译器生成的 PLC 代码
# =============================================================================

set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../generated")

file(GLOB GENERATED_SOURCES
  "${GENERATED_DIR}/*.c"
)

# =============================================================================
# 主可执行文件
# =============================================================================

add_executable(plc-runtime
  ../../examples/stm32/main.c
  ${GENERATED_SOURCES}
)

target_link_libraries(plc-runtime
  PRIVATE plc-core
  PRIVATE plc-platform
)

target_include_directories(plc-runtime PRIVATE
  ${GENERATED_DIR}
)

# 链接 HAL 和 FreeRTOS 源文件到 plc-runtime
target_sources(plc-runtime PRIVATE
  ${PLATFORM_STARTUP_SOURCES}
  ${HAL_SOURCES}
  ${FREERTOS_SOURCES}
)

target_include_directories(plc-runtime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HAL_DRIVER_DIR}/Inc
  ${HAL_DRIVER_DIR}/Inc/Legacy
  ${CMSIS_DIR}/Include
  ${CMSIS_DIR}/Device/ST/STM32F4xx/Include
  ${FREERTOS_SRC_DIR}/include
  ${FREERTOS_PORT_DIR}
)

target_compile_definitions(plc-runtime PRIVATE
  PLATFORM_STM32
  STM32F407xx
  USE_HAL_DRIVER
  HSE_VALUE=8000000
)

# 可选模块
if(BUILD_HAL)
  target_sources(plc-runtime PRIVATE
    ${HAL_SOURCES}
  )
  target_link_libraries(plc-runtime PRIVATE plc-hal)
endif()

if(BUILD_PROTOCOL)
  target_link_libraries(plc-runtime PRIVATE plc-protocol)
endif()

if(BUILD_MOTION)
  target_link_libraries(plc-runtime PRIVATE plc-motion)
  target_compile_definitions(plc-runtime PRIVATE BUILD_MOTION=1)
endif()

# 生成 .bin 和 .hex（用于烧录）
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
find_program(CMAKE_SIZE arm-none-eabi-size)

if(CMAKE_OBJCOPY)
  add_custom_command(TARGET plc-runtime POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:plc-runtime> $<TARGET_FILE_DIR:plc-runtime>/plc-runtime.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:plc-runtime> $<TARGET_FILE_DIR:plc-runtime>/plc-runtime.hex
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:plc-runtime>
    COMMENT "生成 plc-runtime.bin / plc-runtime.hex"
  )
endif()

message(STATUS "PLC STM32F407 运行时已配置")
message(STATUS "  链接脚本: ${LINKER_SCRIPT}")
message(STATUS "  Flash: 1MB  RAM: 192KB (128+64)")
