cmake_minimum_required(VERSION 3.10)
project(plc-sofa-sim C)

# 引用运行时 simulation 库
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../packages/runtime/simulation simulation)

add_executable(sofa-sim
  src/main.c
  src/plc_sofa_sim.c
  src/plc_sofa_server.c
)

target_include_directories(sofa-sim PRIVATE
  include
  ${CMAKE_SOURCE_DIR}/../../../packages/runtime/simulation/include
  ${CMAKE_SOURCE_DIR}/../../../packages/runtime/device/include
)

target_link_libraries(sofa-sim PRIVATE plc-simulation)
