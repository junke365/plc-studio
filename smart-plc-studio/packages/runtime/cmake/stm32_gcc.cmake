# =============================================================================
# STM32 ARM GCC 交叉编译工具链
# 目标: arm-none-eabi (Cortex-M4F, STM32F407)
# =============================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)

# 查找 ARM GCC 工具链
find_program(CMAKE_C_COMPILER arm-none-eabi-gcc)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
find_program(CMAKE_SIZE arm-none-eabi-size)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
find_program(CMAKE_OBJDUMP arm-none-eabi-objdump)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "未找到 arm-none-eabi-gcc，请安装 ARM GCC 工具链")
endif()

# CMake try_compile 阶段只编译不链接（裸机平台无标准 main）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 目标架构标志
set(MCU_FLAGS
  -mcpu=cortex-m4
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
)

# 将 MCU flags 列表转为字符串（去分号）
string(REGEX REPLACE ";" " " MCU_FLAGS_STR "${MCU_FLAGS}")

# C 编译标志
set(CMAKE_C_FLAGS "${MCU_FLAGS_STR} -std=c11 -Wall -Wextra -Wpedantic" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

# ASM 编译标志
set(CMAKE_ASM_FLAGS "${MCU_FLAGS_STR} -x assembler-with-cpp" CACHE STRING "" FORCE)

# 链接标志
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS_STR} -Wl,--gc-sections" CACHE STRING "" FORCE)

# 生成 .bin 和 .hex 的自定义命令
function(stm32_bin_hex target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.hex
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${target}>
    COMMENT "生成 ${target}.bin / ${target}.hex"
  )
endfunction()

# 搜索路径（系统库和 CMSIS）
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
