/**
 * hmi.routes.ts - HMI 导出 API 路由
 *
 * POST /api/hmi/export/code - 生成 C 源码
 * POST /api/hmi/export/exe  - 生成 + 编译 Windows EXE
 * POST /api/hmi/export/hex  - 生成 + 编译 ESP32 HEX (预留)
 */

import type { FastifyInstance } from 'fastify'
import * as fs from 'node:fs/promises'
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
   * 生成 + 编译 Windows EXE
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

      // 1. 复制核心头文件
      await cpDir(
        path.join(runtimeDir, 'core/include'),
        path.join(outDir, 'core/include')
      )

      // 2. 复制 HMI 头文件
      await cpDir(
        path.join(runtimeDir, 'hmi/include'),
        path.join(outDir, 'hmi/include')
      )

      // 3. 复制 HMI 源码
      await cpDir(
        path.join(runtimeDir, 'hmi/src'),
        path.join(outDir, 'hmi/src')
      )

      // 4. 复制 Win32 平台源码
      const win32Dir = path.join(outDir, 'hmi/platforms/windows')
      await fs.mkdir(win32Dir, { recursive: true })
      await cpDir(
        path.join(runtimeDir, 'hmi/platforms/windows'),
        win32Dir
      )

      // 5. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(
        path.join(genDir, 'plc_hmi_generated.c'),
        code,
        'utf-8'
      )

      // 6. 生成自包含的 CMakeLists.txt
      await fs.writeFile(
        path.join(outDir, 'CMakeLists.txt'),
        generateSelfContainedCMake(project),
        'utf-8'
      )

      // 7. 生成 patched hmi_win32.c（添加屏初始化/更新调用）
      const win32Src = await fs.readFile(
        path.join(runtimeDir, 'hmi/platforms/windows/hmi_win32.c'),
        'utf-8'
      )
      const patchedWin32 = patchWin32Main(win32Src)
      await fs.writeFile(
        path.join(win32Dir, 'hmi_win32.c'),
        patchedWin32,
        'utf-8'
      )

      // 8. 尝试构建
      const buildDir = path.join(outDir, 'build')
      await fs.mkdir(buildDir, { recursive: true })

      try {
        execSync(
          `cmake -B "${buildDir}" -DCMAKE_BUILD_TYPE=Release && cmake --build "${buildDir}" --config Release`,
          {
            cwd: outDir,
            timeout: 180000,
            stdio: 'pipe',
          }
        )
      } catch (buildError) {
        const msg = (buildError as Error).message
        return {
          success: false,
          error: '编译失败，请确保已安装 CMake 和 Visual Studio / MinGW',
          details: msg,
          code,
        }
      }

      const exePath = path.join(buildDir, 'plc-hmi-win32.exe')
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
 * 生成自包含的 CMakeLists.txt
 */
function generateSelfContainedCMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.12)
project(plc-hmi-export VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译静态库 plc-hmi
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_input.c
)

target_include_directories(plc-hmi
  PUBLIC  hmi/include
  PRIVATE core/include
)

# Win32 可执行文件
add_executable(plc-hmi-win32 WIN32
  hmi/platforms/windows/hmi_win32.c
  hmi/generated/plc_hmi_generated.c
)

target_include_directories(plc-hmi-win32
  PRIVATE
    hmi/include
    core/include
    hmi/generated
)

target_link_libraries(plc-hmi-win32
  PRIVATE
    plc-hmi
    user32
    gdi32
)

target_compile_definitions(plc-hmi-win32 PRIVATE
  _WIN32
  WIN32
  _CRT_SECURE_NO_WARNINGS
  PLC_HMI_SCREEN_WIDTH=${sw}
  PLC_HMI_SCREEN_HEIGHT=${sh}
)

set_target_properties(plc-hmi-win32 PROPERTIES
  OUTPUT_NAME "plc-hmi-win32"
  SUFFIX ".exe"
)
`
}

/**
 * 修补 hmi_win32.c 以调用生成的屏幕初始化/更新函数
 */
function patchWin32Main(src: string): string {
  // 在 includes 之后添加 extern 声明
  let patched = src.replace(
    '#include "resource.h"',
    `#include "resource.h"

/* 由 plc_hmi_generated.c 提供的函数 */
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);`
  )

  // 在 WM_TIMER 处理中添加屏幕更新调用
  patched = patched.replace(
    'case WM_TIMER: {',
    `case WM_TIMER: {
      plc_hmi_screens_update(NULL, 0);`
  )

  // 在 plc_hmi_navigate("main") 之后添加 screen init
  patched = patched.replace(
    'plc_hmi_navigate("main");',
    'plc_hmi_navigate("main");\n\n  /* 加载用户屏幕 */\n  plc_hmi_screens_init();'
  )

  return patched
}
