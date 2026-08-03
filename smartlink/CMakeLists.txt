# 智能总线 手写协议栈构建脚本
# 核心静态库 + 自检测试
#
# Win32 构建示例:
#   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -S . -B build
#   cmake --build build && build/test_sl_core.exe

cmake_minimum_required(VERSION 3.10)
project(smartlink LANGUAGES C)

set(SL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ========== 协议核心静态库 ==========

add_library(sl_core STATIC
  src/sl_core.c
  src/nmt.c
  src/od.c
  src/sdo.c
  src/dll.c
  src/cnm.c
  src/mn.c
)
target_include_directories(sl_core PUBLIC ${SL_INCLUDE_DIR})
set_target_properties(sl_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========== 以太网驱动 ==========
# 按平台选择驱动实现：
#   - Windows: edrv_winpcap.c（需 WinPcap/Npcap SDK）
#   - Linux:   edrv_linux.c（AF_PACKET 原始套接字, ARM/x86）
# 注册表 sl_edrv.c 在所有平台均编译。

set(WPDK_PACK_DIR
  "G:/cnc/WpdPack"
  CACHE PATH "WinPcap/Npcap SDK 目录（含 Include/Lib）")

set(SL_EDRV_SOURCES src/edrv/sl_edrv.c)
set(SL_EDRV_PUBLIC_INCLUDE ${SL_INCLUDE_DIR})
set(SL_EDRV_LIBS sl_core)

if(WIN32)
  if(EXISTS "${WPDK_PACK_DIR}/Include/pcap.h")
    list(APPEND SL_EDRV_SOURCES src/edrv/edrv_winpcap.c)
    list(APPEND SL_EDRV_PUBLIC_INCLUDE "${WPDK_PACK_DIR}/Include")
    list(APPEND SL_EDRV_LIBS
      "${WPDK_PACK_DIR}/Lib/libwpcap.a"
      "${WPDK_PACK_DIR}/Lib/libpacket.a"
      ws2_32 iphlpapi)
  endif()
elseif(UNIX)
  list(APPEND SL_EDRV_SOURCES src/edrv/edrv_linux.c)
  list(APPEND SL_EDRV_LIBS pthread)
endif()

add_library(sl_edrv STATIC ${SL_EDRV_SOURCES})
target_include_directories(sl_edrv PUBLIC ${SL_EDRV_PUBLIC_INCLUDE})
target_link_libraries(sl_edrv PUBLIC ${SL_EDRV_LIBS})
set_target_properties(sl_edrv PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========== 测试 ==========

enable_testing()

add_executable(test_sl_core tests/test_sl_core.c)
target_link_libraries(test_sl_core sl_core)
add_test(NAME test_sl_core COMMAND test_sl_core)

add_executable(test_sl_proto tests/test_sl_proto.c tests/edrv_loopback.c)
target_link_libraries(test_sl_proto sl_core sl_edrv)
add_test(NAME test_sl_proto COMMAND test_sl_proto)

add_executable(test_sl_mn tests/test_sl_mn.c tests/edrv_loopback.c)
target_link_libraries(test_sl_mn sl_core sl_edrv)
add_test(NAME test_sl_mn COMMAND test_sl_mn)
