# POWERLINK 手写协议栈构建脚本
# 核心静态库 + 自检测试
#
# Win32 构建示例:
#   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -S . -B build
#   cmake --build build && build/test_plk_core.exe

cmake_minimum_required(VERSION 3.10)
project(plk LANGUAGES C)

set(PLK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ========== 协议核心静态库 ==========

add_library(plk_core STATIC
  src/plk_core.c
  src/nmt.c
  src/od.c
  src/sdo.c
  src/dll.c
  src/cnm.c
)
target_include_directories(plk_core PUBLIC ${PLK_INCLUDE_DIR})
set_target_properties(plk_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========== 以太网驱动 ==========
# 按平台选择驱动实现：
#   - Windows: edrv_winpcap.c（需 WinPcap/Npcap SDK）
#   - Linux:   edrv_linux.c（AF_PACKET 原始套接字, ARM/x86）
# 注册表 plk_edrv.c 在所有平台均编译。

set(WPDK_PACK_DIR
  "G:/cnc/openPOWERLINK_V2.7.2/contrib/pcap/windows/WpdPack"
  CACHE PATH "WinPcap/Npcap SDK 目录（含 Include/Lib）")

set(PLK_EDRV_SOURCES src/edrv/plk_edrv.c)
set(PLK_EDRV_PUBLIC_INCLUDE ${PLK_INCLUDE_DIR})
set(PLK_EDRV_LIBS plk_core)

if(WIN32)
  if(EXISTS "${WPDK_PACK_DIR}/Include/pcap.h")
    list(APPEND PLK_EDRV_SOURCES src/edrv/edrv_winpcap.c)
    list(APPEND PLK_EDRV_PUBLIC_INCLUDE "${WPDK_PACK_DIR}/Include")
    list(APPEND PLK_EDRV_LIBS
      "${WPDK_PACK_DIR}/Lib/libwpcap.a"
      "${WPDK_PACK_DIR}/Lib/libpacket.a"
      ws2_32 iphlpapi)
  endif()
elseif(UNIX)
  list(APPEND PLK_EDRV_SOURCES src/edrv/edrv_linux.c)
  list(APPEND PLK_EDRV_LIBS pthread)
endif()

add_library(plk_edrv STATIC ${PLK_EDRV_SOURCES})
target_include_directories(plk_edrv PUBLIC ${PLK_EDRV_PUBLIC_INCLUDE})
target_link_libraries(plk_edrv PUBLIC ${PLK_EDRV_LIBS})
set_target_properties(plk_edrv PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========== 测试 ==========

enable_testing()

add_executable(test_plk_core tests/test_plk_core.c)
target_link_libraries(test_plk_core plk_core)
add_test(NAME test_plk_core COMMAND test_plk_core)

add_executable(test_plk_proto tests/test_plk_proto.c tests/edrv_loopback.c)
target_link_libraries(test_plk_proto plk_core plk_edrv)
add_test(NAME test_plk_proto COMMAND test_plk_proto)
