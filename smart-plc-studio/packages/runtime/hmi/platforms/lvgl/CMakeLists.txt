# ===========================================================================
# LVGL HMI 平台 CMakeLists.txt
# 需要已有的 LVGL 目标（通常由上层项目提供）
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

add_library(plc-hmi-lvgl STATIC
  hmi_lvgl.c
  hmi_lvgl_styles.c
)

target_include_directories(plc-hmi-lvgl PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
)

# LVGL头文件路径 - 根据实际项目调整
# target_include_directories(plc-hmi-lvgl PRIVATE /path/to/lvgl)

target_link_libraries(plc-hmi-lvgl
  PRIVATE plc-hmi
)

target_compile_definitions(plc-hmi-lvgl PRIVATE
  PLC_USE_LVGL
)
