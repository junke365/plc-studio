# ===========================================================================
# SDL2 HMI 平台 CMakeLists.txt
# 需要 SDL2 库
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

find_package(SDL2 REQUIRED)

add_executable(plc-hmi-sdl2
  hmi_sdl2.c
  hmi_sdl2_main.c
  ${PLC_HMI_ROOT}/generated/plc_hmi_demo.c
)

target_include_directories(plc-hmi-sdl2 PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(plc-hmi-sdl2
  PRIVATE plc-hmi
  PRIVATE ${SDL2_LIBRARIES}
  PRIVATE m
)

target_compile_definitions(plc-hmi-sdl2 PRIVATE
  PLC_USE_SDL2
  _CRT_SECURE_NO_WARNINGS
)
