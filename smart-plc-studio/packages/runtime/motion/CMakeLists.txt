cmake_minimum_required(VERSION 3.10)

# ============================================================
# plc-motion: CNC 运动控制子系统
# ============================================================
# 依赖: plc-core (用于 plattform 抽象和错误码)
# 可选依赖: plc-hal, plc-protocol (EtherCAT/CANopen/Modbus 驱动后端)

if(NOT TARGET plc-core)
  message(FATAL_ERROR "plc-motion requires plc-core")
endif()

# --- 选项 ---
option(BUILD_MOTION_EXAMPLE "Build CNC motion example" OFF)

# --- 源文件 ---
set(MOTION_SRC
  src/plc_motorhal.c
  src/plc_axis.c
  src/plc_group.c
  src/plc_gcodeparser.c
  src/plc_planner.c
  src/plc_interpolator.c
  src/plc_kinematics.c
  src/plc_motion.c
)

set(MOTION_MOTORHAL_SRC
  motorhal/plc_motor_stepdir.c
)

# --- 运动学库 ---
add_library(plc-motion STATIC
  ${MOTION_SRC}
  ${MOTION_MOTORHAL_SRC}
)

target_include_directories(plc-motion PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(plc-motion PUBLIC
  plc-core
  m  # math library
)

# --- 可选链接: 协议栈 ---
if(TARGET plc-protocol)
  target_link_libraries(plc-motion PRIVATE plc-protocol)
  target_compile_definitions(plc-motion PRIVATE PLC_MOTION_HAVE_PROTOCOL)
endif()

if(TARGET plc-hal)
  target_link_libraries(plc-motion PRIVATE plc-hal)
  target_compile_definitions(plc-motion PRIVATE PLC_MOTION_HAVE_HAL)
endif()

# --- 编译选项 ---
target_compile_options(plc-motion PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# --- 示例 ---
if(BUILD_MOTION_EXAMPLE)
  add_executable(cnc-example
    examples/cnc_main.c
  )
  target_link_libraries(cnc-example PRIVATE plc-motion)
  target_include_directories(cnc-example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
endif()

# --- 测试 ---
option(BUILD_MOTION_TESTS "Build motion unit tests" OFF)

if(BUILD_MOTION_TESTS)
  enable_testing()

  add_executable(test-motion-core
    tests/test_motion_core.c
    ${MOTION_SRC}
    ${MOTION_MOTORHAL_SRC}
  )
  target_include_directories(test-motion-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(test-motion-core PRIVATE m)
  target_compile_options(test-motion-core PRIVATE
    -Wall -Wextra -Wno-unused-parameter
  )

  add_test(NAME motion-core COMMAND test-motion-core)
endif()
