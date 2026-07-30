@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM ============================================================
REM Smart PLC Runtime - STM32F407 交叉编译构建脚本 (Windows)
REM ============================================================
REM 前置条件:
REM   1. 安装 ARM GCC:   https://developer.arm.com/downloads/-/gnu-rm
REM   2. 安装 CMake:      https://cmake.org/download/
REM   3. 下载 STM32Cube_FW_F4: https://www.st.com/en/embedded-software/stm32cubef4.html
REM ============================================================

set BUILD_DIR=build-stm32
set CMAKE=cmake

REM ---- 默认路径（使用存根 HAL/FreeRTOS，无需 STM32Cube） ----
set STM32_HAL_DIR=%CD%/stm32-stubs
set FREERTOS_DIR=%CD%/stm32-stubs/Middlewares/Third_Party/FreeRTOS

REM ---- 解析参数 ----
set BUILD_TYPE=Release
set CLEAN_BUILD=0

:parse_args
if "%~1"=="" goto :done_parse
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="debug" set BUILD_TYPE=Debug
if /i "%~1"=="clean" set CLEAN_BUILD=1
if /i "%~1"=="flash" set DO_FLASH=1
shift
goto :parse_args
:done_parse

echo ============================================
echo  Smart PLC Runtime - STM32F407 构建
echo ============================================
echo  构建类型:   %BUILD_TYPE%
echo  构建目录:   %BUILD_DIR%
echo  HAL 路径:   %STM32_HAL_DIR%
echo ============================================
echo.

REM ---- 清理 ----
if %CLEAN_BUILD%==1 (
  if exist %BUILD_DIR% (
    echo 清理构建目录...
    rmdir /s /q %BUILD_DIR%
  )
)

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM ---- CMake 配置 ----
echo [1/3] CMake 配置...
%CMAKE% -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DPLATFORM=stm32 ^
  -DSTM32_HAL_DIR=%STM32_HAL_DIR% ^
  -DFREERTOS_DIR=%FREERTOS_DIR% ^
  -DBUILD_HAL=OFF ^
  -DBUILD_PROTOCOL=OFF ^
  -DBUILD_HMI=OFF ^
  -DBUILD_MOTION=ON ^
  -DBUILD_SIMULATION=OFF ^
  -DBUILD_DEVICE=OFF ^
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_gcc.cmake ^
  -S . ^
  -B %BUILD_DIR%

if %ERRORLEVEL% neq 0 (
  echo [错误] CMake 配置失败!
  exit /b 1
)

REM ---- 编译 ----
echo.
echo [2/3] 编译...
%CMAKE% --build %BUILD_DIR% -- -j%NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% neq 0 (
  echo [错误] 编译失败!
  exit /b 1
)

echo.
echo [3/3] 构建产物:
dir /b %BUILD_DIR%\plc-runtime.*

REM ---- 烧录（可选） ----
if "%DO_FLASH%"=="1" (
  echo.
  echo 烧录到 STM32F407...
  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg ^
    -c "program %BUILD_DIR%/plc-runtime.hex verify reset exit"
)

echo.
echo ============================================
echo  STM32F407 构建成功!
echo  Flash:  %BUILD_DIR%\plc-runtime.bin
echo  烧录:   build_stm32.bat flash
echo ============================================

exit /b 0
