# =============================================================================
# ESP32 Xtensa GCC 交叉编译工具链
# 目标: xtensa-esp32-elf (ESP32)
# =============================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

# 查找 ESP-IDF 工具链
set(ESP32_TOOLCHAIN_DIR "C:/Espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin")

find_program(CMAKE_C_COMPILER xtensa-esp32-elf-gcc
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)
find_program(CMAKE_CXX_COMPILER xtensa-esp32-elf-g++
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)
find_program(CMAKE_ASM_COMPILER xtensa-esp32-elf-gcc
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)
find_program(CMAKE_SIZE xtensa-esp32-elf-size
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)
find_program(CMAKE_OBJCOPY xtensa-esp32-elf-objcopy
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)
find_program(CMAKE_OBJDUMP xtensa-esp32-elf-objdump
  PATHS "${ESP32_TOOLCHAIN_DIR}"
  NO_DEFAULT_PATH
)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "未找到 xtensa-esp32-elf-gcc，请安装 ESP-IDF 工具链")
endif()

# CMake try_compile 阶段只编译不链接（裸机平台无标准 main）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# C 编译标志
set(CMAKE_C_FLAGS "-ffreestanding -std=c11 -Wall -Wextra -Wpedantic" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

# ASM 编译标志
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp" CACHE STRING "" FORCE)

# 链接标志
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections" CACHE STRING "" FORCE)

# 生成 .bin 和 .hex 的自定义命令
function(esp32_bin_hex target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.hex
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${target}>
    COMMENT "生成 ${target}.bin / ${target}.hex"
  )
endfunction()

# 搜索路径
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
