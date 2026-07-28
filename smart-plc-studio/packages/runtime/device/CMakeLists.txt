cmake_minimum_required(VERSION 3.10)

# ============================================================
# plc-device: 分布式 PLC 设备拓扑
# ============================================================
# 管理多 PLC 设备发现、连接、通信、变量映射

if(NOT TARGET plc-core)
  message(FATAL_ERROR "plc-device requires plc-core")
endif()

set(DEVICE_SRC
  src/plc_device.c
)

add_library(plc-device STATIC ${DEVICE_SRC})

target_include_directories(plc-device PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(plc-device PUBLIC plc-core)

target_compile_options(plc-device PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# 测试
option(BUILD_DEVICE_TESTS "Build device tests" ON)
if(BUILD_DEVICE_TESTS)
  add_executable(test-rte-topology tests/test_rte_topology.c)
  target_link_libraries(test-rte-topology PRIVATE plc-device)
  target_include_directories(test-rte-topology PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
endif()
