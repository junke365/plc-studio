# =============================================================================
# ESP32 平台库 — PLC 运行时平台适配
#
# 依赖:
#   - Xtensa ESP32 GCC 工具链 (xtensa-esp32-elf-gcc)
#   - ESP-IDF 头文件桩 (stubs) 位于 build-esp32/stubs/include/

cmake_minimum_required(VERSION 3.12)

# 头文件桩路径（所有 ESP-IDF / FreeRTOS / lwIP 头文件均在此）
set(ESP32_STUBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../build-esp32/stubs")

message(STATUS "ESP32 stubs: ${ESP32_STUBS_DIR}")

# =============================================================================
# PLC 平台库
# =============================================================================

add_library(plc-platform STATIC
  platform.c
  ${ESP32_STUBS_DIR}/src/stubs_impl.c
)

target_include_directories(plc-platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ESP32_STUBS_DIR}/include
)

target_compile_definitions(plc-platform PRIVATE
  PLATFORM_ESP32
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
  ../../examples/esp32/main.c
  ${GENERATED_SOURCES}
)

target_link_libraries(plc-runtime
  PRIVATE plc-core
  PRIVATE plc-platform
)

target_include_directories(plc-runtime PRIVATE
  ${GENERATED_DIR}
  ${ESP32_STUBS_DIR}/include
)

target_compile_definitions(plc-runtime PRIVATE
  PLATFORM_ESP32
)

# 可选模块链接
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

# 生成 .bin 和 .hex
find_program(CMAKE_OBJCOPY xtensa-esp32-elf-objcopy
  PATHS "C:/Espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin"
  NO_DEFAULT_PATH
)
find_program(CMAKE_SIZE xtensa-esp32-elf-size
  PATHS "C:/Espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin"
  NO_DEFAULT_PATH
)

if(CMAKE_OBJCOPY)
  add_custom_command(TARGET plc-runtime POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:plc-runtime> $<TARGET_FILE_DIR:plc-runtime>/plc-runtime.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:plc-runtime> $<TARGET_FILE_DIR:plc-runtime>/plc-runtime.hex
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:plc-runtime>
    COMMENT "生成 plc-runtime.bin / plc-runtime.hex"
  )
endif()

message(STATUS "PLC ESP32 运行时已配置")
