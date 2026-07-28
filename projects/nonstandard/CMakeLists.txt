cmake_minimum_required(VERSION 3.10)

# ============================================================
# 非标准项目 (Non-standard Projects)
# ============================================================
# 这些项目不属于 PLCopen IEC 61131-3 标准,
# 但可以与 PLC 运行时集成使用。
#
# 包含:
#   - PX4: 无人机飞控集成
#   - OpenCV5: 计算机视觉
#   - SOFA: 物理仿真框架桥接
# ============================================================

option(BUILD_NONSTANDARD_PX4     "Build PX4 Autopilot integration"    OFF)
option(BUILD_NONSTANDARD_OPENCV5 "Build OpenCV5 vision module"       OFF)
option(BUILD_NONSTANDARD_SOFA    "Build SOFA simulation bridge"      OFF)

# --- 通用头文件 ---
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/px4/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/opencv5/include)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/sofa/include)

# --- PX4 ---
if(BUILD_NONSTANDARD_PX4)
  add_subdirectory(px4)
endif()

# --- OpenCV5 ---
if(BUILD_NONSTANDARD_OPENCV5)
  add_subdirectory(opencv5)
endif()

# --- SOFA ---
if(BUILD_NONSTANDARD_SOFA)
  add_subdirectory(sofa)
endif()
