/**
 * hmi.routes.ts - HMI 导出 API 路由
 *
 * POST /api/hmi/export/code     - 生成 C 源码
 * POST /api/hmi/export/exe      - 生成 + 编译 SDL2 EXE（Windows/Linux）
 * POST /api/hmi/export/lvgl-sim - 生成 + 编译 LVGL v9 PC 仿真 EXE
 * POST /api/hmi/export/hex      - 生成 + 编译 ESP32 HEX (预留)
 */

import type { FastifyInstance } from 'fastify'
import * as fs from 'node:fs/promises'
import { existsSync } from 'node:fs'
import * as path from 'node:path'
import { execSync } from 'node:child_process'
import { HmiGenerator } from './hmi.generator.js'

const generator = new HmiGenerator()

const PROJECT_ROOT = path.resolve(process.cwd(), '../..')
const HMI_OUTPUT_DIR = path.resolve(PROJECT_ROOT, 'hmi-output')
const RUNTIME_DIR = path.resolve(PROJECT_ROOT, 'packages/runtime')

interface HmiProject {
  forms: {
    id: string
    name: string
    width: number
    height: number
    bgColor: string
    elements: any[]
  }[]
}

/**
 * 拷贝目录（递归，跳过已存在）
 */
async function cpDir(src: string, dest: string) {
  await fs.cp(src, dest, { recursive: true, force: true })
}

export async function createHmiRoutes(fastify: FastifyInstance) {
  await fs.mkdir(HMI_OUTPUT_DIR, { recursive: true })

  /**
   * 生成 C 源码
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/code', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      return { success: true, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 + 编译 SDL2 EXE（Windows / Linux）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/exe', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)

      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `hmi_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR

      // 1. 复制核心头文件 + 源码
      await cpDir(
        path.join(runtimeDir, 'core/include'),
        path.join(outDir, 'core/include')
      )
      await cpDir(
        path.join(runtimeDir, 'core/src'),
        path.join(outDir, 'core/src')
      )

      // 2. 复制 HMI 头文件
      await cpDir(
        path.join(runtimeDir, 'hmi/include'),
        path.join(outDir, 'hmi/include')
      )

      // 3. 复制 HMI 源码（含 SDL 驱动）
      await cpDir(
        path.join(runtimeDir, 'hmi/src'),
        path.join(outDir, 'hmi/src')
      )

      // 4. 复制 SDL 平台入口
      const sdlDir = path.join(outDir, 'hmi/platforms/sdl')
      await fs.mkdir(sdlDir, { recursive: true })
      await cpDir(
        path.join(runtimeDir, 'hmi/platforms/sdl'),
        sdlDir
      )

      // 4b. 复制平台抽象层（plc_platform_* 实现，含高精度计时器）
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(
        path.join(runtimeDir, 'platform/win32'),
        path.join(outDir, 'platform/win32')
      )

      // 5. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(
        path.join(genDir, 'plc_hmi_generated.c'),
        code,
        'utf-8'
      )

      // 6. 生成 CMakeLists.txt（SDL2 版本，自动下载 SDL2 如未安装）
      await fs.writeFile(
        path.join(outDir, 'CMakeLists.txt'),
        generateSelfContainedCMake(project),
        'utf-8'
      )

      // 7. 尝试构建
      const buildDir = path.join(outDir, 'build')
      await fs.mkdir(buildDir, { recursive: true })

      const isMinGW = process.env.CC?.includes('mingw') ||
        process.env.CXX?.includes('mingw') ||
        process.env.PATH?.toLowerCase().includes('mingw') ||
        process.env.PATH?.toLowerCase().includes('mingw32')
      const generatorFlag = isMinGW ? '-G "MinGW Makefiles" ' : ''

      // 查找本地 SDL2（third_party 目录）
      const sdl2Root = findLocalSDL2(PROJECT_ROOT)

      try {
        execSync(
          `cmake ${generatorFlag}-B "${buildDir}" -DCMAKE_BUILD_TYPE=Release${sdl2Root ? ` -DSDL2_ROOT="${sdl2Root}"` : ''} && cmake --build "${buildDir}"${isMinGW ? '' : ' --config Release'}`,
          {
            cwd: outDir,
            timeout: 300000,
            stdio: 'pipe',
          }
        )
      } catch (buildError) {
        const msg = (buildError as Error).message
        return {
          success: false,
          error: '编译失败，请确保已安装 CMake 和 MinGW / GCC（首次编译会自动下载 SDL2 源码）',
          details: msg,
          code,
        }
      }

      const exePath = path.join(buildDir, 'plc-hmi.exe')
      const exists = await fs.stat(exePath).then(() => true).catch(() => false)

      return {
        success: exists,
        exePath: exists ? exePath : null,
        code,
        message: exists ? 'EXE 编译成功' : 'EXE 未生成，请检查编译输出',
      }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 + 编译 LVGL v9 PC 仿真 EXE
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/lvgl-sim', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)

      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `hmi_lvgl_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR
      const thirdPartyDir = path.join(PROJECT_ROOT, 'third_party')

      // 1. 复制核心头文件 + 源码
      await cpDir(
        path.join(runtimeDir, 'core/include'),
        path.join(outDir, 'core/include')
      )
      await cpDir(
        path.join(runtimeDir, 'core/src'),
        path.join(outDir, 'core/src')
      )

      // 2. 复制 HMI 头文件
      await cpDir(
        path.join(runtimeDir, 'hmi/include'),
        path.join(outDir, 'hmi/include')
      )

      // 3. 复制 HMI 源码（含 LVGL 驱动）
      await cpDir(
        path.join(runtimeDir, 'hmi/src'),
        path.join(outDir, 'hmi/src')
      )

      // 4. 复制 LVGL 仿真平台入口
      const lvglSimDir = path.join(outDir, 'hmi/platforms/lvgl-sim')
      await fs.mkdir(lvglSimDir, { recursive: true })
      await cpDir(
        path.join(runtimeDir, 'hmi/platforms/lvgl-sim'),
        lvglSimDir
      )

      // 5. 复制 LVGL v9 源码
      const lvglSrc = path.join(thirdPartyDir, 'lvgl')
      if (!existsSync(lvglSrc)) {
        return {
          success: false,
          error: '未找到 LVGL v9 源码，请确保 third_party/lvgl/ 存在',
          code,
        }
      }
      await cpDir(lvglSrc, path.join(outDir, 'lvgl'))

      // 5b. 复制平台抽象层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(
        path.join(runtimeDir, 'platform/win32'),
        path.join(outDir, 'platform/win32')
      )

      // 6. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(
        path.join(genDir, 'plc_hmi_generated.c'),
        code,
        'utf-8'
      )

      // 7. 生成 CMakeLists.txt（LVGL + SDL2 版本）
      await fs.writeFile(
        path.join(outDir, 'CMakeLists.txt'),
        generateLvglSimCMake(project, thirdPartyDir),
        'utf-8'
      )

      // 8. 尝试构建
      const buildDir = path.join(outDir, 'build')
      await fs.mkdir(buildDir, { recursive: true })

      const isMinGW = process.env.CC?.includes('mingw') ||
        process.env.CXX?.includes('mingw') ||
        process.env.PATH?.toLowerCase().includes('mingw') ||
        process.env.PATH?.toLowerCase().includes('mingw32')
      const generatorFlag = isMinGW ? '-G "MinGW Makefiles" ' : ''

      const sdl2Root = findLocalSDL2(PROJECT_ROOT)
      const sdl2RootFlag = sdl2Root ? ` -DSDL2_ROOT="${sdl2Root}"` : ''

      try {
        execSync(
          `cmake ${generatorFlag}-B "${buildDir}" -DCMAKE_BUILD_TYPE=Release${sdl2RootFlag} && cmake --build "${buildDir}"${isMinGW ? '' : ' --config Release'}`,
          {
            cwd: outDir,
            timeout: 300000,
            stdio: 'pipe',
          }
        )
      } catch (buildError) {
        const msg = (buildError as Error).message
        return {
          success: false,
          error: 'LVGL 仿真编译失败，请确保 third_party/ 中有 SDL2 和 LVGL',
          details: msg,
          code,
        }
      }

      // 9. 复制 SDL2.dll 到 EXE 旁
      const sdl2Dll = findLocalSDL2Dll(PROJECT_ROOT)
      if (sdl2Dll) {
        try {
          await fs.copyFile(sdl2Dll, path.join(buildDir, 'SDL2.dll'))
        } catch { /* 非致命 */ }
      }

      const exePath = path.join(buildDir, 'plc-hmi-lvgl-sim.exe')
      const exists = await fs.stat(exePath).then(() => true).catch(() => false)

      return {
        success: exists,
        exePath: exists ? exePath : null,
        code,
        message: exists ? 'LVGL 仿真 EXE 编译成功' : 'EXE 未生成，请检查编译输出',
      }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 + 编译 ESP32 HEX (预留)
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/hex', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      return {
        success: false,
        code,
        error: 'ESP32 HEX 导出尚未实现，需要 ESP-IDF 环境',
      }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })
}

// ==================== 辅助函数 ====================

/**
 * 生成自包含的 CMakeLists.txt（SDL2 版本）
 * - 自动检测已安装的 SDL2，未找到则通过 FetchContent 下载编译
 * - 跨平台：Windows MinGW / Linux GCC
 */
/**
 * 在项目目录中查找本地 SDL2 安装
 * 返回 SDL2 根目录路径，未找到则返回 null
 */
function findLocalSDL2(projectRoot: string): string | null {
  const bases = [
    path.join(projectRoot, 'third_party', 'SDL2-2.30.10'),
    path.join(projectRoot, 'third_party'),
    path.join(projectRoot, 'SDL2-2.30.10'),
  ]
  for (const base of bases) {
    for (const archDir of [base, ...['i686-w64-mingw32', 'x86_64-w64-mingw32'].map(a => path.join(base, a))]) {
      const cmakePaths = [
        path.join(archDir, 'lib', 'cmake', 'SDL2', 'sdl2-config.cmake'),
        path.join(archDir, 'cmake', 'SDL2Config.cmake'),
        path.join(archDir, 'SDL2Config.cmake'),
      ]
      for (const cp of cmakePaths) {
        if (existsSync(cp)) {
          return archDir
        }
      }
    }
  }
  return null
}

function findLocalLVGL(projectRoot: string): string | null {
  const candidate = path.join(projectRoot, 'third_party', 'lvgl')
  if (existsSync(path.join(candidate, 'CMakeLists.txt'))) {
    return candidate
  }
  return null
}

function findLocalSDL2Dll(projectRoot: string): string | null {
  const sdls = findLocalSDL2(projectRoot)
  if (!sdls) return null
  const candidates = [
    path.join(sdls, 'bin', 'SDL2.dll'),
    path.join(sdls, '..', 'bin', 'SDL2.dll'),
  ]
  for (const c of candidates) {
    const p = path.resolve(c)
    if (existsSync(p)) return p
  }
  return null
}

/**
 * 生成 LVGL v9 PC 仿真 CMakeLists.txt
 */
function generateLvglSimCMake(project: HmiProject, thirdPartyDir: string): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-lvgl-sim VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== SDL2 依赖 ====================
if(SDL2_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "\${SDL2_ROOT}")
  message(STATUS "SDL2 搜索路径: \${SDL2_ROOT}")
endif()
find_package(SDL2 REQUIRED)

# ==================== LVGL（指定 lv_conf.h 路径）====================
set(LV_BUILD_CONF_DIR "\${CMAKE_SOURCE_DIR}/hmi/platforms/lvgl-sim" CACHE PATH "")
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
add_subdirectory(lvgl)

# LVGL SDL 驱动需要 SDL2 头文件
target_include_directories(lvgl PUBLIC \${SDL2_INCLUDE_DIRS})

# ==================== 核心库 ====================
file(GLOB PLC_CORE_SOURCES "core/src/*.c")
add_library(plc-core STATIC \${PLC_CORE_SOURCES})
target_include_directories(plc-core PUBLIC core/include)

# ==================== HMI 库 ====================
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_input.c
  hmi/src/plc_hmi_lvgl.c
)
target_include_directories(plc-hmi
  PUBLIC  hmi/include
  PRIVATE core/include
)
target_compile_definitions(plc-hmi PRIVATE PLC_USE_LVGL)

# ==================== 可执行文件 ====================
add_executable(plc-hmi-lvgl-sim
  hmi/platforms/lvgl-sim/hmi_lvgl_sim_main.c
  hmi/generated/plc_hmi_generated.c
  platform/win32/platform.c
)
target_include_directories(plc-hmi-lvgl-sim
  PRIVATE
    hmi/include
    core/include
    hmi/generated
    platform/win32
    hmi/platforms/lvgl-sim
)
target_compile_definitions(plc-hmi-lvgl-sim PRIVATE
  PLC_USE_LVGL
  LV_CONF_INCLUDE_SIMPLE
  PLC_HMI_SCREEN_WIDTH=${sw}
  PLC_HMI_SCREEN_HEIGHT=${sh}
  _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(plc-hmi-lvgl-sim
  PRIVATE
    plc-core
    plc-hmi
    lvgl
)
if(WIN32)
  target_link_libraries(plc-hmi-lvgl-sim PRIVATE SDL2::SDL2main SDL2::SDL2 winmm ws2_32 mingw32)
endif()
if(WIN32)
  set_target_properties(plc-hmi-lvgl-sim PROPERTIES SUFFIX ".exe")
endif()
`
}

function generateSelfContainedCMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-export VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== SDL2 依赖 ====================
# 查找顺序：
#   1. SDL2_ROOT 缓存变量（由构建命令传入，指向 local/third_party）
#   2. 系统已安装的 SDL2（apt/vcpkg/msys2）
#   3. FetchContent 自动下载（备用，需要网络）

# 如果 SDL2_ROOT 指定了本地路径，优先使用
if(SDL2_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "\${SDL2_ROOT}")
  message(STATUS "SDL2 搜索路径: \${SDL2_ROOT}")
endif()

find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
  message(STATUS "SDL2 未在系统中找到，正在通过 FetchContent 下载...")
  include(FetchContent)
  FetchContent_Declare(SDL2
    URL https://www.libsdl.org/release/SDL2-2.30.10.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  set(SDL_SHARED_ENABLED_BY_DEFAULT ON)
  set(SDL_STATIC_ENABLED_BY_DEFAULT OFF)
  FetchContent_MakeAvailable(SDL2)
  message(STATUS "SDL2 自动下载完成")
endif()

# ==================== 核心库 ====================
file(GLOB PLC_CORE_SOURCES "core/src/*.c")
add_library(plc-core STATIC \${PLC_CORE_SOURCES})
target_include_directories(plc-core PUBLIC core/include)
target_compile_options(plc-core PRIVATE -Wall -Wextra)

# ==================== HMI 库 ====================
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_driver_sdl.c
  hmi/src/plc_hmi_input.c
)
target_include_directories(plc-hmi
  PUBLIC  hmi/include
  PRIVATE core/include
)
target_link_libraries(plc-hmi PUBLIC SDL2::SDL2)

# ==================== 可执行文件 ====================
add_executable(plc-hmi
  hmi/platforms/sdl/hmi_sdl.c
  hmi/generated/plc_hmi_generated.c
  platform/win32/platform.c
)
target_include_directories(plc-hmi
  PRIVATE
    hmi/include
    core/include
    hmi/generated
    platform/win32
)
target_link_libraries(plc-hmi
  PRIVATE
    plc-core
    plc-hmi
    SDL2::SDL2
)
if(WIN32)
  target_link_libraries(plc-hmi PRIVATE SDL2::SDL2main winmm ws2_32)
endif()
target_compile_definitions(plc-hmi PRIVATE
  _CRT_SECURE_NO_WARNINGS
  PLC_HMI_SCREEN_WIDTH=${sw}
  PLC_HMI_SCREEN_HEIGHT=${sh}
)
set_target_properties(plc-hmi PROPERTIES
  OUTPUT_NAME "plc-hmi"
)
if(WIN32)
  set_target_properties(plc-hmi PROPERTIES SUFFIX ".exe")
endif()
`
}
