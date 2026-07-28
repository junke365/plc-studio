cmake_minimum_required(VERSION 3.10)
project(hybrid-vcnc C)

set(CMAKE_C_STANDARD 11)

option(BUILD_HYBRID_EXAMPLE "Build hybrid VCNC example" ON)

if(BUILD_HYBRID_EXAMPLE)
  # 要求运行时路径
  set(RUNTIME_DIR "${CMAKE_SOURCE_DIR}/../../smart-plc-studio/packages/runtime")

  add_library(hybrid-vision STATIC vision_pipeline.c)
  target_include_directories(hybrid-vision PRIVATE
    ${RUNTIME_DIR}/core/include
    ${RUNTIME_DIR}/simulation/include
    ${RUNTIME_DIR}/device/include
  )

  add_library(hybrid-drone STATIC drone_mixer.c)
  target_include_directories(hybrid-drone PRIVATE
    ${RUNTIME_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/nonstandard/px4/include
  )

  add_library(hybrid-sim STATIC sim_bridge.c)
  target_include_directories(hybrid-sim PRIVATE
    ${RUNTIME_DIR}/core/include
    ${RUNTIME_DIR}/simulation/include
  )

  add_executable(hybrid-vcnc main.c)
  target_link_libraries(hybrid-vcnc
    hybrid-vision
    hybrid-drone
    hybrid-sim
  )
  target_include_directories(hybrid-vcnc PRIVATE .)
endif()
