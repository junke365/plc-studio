@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM ============================================================
REM Smart PLC Runtime - Windows 构建脚本 (MinGW)
REM ============================================================

set BUILD_DIR=build-mingw
set GENERATOR="MinGW Makefiles"
set CMAKE=cmake
set MAKE=mingw32-make

REM ---- 解析参数 ----
set BUILD_TYPE=Debug
set CLEAN_BUILD=0
set RUN_TESTS=0
set BUILD_MOTION=ON
set BUILD_MOTION_TESTS=OFF

:parse_args
if "%~1"=="" goto :done_parse
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="debug" set BUILD_TYPE=Debug
if /i "%~1"=="clean" set CLEAN_BUILD=1
if /i "%~1"=="test" set RUN_TESTS=1
if /i "%~1"=="test" set BUILD_MOTION_TESTS=ON
if /i "%~1"=="notest" set BUILD_MOTION_TESTS=OFF
if /i "%~1"=="nomotion" set BUILD_MOTION=OFF
shift
goto :parse_args
:done_parse

echo ============================================
echo  Smart PLC Runtime - Win32 构建
echo ============================================
echo  生成器:     %GENERATOR%
echo  构建类型:   %BUILD_TYPE%
echo  构建目录:   %BUILD_DIR%
echo  运动控制:   %BUILD_MOTION%
echo  运动测试:   %BUILD_MOTION_TESTS%
echo ============================================
echo.

REM ---- 清理 ----
if %CLEAN_BUILD%==1 (
  if exist %BUILD_DIR% (
    echo 清理构建目录...
    rmdir /s /q %BUILD_DIR%
  )
)

REM ---- 创建构建目录 ----
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM ---- CMake 配置 ----
echo [1/3] CMake 配置...
%CMAKE% -G %GENERATOR% ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DPLATFORM=win32 ^
  -DBUILD_MOTION=%BUILD_MOTION% ^
  -DBUILD_MOTION_EXAMPLE=ON ^
  -DBUILD_MOTION_TESTS=%BUILD_MOTION_TESTS% ^
  -DBUILD_HAL=OFF ^
  -DBUILD_PROTOCOL=OFF ^
  -DBUILD_HMI=OFF ^
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

REM ---- 运行测试 ----
if %RUN_TESTS%==1 (
  echo.
  echo [3/3] 运行测试...
  cd %BUILD_DIR%
  ctest --output-on-failure
  cd ..
) else (
  echo.
  echo [3/3] 跳过测试 (使用 test 参数运行测试)
)

echo.
echo ============================================
echo  构建成功!
echo  输出目录: %BUILD_DIR%
echo  运行:     %BUILD_DIR%\plc-runtime.exe
echo ============================================

exit /b 0
