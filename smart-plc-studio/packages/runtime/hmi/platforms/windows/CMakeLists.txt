# ===========================================================================
# Win32 HMI 平台 CMakeLists.txt
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# Win32 GDI 可执行文件
add_executable(plc-hmi-win32 WIN32
  hmi_win32.c
  ${PLC_HMI_ROOT}/generated/plc_hmi_demo.c
)

target_include_directories(plc-hmi-win32 PRIVATE
  ${PLC_HMI_ROOT}/include
  ${PLC_CORE_INCLUDE_DIR}
)

target_link_libraries(plc-hmi-win32
  PRIVATE plc-hmi
  PRIVATE user32
  PRIVATE gdi32
)

target_compile_definitions(plc-hmi-win32 PRIVATE
  _WIN32
  WIN32
  _CRT_SECURE_NO_WARNINGS
)

set_target_properties(plc-hmi-win32 PROPERTIES
  OUTPUT_NAME "plc-hmi-win32"
  SUFFIX ".exe"
)
