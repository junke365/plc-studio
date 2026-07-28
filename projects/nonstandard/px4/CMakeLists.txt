cmake_minimum_required(VERSION 3.10)

# PX4 Autopilot 轻量级 C 集成
# 依赖: plc-core (平台抽象)

if(NOT TARGET plc-core)
  find_library(PLC_CORE_LIB plc-core PATHS ${CMAKE_BINARY_DIR})
  if(NOT PLC_CORE_LIB)
    message(STATUS "PX4: plc-core not found, building standalone")
  endif()
endif()

set(PX4_SRC
  src/plc_px4.c
)

add_library(plc-px4 STATIC ${PX4_SRC})

target_include_directories(plc-px4 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(plc-px4 PRIVATE
  -Wall -Wextra -Wno-unused-parameter
)

if(TARGET plc-core)
  target_link_libraries(plc-px4 PRIVATE plc-core)
endif()

if(TARGET plc-simulation)
  target_link_libraries(plc-px4 PRIVATE plc-simulation)
  target_compile_definitions(plc-px4 PRIVATE HAVE_SIMULATION)
endif()

message(STATUS "  [PX4] 构建 PX4 飞控集成库")
