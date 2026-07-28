cmake_minimum_required(VERSION 3.10)
project(plc-hal C)

# 收集所有 HAL 源文件
file(GLOB HAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# 创建静态库
add_library(plc-hal STATIC ${HAL_SOURCES})

# 包含头文件目录 (PUBLIC: 对依赖此库的目标可见)
target_include_directories(plc-hal
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接核心库
target_link_libraries(plc-hal
  PUBLIC
    plc-core
)

# C99 标准
target_compile_features(plc-hal PRIVATE c_std_99)
