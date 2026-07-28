cmake_minimum_required(VERSION 3.10)

# SOFA 仿真桥接
# 连接 PLC 运行时到完整的 SOFA 框架

set(SOFA_BRIDGE_SRC
  src/plc_sofa_bridge.c
)

add_library(plc-sofa-bridge STATIC ${SOFA_BRIDGE_SRC})

target_include_directories(plc-sofa-bridge PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(plc-sofa-bridge PRIVATE
  -Wall -Wextra -Wno-unused-parameter
)

if(TARGET plc-core)
  target_link_libraries(plc-sofa-bridge PRIVATE plc-core)
endif()

if(TARGET plc-simulation)
  target_link_libraries(plc-sofa-bridge PRIVATE plc-simulation)
  target_compile_definitions(plc-sofa-bridge PRIVATE HAVE_SIMULATION)
endif()

message(STATUS "  [SOFA] 构建 SOFA 仿真桥接库")
