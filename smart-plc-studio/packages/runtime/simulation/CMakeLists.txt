cmake_minimum_required(VERSION 3.10)

# ============================================================
# plc-simulation: 多物理仿真子系统
# ============================================================
# 依赖: plc-core (平台抽象)
# 可选: plc-motion (仿真电机驱动)

if(NOT TARGET plc-core)
  message(FATAL_ERROR "plc-simulation requires plc-core")
endif()

# --- 选项 ---
option(BUILD_SIMULATION_EXAMPLE "Build simulation example" OFF)

# --- 源文件 ---
set(SIMULATION_SRC
  src/plc_physics.c
  src/plc_multibody.c
  src/plc_surgical.c
  src/plc_softbody.c
  src/plc_simulation.c
)

# --- 仿真库 ---
add_library(plc-simulation STATIC
  ${SIMULATION_SRC}
)

target_include_directories(plc-simulation PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(plc-simulation PUBLIC
  plc-core
  m
)

# --- 可选链接: 运动控制 (仿真电机驱动需要) ---
if(TARGET plc-motion)
  target_link_libraries(plc-simulation PRIVATE plc-motion)
  target_compile_definitions(plc-simulation PRIVATE PLC_SIM_HAVE_MOTION)
endif()

# --- 编译选项 ---
target_compile_options(plc-simulation PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# --- 示例 ---
if(BUILD_SIMULATION_EXAMPLE)
  add_executable(surgical-sim
    examples/surgical_sim.c
  )
  target_link_libraries(surgical-sim PRIVATE plc-simulation)
  target_include_directories(surgical-sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  if(TARGET plc-motion)
    target_link_libraries(surgical-sim PRIVATE plc-motion)
  endif()
endif()

# --- 测试 ---
option(BUILD_SIMULATION_TESTS "Build simulation unit tests" OFF)

if(BUILD_SIMULATION_TESTS)
  enable_testing()

  add_executable(test-simulation-core
    tests/test_simulation_core.c
    ${SIMULATION_SRC}
  )
  target_include_directories(test-simulation-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(test-simulation-core PRIVATE m)
  target_compile_options(test-simulation-core PRIVATE
    -Wall -Wextra -Wno-unused-parameter
  )

  add_test(NAME simulation-core COMMAND test-simulation-core)

  # --- 手术机器人 IK 测试 ---
  add_executable(test-surgical-ik
    tests/test_surgical_ik.c
    src/plc_surgical.c
    src/plc_multibody.c
    src/plc_physics.c
  )
  target_include_directories(test-surgical-ik PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(test-surgical-ik PRIVATE m)
  target_compile_options(test-surgical-ik PRIVATE
    -Wall -Wextra -Wno-unused-parameter
  )
  add_test(NAME surgical-ik COMMAND test-surgical-ik)

  # --- PX4 动力学测试 (需要 px4 库) ---
  if(TARGET px4)
    add_executable(test-px4-dynamics
      tests/test_px4_dynamics.c
    )
    target_link_libraries(test-px4-dynamics PRIVATE px4)
    target_include_directories(test-px4-dynamics PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/projects/nonstandard/px4/include
    )
    target_compile_options(test-px4-dynamics PRIVATE
      -Wall -Wextra -Wno-unused-parameter
    )
    add_test(NAME px4-dynamics COMMAND test-px4-dynamics)
  endif()
endif()
