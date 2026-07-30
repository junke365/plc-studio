#!/bin/bash
# ============================================================
# Smart PLC Runtime - STM32F407 交叉编译构建脚本 (Linux)
# ============================================================
# 前置条件:
#   1. 安装 ARM GCC:   sudo apt install gcc-arm-none-eabi
#   2. 安装 CMake:      sudo apt install cmake
#   3. 下载 STM32Cube_FW_F4: https://www.st.com/en/embedded-software/stm32cubef4.html
# ============================================================

BUILD_DIR="build-stm32"
CMAKE="cmake"

# 默认路径（按实际安装位置修改）
STM32_HAL_DIR="/opt/STM32Cube_FW_F4_V1.28.0"

# 解析参数
BUILD_TYPE="Release"
CLEAN_BUILD=0
DO_FLASH=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    release) BUILD_TYPE="Release" ;;
    debug)   BUILD_TYPE="Debug" ;;
    clean)   CLEAN_BUILD=1 ;;
    flash)   DO_FLASH=1 ;;
    *)       echo "用法: $0 [release|debug] [clean] [flash]"; exit 1 ;;
  esac
  shift
done

echo "============================================"
echo " Smart PLC Runtime - STM32F407 构建"
echo "============================================"
echo " 构建类型:   $BUILD_TYPE"
echo " 构建目录:   $BUILD_DIR"
echo " HAL 路径:   $STM32_HAL_DIR"
echo "============================================"
echo ""

# 清理
if [[ $CLEAN_BUILD -eq 1 ]]; then
  if [[ -d "$BUILD_DIR" ]]; then
    echo "清理构建目录..."
    rm -rf "$BUILD_DIR"
  fi
fi

mkdir -p "$BUILD_DIR"

# CMake 配置
echo "[1/3] CMake 配置..."
$CMAKE -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DPLATFORM=stm32 \
  -DSTM32_HAL_DIR="$STM32_HAL_DIR" \
  -DBUILD_HAL=OFF \
  -DBUILD_PROTOCOL=OFF \
  -DBUILD_HMI=OFF \
  -DBUILD_MOTION=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_gcc.cmake \
  -S . \
  -B "$BUILD_DIR"

if [[ $? -ne 0 ]]; then
  echo "[错误] CMake 配置失败!"
  exit 1
fi

# 编译
echo ""
echo "[2/3] 编译..."
$CMAKE --build "$BUILD_DIR" -- -j$(nproc)

if [[ $? -ne 0 ]]; then
  echo "[错误] 编译失败!"
  exit 1
fi

echo ""
echo "[3/3] 构建产物:"
ls -lh "$BUILD_DIR"/plc-runtime.*

# 烧录
if [[ $DO_FLASH -eq 1 ]]; then
  echo ""
  echo "烧录到 STM32F407..."
  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "program ${BUILD_DIR}/plc-runtime.hex verify reset exit"
fi

echo ""
echo "============================================"
echo " STM32F407 构建成功!"
echo " Flash:  $BUILD_DIR/plc-runtime.bin"
echo " 烧录:   build_stm32.sh flash"
echo "============================================"
