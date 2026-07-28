# ===========================================================================
# Linux Framebuffer HMI 平台 CMakeLists.txt
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

find_package(Threads REQUIRED)

add_executable(plc-hmi-fb
  hmi_fb.c
  hmi_fb_main.c
  ${PLC_HMI_ROOT}/generated/plc_hmi_demo.c
)

target_include_directories(plc-hmi-fb PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
)

target_link_libraries(plc-hmi-fb
  PRIVATE plc-hmi
  PRIVATE Threads::Threads
  PRIVATE rt
)

target_compile_definitions(plc-hmi-fb PRIVATE
  _GNU_SOURCE
  PLATFORM_LINUX
)
