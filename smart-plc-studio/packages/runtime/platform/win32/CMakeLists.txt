# ===========================================================================
# Windows 平台库
# ===========================================================================

cmake_minimum_required(VERSION 3.12)

# Windows 平台适配库
add_library(plc-platform STATIC
  platform.c
)

target_include_directories(plc-platform
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
)

target_compile_definitions(plc-platform PRIVATE
  PLATFORM_WIN32
)

# Windows 链接库
target_link_libraries(plc-platform PRIVATE
  winmm
  ws2_32
)

# 启用大页支持 (Huge Pages 可减少 TLBMiss)
option(WIN32_USE_LARGE_PAGES "启用大页内存支持" OFF)
if(WIN32_USE_LARGE_PAGES)
  target_compile_definitions(plc-platform PRIVATE
    PLC_WIN32_USE_LARGE_PAGES
  )
endif()
