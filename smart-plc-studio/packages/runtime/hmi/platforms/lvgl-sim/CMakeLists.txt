# LVGL v9 PC 仿真平台
# 构建可在 Windows 上运行的 LVGL 仿真 EXE

cmake_minimum_required(VERSION 3.14)

find_package(SDL2 REQUIRED)

add_executable(plc-hmi-lvgl-sim
  hmi_lvgl_sim_main.c
  ${PLC_HMI_ROOT}/generated/plc_hmi_generated.c
)

target_include_directories(plc-hmi-lvgl-sim PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(plc-hmi-lvgl-sim
  PRIVATE
    plc-hmi
    lvgl::lvgl
)

if(WIN32)
  target_link_libraries(plc-hmi-lvgl-sim PRIVATE SDL2::SDL2main SDL2::SDL2 winmm ws2_32 mingw32)
endif()

target_compile_definitions(plc-hmi-lvgl-sim PRIVATE
  PLC_USE_LVGL
  LV_CONF_INCLUDE_SIMPLE
  _CRT_SECURE_NO_WARNINGS
)
