# ===========================================================================
# PLC 协议库 CMakeLists.txt
#
# 编译 src/ 下的所有源文件为静态库 plc-protocol
# 头文件目录: include/
# 依赖: plc-hal, plc-core
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# 收集所有源文件
file(GLOB PLC_PROTOCOL_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 如果 src/ 为空，创建占位源文件
if(NOT PLC_PROTOCOL_SOURCES)
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/plc_protocol_placeholder.c"
    "/* 占位文件 - 等待协议模块实现 */\n"
  )
  set(PLC_PROTOCOL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plc_protocol_placeholder.c"
  )
  message(STATUS "protocol/src/ 为空，已创建占位文件")
endif()

# 创建静态库
add_library(plc-protocol STATIC ${PLC_PROTOCOL_SOURCES})

# 头文件搜索路径（PUBLIC：消费者也会继承）
target_include_directories(plc-protocol
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/plc_runtime/protocol>
)

# 依赖 plc-hal 和 plc-core
if(TARGET plc-hal)
  target_link_libraries(plc-protocol PUBLIC plc-hal)
endif()

if(TARGET plc-core)
  target_link_libraries(plc-protocol PUBLIC plc-core)
endif()

# 编译选项
target_compile_options(plc-protocol PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# C99 标准
target_compile_features(plc-protocol PUBLIC c_std_99)

# 库信息
message(STATUS "协议库: plc-protocol")
message(STATUS "  源文件: ${PLC_PROTOCOL_SOURCES}")
message(STATUS "  头文件: ${CMAKE_CURRENT_SOURCE_DIR}/include/")
