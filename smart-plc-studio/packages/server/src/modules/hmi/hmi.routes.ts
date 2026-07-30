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

  /**
   * 生成 Linux LVGL 仿真项目（不编译，生成源码 + CMakeLists.txt）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/linux', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `hmi_linux_${ts}`)
      await fs.mkdir(outDir, { recursive: true })
      const runtimeDir = RUNTIME_DIR

      // 复制核心 + HMI 源码
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'core/src'))
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'hmi/src'))

      // 复制 LVGL 仿真平台入口 + lv_conf.h
      const lvglSimDir = path.join(outDir, 'hmi/platforms/lvgl-sim')
      await fs.mkdir(lvglSimDir, { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/platforms/lvgl-sim'), lvglSimDir)

      // 复制 Linux x86 平台层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'platform/linux-x86'), path.join(outDir, 'platform/linux-x86'))

      // 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 生成 Linux CMakeLists.txt
      const cmake = generateLinuxCMake(project)
      await fs.writeFile(path.join(outDir, 'CMakeLists.txt'), cmake, 'utf-8')

      return { success: true, outDir, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 STM32 LVGL 项目（不编译，生成源码 + ARM GCC 工程）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/stm32', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `hmi_stm32_${ts}`)
      await fs.mkdir(outDir, { recursive: true })
      const runtimeDir = RUNTIME_DIR

      // 复制核心 + HMI 源码
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'core/src'))
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'hmi/src'))

      // 复制嵌入式 lv_conf.h 配置
      const hmiConfigDir = path.join(outDir, 'hmi/config')
      await fs.mkdir(hmiConfigDir, { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/config'), hmiConfigDir)

      // 复制 STM32 平台层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'platform/stm32'), path.join(outDir, 'platform/stm32'))

      // 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 生成 STM32 CMakeLists.txt
      const cmake = generateStm32CMake(project)
      await fs.writeFile(path.join(outDir, 'CMakeLists.txt'), cmake, 'utf-8')

      return { success: true, outDir, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 Android LVGL 项目（不编译，生成源码 + NDK 工程结构）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/android', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `hmi_android_${ts}`)

      // 生成项目结构描述
      const projectInfo = {
        type: 'android-ndk',
        source: 'HMI 生成的 C 代码',
        ndkRequired: true,
        steps: [
          '1. 安装 Android NDK (r25+)',
          '2. 将 hmi/ 和 core/ 源码添加到 Android 项目的 jni/ 目录',
          '3. 使用 ndk-build 或 CMake 交叉编译为 arm64-v8a / armeabi-v7a',
          '4. 通过 JNI 调用 LVGL 渲染到 Android SurfaceView',
        ],
        generatedAt: new Date().toISOString(),
        outDir,
      }
      await fs.mkdir(outDir, { recursive: true })
      await fs.writeFile(path.join(outDir, 'README.md'),
        `# HMI Android 项目\n\n${projectInfo.steps.join('\n')}\n`, 'utf-8')
      await fs.writeFile(path.join(outDir, 'project.json'), JSON.stringify(projectInfo, null, 2), 'utf-8')

      return {
        success: false,
        code,
        outDir,
        error: 'Android 导出需要 NDK 环境，已生成项目骨架，请参考 README.md 手动配置',
      }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 ESP32 运行时项目（PLC 核心 + HMI LVGL + ESP-IDF 完整工程）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/esp32-runtime', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `esp32_runtime_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR
      const thirdPartyDir = path.join(PROJECT_ROOT, 'third_party')

      // 1. 复制核心源码
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'core/src'))

      // 2. 复制 HMI 源码
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'hmi/src'))

      // 3. 复制嵌入式 lv_conf.h
      await fs.mkdir(path.join(outDir, 'hmi/config'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/config'), path.join(outDir, 'hmi/config'))

      // 4. 复制 ESP32 平台层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'platform/esp32'), path.join(outDir, 'platform/esp32'))

      // 5. 复制 LVGL 源码到 components/
      const lvglSrc = path.join(thirdPartyDir, 'lvgl')
      if (!existsSync(lvglSrc)) {
        return { success: false, error: '未找到 LVGL 源码 (third_party/lvgl)', code }
      }
      await cpDir(lvglSrc, path.join(outDir, 'components/lvgl'))

      // 6. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 7. 写入 app_main.c
      const mainC = generateEsp32AppMain(project)
      await fs.writeFile(path.join(outDir, 'main/app_main.c'), mainC, 'utf-8')

      // 8. 写入 main/CMakeLists.txt
      const mainCMake = generateEsp32MainCMake(project)
      await fs.writeFile(path.join(outDir, 'main/CMakeLists.txt'), mainCMake, 'utf-8')

      // 9. 写入顶层 CMakeLists.txt
      const topCMake = `cmake_minimum_required(VERSION 3.14)\ninclude(\$ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject(plc-hmi-esp32-runtime)\n`
      await fs.writeFile(path.join(outDir, 'CMakeLists.txt'), topCMake, 'utf-8')

      // 10. 写入 sdkconfig.defaults
      await fs.writeFile(path.join(outDir, 'sdkconfig.defaults'),
        'CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y\n' +
        'CONFIG_PARTITION_TABLE_OFFSET=0x8000\n' +
        'CONFIG_FREERTOS_HZ=1000\n' +
        'CONFIG_LV_MEM_SIZE=65536\n'
      )

      return { success: true, outDir, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 STM32 运行时项目（PLC 核心 + HMI LVGL + ARM GCC 工程）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/stm32-runtime', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `stm32_runtime_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR
      const thirdPartyDir = path.join(PROJECT_ROOT, 'third_party')

      // 1. 复制核心源码
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'core/src'))

      // 2. 复制 HMI 源码
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'hmi/src'))

      // 3. 复制嵌入式 lv_conf.h
      await fs.mkdir(path.join(outDir, 'hmi/config'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/config'), path.join(outDir, 'hmi/config'))

      // 4. 复制 STM32 平台层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'platform/stm32'), path.join(outDir, 'platform/stm32'))

      // 5. 复制 LVGL 源码
      const lvglSrc = path.join(thirdPartyDir, 'lvgl')
      if (!existsSync(lvglSrc)) {
        return { success: false, error: '未找到 LVGL 源码 (third_party/lvgl)', code }
      }
      await cpDir(lvglSrc, path.join(outDir, 'lvgl'))

      // 6. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 7. 写入 main.c
      await fs.writeFile(path.join(outDir, 'main.c'), generateStm32Main(project), 'utf-8')

      // 8. 写入 CMakeLists.txt
      await fs.writeFile(path.join(outDir, 'CMakeLists.txt'), generateStm32RuntimeCMake(project), 'utf-8')

      return { success: true, outDir, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 Linux ARM 运行时项目（PLC 核心 + HMI LVGL + Linux GCC）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/linux-arm-runtime', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `linux_arm_runtime_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR
      const thirdPartyDir = path.join(PROJECT_ROOT, 'third_party')

      // 1. 复制核心源码
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'core/src'))

      // 2. 复制 HMI 源码（含 LVGL 驱动）
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'hmi/src'))

      // 3. 复制嵌入式 lv_conf.h
      await fs.mkdir(path.join(outDir, 'hmi/config'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/config'), path.join(outDir, 'hmi/config'))

      // 4. 复制 Linux ARM 平台层
      await fs.mkdir(path.join(outDir, 'platform'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'platform/linux-arm'), path.join(outDir, 'platform/linux-arm'))

      // 5. 复制 LVGL 源码
      const lvglSrc = path.join(thirdPartyDir, 'lvgl')
      if (!existsSync(lvglSrc)) {
        return { success: false, error: '未找到 LVGL 源码 (third_party/lvgl)', code }
      }
      await cpDir(lvglSrc, path.join(outDir, 'lvgl'))

      // 6. 写入生成代码
      const genDir = path.join(outDir, 'hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 7. 写入 main.c
      await fs.writeFile(path.join(outDir, 'main.c'), generateLinuxArmMain(project), 'utf-8')

      // 8. 写入 CMakeLists.txt
      await fs.writeFile(path.join(outDir, 'CMakeLists.txt'), generateLinuxArmRuntimeCMake(project), 'utf-8')

      return { success: true, outDir, code }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  /**
   * 生成 Arduino 运行时项目（PLC 核心 + HMI LVGL + Arduino 工程）
   */
  fastify.post<{
    Body: { project: HmiProject }
  }>('/export/arduino', async (request, reply) => {
    const { project } = request.body
    try {
      const code = generator.generate(project)
      const ts = Date.now()
      const outDir = path.join(HMI_OUTPUT_DIR, `arduino_runtime_${ts}`)
      await fs.mkdir(outDir, { recursive: true })

      const runtimeDir = RUNTIME_DIR
      const thirdPartyDir = path.join(PROJECT_ROOT, 'third_party')

      // 1. 复制核心源码到 src/
      await cpDir(path.join(runtimeDir, 'core/include'), path.join(outDir, 'src/core/include'))
      await cpDir(path.join(runtimeDir, 'core/src'), path.join(outDir, 'src/core/src'))

      // 2. 复制 HMI 源码到 src/
      await cpDir(path.join(runtimeDir, 'hmi/include'), path.join(outDir, 'src/hmi/include'))
      await cpDir(path.join(runtimeDir, 'hmi/src'), path.join(outDir, 'src/hmi/src'))

      // 3. 复制嵌入式 lv_conf.h
      await fs.mkdir(path.join(outDir, 'src/hmi/config'), { recursive: true })
      await cpDir(path.join(runtimeDir, 'hmi/config'), path.join(outDir, 'src/hmi/config'))

      // 4. 创建 Arduino 平台层
      await fs.mkdir(path.join(outDir, 'src/platform/arduino'), { recursive: true })
      await fs.writeFile(path.join(outDir, 'src/platform/arduino/platform.c'), generateArduinoPlatformC(), 'utf-8')
      await fs.writeFile(path.join(outDir, 'src/platform/arduino/platform.h'), generateArduinoPlatformH(), 'utf-8')

      // 5. 复制 LVGL 源码
      const lvglSrc = path.join(thirdPartyDir, 'lvgl')
      if (!existsSync(lvglSrc)) {
        return { success: false, error: '未找到 LVGL 源码 (third_party/lvgl)', code }
      }
      await cpDir(lvglSrc, path.join(outDir, 'src/lvgl'))

      // 6. 写入生成代码
      const genDir = path.join(outDir, 'src/hmi/generated')
      await fs.mkdir(genDir, { recursive: true })
      await fs.writeFile(path.join(genDir, 'plc_hmi_generated.c'), code, 'utf-8')

      // 7. 写入 Arduino .ino 主文件
      await fs.writeFile(path.join(outDir, 'hmi_plc_runtime.ino'), generateArduinoSketch(project), 'utf-8')

      return { success: true, outDir, code }
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

/**
 * 生成 Linux LVGL 仿真 CMakeLists.txt
 * Linux GCC + SDL2（通过 pkg-config 或 find_package）
 */
function generateLinuxCMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-linux VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== LVGL ====================
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
add_subdirectory(lvgl)

# ==================== SDL2 ====================
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(SDL2 REQUIRED sdl2)
else()
  find_package(SDL2 REQUIRED)
endif()

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
target_include_directories(plc-hmi PUBLIC hmi/include PRIVATE core/include)
target_compile_definitions(plc-hmi PRIVATE PLC_USE_LVGL)
target_link_libraries(plc-hmi PUBLIC lvgl)

# ==================== 可执行文件 ====================
add_executable(plc-hmi-linux
  hmi/platforms/lvgl-sim/hmi_lvgl_sim_main.c
  hmi/generated/plc_hmi_generated.c
  platform/linux-x86/platform.c
)
target_include_directories(plc-hmi-linux
  PRIVATE
    hmi/include core/include hmi/generated platform/linux-x86
    hmi/platforms/lvgl-sim
)
target_compile_definitions(plc-hmi-linux PRIVATE
  PLC_USE_LVGL LV_CONF_INCLUDE_SIMPLE
  PLC_HMI_SCREEN_WIDTH=${sw} PLC_HMI_SCREEN_HEIGHT=${sh}
)
target_link_libraries(plc-hmi-linux PRIVATE plc-core plc-hmi lvgl \${SDL2_LIBRARIES})
`
}

/**
 * 生成 STM32 LVGL CMakeLists.txt
 * ARM GCC 交叉编译 + STM32Cube HAL
 */
function generateStm32CMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-stm32 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ==================== ARM GCC 工具链 ====================
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER \${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER \${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER \${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR \${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY \${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE \${TOOLCHAIN_PREFIX}size)

set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -DUSE_HAL_DRIVER -DSTM32F407xx -O2 -ffunction-sections -fdata-sections" CACHE STRING "")
set(CMAKE_CXX_FLAGS "\${CMAKE_C_FLAGS}" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-Map=output.map" CACHE STRING "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ==================== LVGL ====================
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
# 使用 lv_conf.h 精简配置（不含 SDL）
set(LV_BUILD_CONF_DIR "\${CMAKE_SOURCE_DIR}/hmi/config" CACHE PATH "")
add_subdirectory(lvgl)

# ==================== 核心库 ====================
file(GLOB PLC_CORE_SOURCES "core/src/*.c")
add_library(plc-core STATIC \${PLC_CORE_SOURCES})
target_include_directories(plc-core PUBLIC core/include)
target_compile_definitions(plc-core PRIVATE PLATFORM_STM32)

# ==================== HMI 库 ====================
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_input.c
  hmi/src/plc_hmi_lvgl.c
)
target_include_directories(plc-hmi PUBLIC hmi/include PRIVATE core/include)
target_compile_definitions(plc-hmi PRIVATE PLC_USE_LVGL)
target_link_libraries(plc-hmi PUBLIC lvgl)

# ==================== 固件 ====================
# 注意：需要自行添加启动文件和链接脚本
#   platform/stm32/startup_stm32f4xx.s
#   platform/stm32/STM32F4xx_FLASH.ld
add_executable(plc-hmi-stm32.elf
  hmi/generated/plc_hmi_generated.c
  platform/stm32/platform.c
)
target_include_directories(plc-hmi-stm32.elf
  PRIVATE hmi/include core/include hmi/generated platform/stm32
)
target_compile_definitions(plc-hmi-stm32.elf PRIVATE
  PLC_USE_LVGL
  PLC_HMI_SCREEN_WIDTH=${sw} PLC_HMI_SCREEN_HEIGHT=${sh}
)
target_link_libraries(plc-hmi-stm32.elf PRIVATE plc-core plc-hmi lvgl)

# 生成 HEX / BIN（需配置 start / ld 后方可生效）
# add_custom_command(TARGET plc-hmi-stm32.elf POST_BUILD
#   COMMAND \${CMAKE_OBJCOPY} -O ihex plc-hmi-stm32.elf plc-hmi-stm32.hex
#   COMMAND \${CMAKE_OBJCOPY} -O binary plc-hmi-stm32.elf plc-hmi-stm32.bin
#   COMMAND \${CMAKE_SIZE} plc-hmi-stm32.elf
# )
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

// ==================== 运行时导出模板生成函数 ====================

function generateEsp32AppMain(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `/**
 * app_main.c - PLC 运行时 + HMI LVGL for ESP32
 *
 * 自动生成的 ESP-IDF 主入口，集成 PLC 核心引擎和 LVGL HMI 显示。
 * 包含 FreeRTOS 任务：PLC 扫描周期、HMI 刷新、通信处理。
 */

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "plc_runtime.h"
#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include "lvgl.h"

/* ESP-IDF 头文件 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char* TAG = "PLC-HMI";

/* ========== LVGL 外部接口 ========== */
extern int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                              void* buf1, void* buf2);
extern void plc_hmi_lvgl_update(void);
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* ========== 全局状态 ========== */
static PlcRuntime g_runtime;
static int32_t g_var_table[256];

/* ========== 引脚配置（用户需根据实际硬件修改） ========== */
#define PIN_DI_0         4
#define PIN_DO_0         18
#define PIN_AI_0         36  /* ADC1_CH0 (GPIO36) */

/* ========== 任务函数 ========== */

static void plc_scan_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;
  ESP_LOGI(TAG, "PLC 扫描任务已启动");

  for (;;) {
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);
    vTaskDelay(pdMS_TO_TICKS(1)); /* 1ms 扫描周期 */
  }
}

static void hmi_refresh_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  for (;;) {
    /* 刷新 LVGL */
    plc_hmi_lvgl_update();
    /* 同步 PLC 变量到 HMI */
    plc_hmi_screens_update(g_var_table, sizeof(g_var_table));
    vTaskDelay(pdMS_TO_TICKS(33)); /* ~30fps */
  }
}

/* ========== app_main ========== */

void app_main(void)
{
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "PLC Runtime + HMI - ESP32");
  ESP_LOGI(TAG, "  屏幕: %u x %u", ${sw}, ${sh});
  ESP_LOGI(TAG, "========================================");

  /* ====== 1. 初始化 PLC 运行时 ====== */
  plc_runtime_init(&g_runtime);

  /* 注册变量 */
  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "input_0", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "数字量输入 0");
  plc_var_register(vt, "output_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "数字量输出 0");
  plc_var_register(vt, "adc_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "模拟量输入 0");

  /* 配置 I/O */
  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_DI, "DI_0", "input_0", PIN_DI_0);
  plc_io_register(io, IO_TYPE_DO, "DO_0", "output_0", PIN_DO_0);
  plc_io_register(io, IO_TYPE_AI, "AI_0", "adc_0", PIN_AI_0);
  plc_io_bind(io, 0, vt);
  plc_io_bind(io, 1, vt);
  plc_io_bind(io, 2, vt);

  /* 创建任务 */
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  1, 200, NULL, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  10, 100, NULL, &g_runtime);

  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  /* ====== 2. 初始化 LVGL HMI ====== */
  /* TODO: 替换 buf1/buf2 为实际显示缓冲区 */
  static lv_color_t lvgl_buf1[${sw} * 40];
  plc_hmi_lvgl_init(${sw}, ${sh}, lvgl_buf1, NULL);
  plc_hmi_screens_init();

  /* ====== 3. 创建 FreeRTOS 任务 ====== */
  xTaskCreatePinnedToCore(plc_scan_task, "plc_scan", 4096,
                          &g_runtime, configMAX_PRIORITIES - 1, NULL, 1);
  xTaskCreatePinnedToCore(hmi_refresh_task, "hmi_refresh", 4096,
                          &g_runtime, configMAX_PRIORITIES - 2, NULL, 1);

  ESP_LOGI(TAG, "PLC + HMI 系统已启动");

  /* 主循环：状态统计 */
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    PlcStats stats;
    plc_runtime_get_stats(&g_runtime, &stats);
    ESP_LOGI(TAG, "运行: %u s | 周期: %u us | 最大: %u us | 错误: %u",
             stats.uptime_ms / 1000, stats.cycle_time_us,
             stats.max_cycle_time_us, stats.error_count);
  }
}
`
}

function generateEsp32MainCMake(project: HmiProject): string {
  return `idf_component_register(
  SRCS
    "app_main.c"
    "../hmi/generated/plc_hmi_generated.c"
    "../core/src/plc_runtime.c"
    "../core/src/plc_platform.c"
    "../core/src/plc_task.c"
    "../core/src/plc_io.c"
    "../core/src/plc_var.c"
    "../core/src/plc_comm.c"
    "../core/src/plc_debug.c"
    "../hmi/src/plc_hmi.c"
    "../hmi/src/plc_hmi_widget.c"
    "../hmi/src/plc_hmi_driver.c"
    "../hmi/src/plc_hmi_input.c"
    "../hmi/src/plc_hmi_lvgl.c"
    "../platform/esp32/platform.c"
  INCLUDE_DIRS
    "."
    "../core/include"
    "../hmi/include"
    "../hmi/generated"
    "../hmi/config"
    "../platform/esp32"
  REQUIRES
    lvgl
)
`
}

function generateStm32Main(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `/**
 * main.c - PLC 运行时 + HMI LVGL for STM32
 *
 * 自动生成的 ARM GCC 主入口，集成 PLC 核心引擎和 LVGL HMI 显示。
 * 基于 STM32 HAL + FreeRTOS。
 * 注意：需配置启动文件和链接脚本（startup_stm32f4xx.s, STM32F4xx_FLASH.ld）。
 */

#include <stdio.h>
#include <string.h>
#include "plc_runtime.h"
#include "plc_hmi.h"
#include "plc_hmi_widget.h"

/* STM32 HAL 头文件（用户根据实际芯片修改） */
/* #include "stm32f4xx_hal.h" */
#include "lvgl.h"

/* ========== LVGL 外部接口 ========== */
extern int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                              void* buf1, void* buf2);
extern void plc_hmi_lvgl_update(void);
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* ========== 全局状态 ========== */
static PlcRuntime g_runtime;
static int32_t g_var_table[128];

/* ========== HAL 初始化桩 ========== */
/* TODO: 替换为实际 HAL 初始化（时钟、GPIO、ADC、PWM、显示接口等） */

static void SystemClock_Config(void)
{
  /* TODO: HAL_RCC_OscConfig + HAL_RCC_ClockConfig */
}

static void MX_GPIO_Init(void)
{
  /* TODO: GPIO 初始化（DIs, DOs, LCD 控制等） */
}

static void MX_LTDC_Init(void)
{
  /* TODO: LTDC 初始化（LVGL 显示输出，如使用 TFT LCD） */
}

static void MX_SPI_Init(void)
{
  /* TODO: SPI 初始化（如使用 SPI 显示屏） */
}

/* ========== printf 重定向到 UART ========== */

#ifdef __GNUC__
int _write(int fd, char* ptr, int len)
{
  (void)fd;
  /* TODO: HAL_UART_Transmit(&huartx, (uint8_t*)ptr, len, HAL_MAX_DELAY); */
  (void)ptr;
  return len;
}
#endif

/* ========== FreeRTOS 任务 ========== */

static void plc_scan_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  for (;;) {
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static void hmi_refresh_task(void* pvParameters)
{
  PlcRuntime* rt = (PlcRuntime*)pvParameters;

  for (;;) {
    plc_hmi_lvgl_update();
    plc_hmi_screens_update(g_var_table, sizeof(g_var_table));
    vTaskDelay(pdMS_TO_TICKS(33));
  }
}

static void led_task(void* pvParameters)
{
  (void)pvParameters;
  for (;;) {
    /* TODO: HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); */
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/* ========== main ========== */

int main(void)
{
  printf("PLC Runtime + HMI - STM32\\r\\n");
  printf("  屏幕: %u x %u\\r\\n", ${sw}, ${sh});

  /* HAL 初始化 */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_LTDC_Init();
  MX_SPI_Init();

  printf("HAL 初始化完成\\r\\n");

  /* ====== 1. 初始化 PLC 运行时 ====== */
  plc_runtime_init(&g_runtime);

  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "sensor_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "传感器 0");
  plc_var_register(vt, "motor_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "电机 0");
  plc_var_register(vt, "alarm", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "报警输出");

  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_AI, "AI_0", "sensor_0", 0);
  plc_io_register(io, IO_TYPE_DO, "DO_0", "motor_0", 1);
  plc_io_register(io, IO_TYPE_DO, "DO_1", "alarm", 2);
  plc_io_bind(io, 0, vt);
  plc_io_bind(io, 1, vt);
  plc_io_bind(io, 2, vt);

  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  1, 200, NULL, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  10, 100, NULL, &g_runtime);

  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  /* ====== 2. 初始化 LVGL HMI ====== */
  /* TODO: 替换 buf1/buf2 为实际显示缓冲区 */
  static lv_color_t lvgl_buf1[${sw} * 40];
  plc_hmi_lvgl_init(${sw}, ${sh}, lvgl_buf1, NULL);
  plc_hmi_screens_init();

  /* ====== 3. 创建 FreeRTOS 任务 ====== */
  xTaskCreate(plc_scan_task, "PLC_Scan", 512, &g_runtime, 5, NULL);
  xTaskCreate(hmi_refresh_task, "HMI", 1024, &g_runtime, 4, NULL);
  xTaskCreate(led_task, "LED", 128, NULL, 1, NULL);

  printf("FreeRTOS 任务已创建，启动调度器...\\r\\n");

  vTaskStartScheduler();

  for (;;) { }
}

/* ========== FreeRTOS 钩子 ========== */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
  (void)xTask;
  printf("栈溢出! 任务: %s\\r\\n", pcTaskName);
  for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
  printf("内存分配失败!\\r\\n");
  for (;;) { }
}
`
}

function generateStm32RuntimeCMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-stm32-runtime VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ==================== ARM GCC 工具链 ====================
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER \${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER \${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER \${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR \${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY \${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE \${TOOLCHAIN_PREFIX}size)

# ==================== 编译选项 ====================
# 注意：根据实际芯片修改 -mcpu 和 -DSTM32Fxxx
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -DUSE_HAL_DRIVER -DSTM32F407xx -O2 -ffunction-sections -fdata-sections" CACHE STRING "")
set(CMAKE_CXX_FLAGS "\${CMAKE_C_FLAGS}" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-Map=output.map \
  -Tplatform/stm32/STM32F4xx_FLASH.ld" CACHE STRING "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ==================== LVGL ====================
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
set(LV_BUILD_CONF_DIR "\${CMAKE_SOURCE_DIR}/hmi/config" CACHE PATH "")
add_subdirectory(lvgl)

# ==================== 核心库 ====================
file(GLOB PLC_CORE_SOURCES "core/src/*.c")
add_library(plc-core STATIC \${PLC_CORE_SOURCES})
target_include_directories(plc-core PUBLIC core/include)
target_compile_definitions(plc-core PRIVATE PLATFORM_STM32)

# ==================== HMI 库 ====================
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_input.c
  hmi/src/plc_hmi_lvgl.c
)
target_include_directories(plc-hmi PUBLIC hmi/include PRIVATE core/include)
target_compile_definitions(plc-hmi PRIVATE PLC_USE_LVGL)
target_link_libraries(plc-hmi PUBLIC lvgl)

# ==================== 固件 ====================
add_executable(plc-hmi-stm32-runtime.elf
  main.c
  hmi/generated/plc_hmi_generated.c
  platform/stm32/platform.c
)
target_include_directories(plc-hmi-stm32-runtime.elf
  PRIVATE
    hmi/include core/include hmi/generated hmi/config
    platform/stm32
)
target_compile_definitions(plc-hmi-stm32-runtime.elf PRIVATE
  PLC_USE_LVGL
  PLC_HMI_SCREEN_WIDTH=${sw}
  PLC_HMI_SCREEN_HEIGHT=${sh}
)
target_link_libraries(plc-hmi-stm32-runtime.elf PRIVATE plc-core plc-hmi lvgl)

# HEX / BIN 生成（配置 startup + ld 后方可生效）
add_custom_command(TARGET plc-hmi-stm32-runtime.elf POST_BUILD
  COMMAND \${CMAKE_OBJCOPY} -O ihex plc-hmi-stm32-runtime.elf plc-hmi-stm32-runtime.hex
  COMMAND \${CMAKE_OBJCOPY} -O binary plc-hmi-stm32-runtime.elf plc-hmi-stm32-runtime.bin
  COMMAND \${CMAKE_SIZE} plc-hmi-stm32-runtime.elf
)
`
}

function generateLinuxArmMain(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `/**
 * main.c - PLC 运行时 + HMI LVGL for ARM Linux
 *
 * 自动生成的 Linux GCC 主入口，集成 PLC 核心引擎和 LVGL HMI 显示。
 * 使用 Linux 帧缓冲（fbdev）作为 LVGL 显示后端。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "plc_runtime.h"
#include "plc_hmi.h"
#include "plc_hmi_widget.h"
#include "lvgl.h"

/* ========== LVGL 外部接口 ========== */
extern int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                              void* buf1, void* buf2);
extern void plc_hmi_lvgl_update(void);
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* ========== 全局状态 ========== */
static volatile int g_running = 1;
static PlcRuntime g_runtime;
static int32_t g_var_table[256];

/* ========== 信号处理 ========== */

static void signal_handler(int signo)
{
  (void)signo;
  printf("[ARM] 收到信号，正在关闭...\\n");
  g_running = 0;
}

/* ========== PLC 扫描线程 ========== */

static void* plc_scan_thread(void* arg)
{
  PlcRuntime* rt = (PlcRuntime*)arg;
  printf("[ARM] PLC 扫描线程已启动\\n");

  while (g_running) {
    plc_task_schedule(&rt->task_scheduler);
    plc_runtime_scan(rt);
    usleep(1000); /* 1ms */
  }
  return NULL;
}

/* ========== HMI 刷新线程 ========== */

static void* hmi_refresh_thread(void* arg)
{
  PlcRuntime* rt = (PlcRuntime*)arg;
  printf("[ARM] HMI 刷新线程已启动\\n");

  while (g_running) {
    plc_hmi_lvgl_update();
    plc_hmi_screens_update(g_var_table, sizeof(g_var_table));
    usleep(33333); /* ~30fps */
  }
  return NULL;
}

/* ========== 主函数 ========== */

int main(void)
{
  printf("========================================\\n");
  printf("PLC Runtime + HMI - ARM Linux\\n");
  printf("  屏幕: %u x %u\\n", ${sw}, ${sh});
  printf("========================================\\n");

  /* 信号处理 */
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  /* ====== 1. 初始化 PLC 运行时 ====== */
  plc_runtime_init(&g_runtime);

  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "motor_speed", VAR_TYPE_REAL, VAR_ATTR_OUTPUT,
                   sizeof(plc_real), "电机转速 (RPM)");
  plc_var_register(vt, "sensor_temp", VAR_TYPE_REAL, VAR_ATTR_INPUT,
                   sizeof(plc_real), "温度传感器");
  plc_var_register(vt, "start_btn", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "启动按钮");
  plc_var_register(vt, "alarm_out", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "报警输出");

  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  int ret;
  ret = plc_io_register(io, IO_TYPE_DI, "DI_0", "start_btn", 0);
  ret = plc_io_register(io, IO_TYPE_AI, "AI_0", "sensor_temp", 1);
  ret = plc_io_register(io, IO_TYPE_DO, "DO_0", "alarm_out", 2);
  ret = plc_io_register(io, IO_TYPE_AO, "AO_0", "motor_speed", 3);
  (void)ret;
  plc_io_bind(io, 0, vt);
  plc_io_bind(io, 1, vt);
  plc_io_bind(io, 2, vt);
  plc_io_bind(io, 3, vt);

  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  10, 200, NULL, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  100, 100, NULL, &g_runtime);

  ret = plc_runtime_load(&g_runtime);
  if (ret != 0) {
    fprintf(stderr, "[ARM] 加载 PLC 运行时失败: %d\\n", ret);
    return EXIT_FAILURE;
  }
  plc_runtime_start(&g_runtime);

  /* ====== 2. 初始化 LVGL HMI ====== */
  /* TODO: 替换为实际显示缓冲区或使用 fbdev 驱动 */
  static lv_color_t lvgl_buf1[${sw} * 60];
  plc_hmi_lvgl_init(${sw}, ${sh}, lvgl_buf1, NULL);
  plc_hmi_screens_init();

  /* ====== 3. 创建线程 ====== */
  pthread_t plc_thread, hmi_thread;
  pthread_create(&plc_thread, NULL, plc_scan_thread, &g_runtime);
  pthread_create(&hmi_thread, NULL, hmi_refresh_thread, &g_runtime);

  printf("[ARM] PLC + HMI 系统已启动 (Ctrl+C 退出)\\n");

  /* 主循环：状态统计 */
  uint32_t last_tick = plc_platform_tick_ms();

  while (g_running) {
    uint32_t now = plc_platform_tick_ms();
    if (now - last_tick >= 5000) {
      PlcStats stats;
      plc_runtime_get_stats(&g_runtime, &stats);
      printf("[ARM] 运行: %u s | 周期: %u us | 最大: %u us | 错误: %u\\n",
             stats.uptime_ms / 1000, stats.cycle_time_us,
             stats.max_cycle_time_us, stats.error_count);
      last_tick = now;
    }
    usleep(100000); /* 100ms */
  }

  /* 清理 */
  printf("\\n[ARM] 正在停止...\\n");
  g_running = 0;
  pthread_join(plc_thread, NULL);
  pthread_join(hmi_thread, NULL);
  plc_runtime_stop(&g_runtime);
  printf("[ARM] PLC 运行时已停止\\n");

  return EXIT_SUCCESS;
}
`
}

function generateLinuxArmRuntimeCMake(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `cmake_minimum_required(VERSION 3.14)
project(plc-hmi-linux-arm-runtime VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== LVGL ====================
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
set(LV_BUILD_CONF_DIR "\${CMAKE_SOURCE_DIR}/hmi/config" CACHE PATH "")
add_subdirectory(lvgl)

# LVGL 启用 Linux fbdev 驱动
target_compile_definitions(lvgl PRIVATE LV_USE_LINUX_FBDEV=1)

# ==================== 核心库 ====================
file(GLOB PLC_CORE_SOURCES "core/src/*.c")
add_library(plc-core STATIC \${PLC_CORE_SOURCES})
target_include_directories(plc-core PUBLIC core/include)
target_compile_definitions(plc-core PRIVATE PLATFORM_LINUX_ARM)

# ==================== HMI 库 ====================
add_library(plc-hmi STATIC
  hmi/src/plc_hmi.c
  hmi/src/plc_hmi_widget.c
  hmi/src/plc_hmi_driver.c
  hmi/src/plc_hmi_input.c
  hmi/src/plc_hmi_lvgl.c
)
target_include_directories(plc-hmi PUBLIC hmi/include PRIVATE core/include)
target_compile_definitions(plc-hmi PRIVATE PLC_USE_LVGL)
target_link_libraries(plc-hmi PUBLIC lvgl)

# ==================== 可执行文件 ====================
add_executable(plc-hmi-linux-arm-runtime
  main.c
  hmi/generated/plc_hmi_generated.c
  platform/linux-arm/platform.c
)
target_include_directories(plc-hmi-linux-arm-runtime
  PRIVATE
    hmi/include core/include hmi/generated hmi/config
    platform/linux-arm
)
target_compile_definitions(plc-hmi-linux-arm-runtime PRIVATE
  PLC_USE_LVGL
  PLC_HMI_SCREEN_WIDTH=${sw}
  PLC_HMI_SCREEN_HEIGHT=${sh}
)
target_link_libraries(plc-hmi-linux-arm-runtime
  PRIVATE plc-core plc-hmi lvgl pthread)
`
}

function generateArduinoSketch(project: HmiProject): string {
  const firstForm = project.forms[0] || { width: 800, height: 480 }
  const sw = firstForm.width
  const sh = firstForm.height

  return `/**
 * hmi_plc_runtime.ino - PLC 运行时 + HMI for Arduino
 *
 * 自动生成的 Arduino 主文件，集成 PLC 核心引擎和 HMI 显示。
 * 在 setup() 中初始化 PLC 运行时和 HMI，在 loop() 中执行扫描周期。
 */

#include "src/core/include/plc_runtime.h"
#include "src/hmi/include/plc_hmi.h"
#include "src/hmi/include/plc_hmi_widget.h"

/* ========== LVGL 外部接口 ========== */
extern int plc_hmi_lvgl_init(uint16_t display_w, uint16_t display_h,
                              void* buf1, void* buf2);
extern void plc_hmi_lvgl_update(void);
extern void plc_hmi_screens_init(void);
extern void plc_hmi_screens_update(void* var_table, uint32_t var_table_size);

/* ========== 全局状态 ========== */
static PlcRuntime g_runtime;
static int32_t g_var_table[64];

/* ========== 引脚定义 ========== */
const int PIN_DI_0 = 2;
const int PIN_DI_1 = 3;
const int PIN_DO_0 = 4;
const int PIN_DO_1 = 5;
const int PIN_AI_0 = A0;
const int PIN_AI_1 = A1;

/* ========== 时钟跟踪 ========== */
static uint32_t g_last_plc_tick = 0;
static uint32_t g_last_hmi_tick = 0;
static uint32_t g_last_stat_tick = 0;
static uint32_t g_scan_count = 0;

/* ========== setup ========== */

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("========================================");
  Serial.println("PLC Runtime + HMI - Arduino");
  Serial.println("  屏幕: ${sw} x ${sh}");
  Serial.println("========================================");

  /* 引脚初始化 */
  pinMode(PIN_DI_0, INPUT_PULLUP);
  pinMode(PIN_DI_1, INPUT_PULLUP);
  pinMode(PIN_DO_0, OUTPUT);
  pinMode(PIN_DO_1, OUTPUT);

  /* ====== 1. 初始化 PLC 运行时 ====== */
  plc_runtime_init(&g_runtime);

  PlcVarTable* vt = plc_runtime_get_var_table(&g_runtime);
  plc_var_register(vt, "button_0", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "按钮 0");
  plc_var_register(vt, "button_1", VAR_TYPE_BOOL, VAR_ATTR_INPUT,
                   sizeof(plc_bool), "按钮 1");
  plc_var_register(vt, "led_0", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "LED 0");
  plc_var_register(vt, "led_1", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT,
                   sizeof(plc_bool), "LED 1");
  plc_var_register(vt, "analog_0", VAR_TYPE_UINT, VAR_ATTR_INPUT,
                   sizeof(plc_uint), "模拟量 0");

  PlcIoConfig* io = plc_runtime_get_io_config(&g_runtime);
  plc_io_register(io, IO_TYPE_DI, "DI_0", "button_0", PIN_DI_0);
  plc_io_register(io, IO_TYPE_DI, "DI_1", "button_1", PIN_DI_1);
  plc_io_register(io, IO_TYPE_DO, "DO_0", "led_0", PIN_DO_0);
  plc_io_register(io, IO_TYPE_DO, "DO_1", "led_1", PIN_DO_1);
  plc_io_register(io, IO_TYPE_AI, "AI_0", "analog_0", PIN_AI_0);
  plc_io_bind(io, 0, vt);
  plc_io_bind(io, 1, vt);
  plc_io_bind(io, 2, vt);
  plc_io_bind(io, 3, vt);
  plc_io_bind(io, 4, vt);

  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);
  plc_task_create(sched, "MainTask", TASK_TYPE_CYCLIC,
                  10, 200, NULL, &g_runtime);
  plc_task_create(sched, "CommTask", TASK_TYPE_CYCLIC,
                  100, 100, NULL, &g_runtime);

  plc_runtime_load(&g_runtime);
  plc_runtime_start(&g_runtime);

  /* ====== 2. 初始化 HMI ====== */
  /* TODO: 初始化具体显示驱动（TFT、OLED 等） */
  /* 示例：plc_hmi_lvgl_init(${sw}, ${sh}, display_buf, NULL); */
  plc_hmi_screens_init();

  Serial.println("PLC + HMI 系统已启动");
}

/* ========== loop ========== */

void loop()
{
  uint32_t now = millis();
  PlcTaskScheduler* sched = plc_runtime_get_scheduler(&g_runtime);

  /* PLC 扫描：1ms 周期 */
  if (now - g_last_plc_tick >= 1) {
    g_last_plc_tick = now;
    plc_task_schedule(sched);
    plc_runtime_scan(&g_runtime);
    g_scan_count++;
  }

  /* HMI 刷新：~30fps */
  if (now - g_last_hmi_tick >= 33) {
    g_last_hmi_tick = now;
    plc_hmi_screens_update(g_var_table, sizeof(g_var_table));
    /* TODO: 调用具体显示驱动的刷新函数 */
    /* plc_hmi_lvgl_update(); */
  }

  /* 状态输出：每 5 秒 */
  if (now - g_last_stat_tick >= 5000) {
    g_last_stat_tick = now;
    PlcStats stats;
    plc_runtime_get_stats(&g_runtime, &stats);
    Serial.print("运行: ");
    Serial.print(stats.uptime_ms / 1000);
    Serial.print(" s | 扫描: ");
    Serial.print(g_scan_count);
    Serial.print(" | 周期: ");
    Serial.print(stats.cycle_time_us);
    Serial.print(" us | 错误: ");
    Serial.println(stats.error_count);
  }
}
`
}

function generateArduinoPlatformC(): string {
  return `/**
 * platform.c - Arduino 平台适配
 *
 * 使用 Arduino API 实现 PLC 运行时 HAL 接口。
 * 支持: GPIO (digitalRead/Write), ADC (analogRead),
 *       PWM (analogWrite), UART (Serial), 计时器 (millis/micros)
 */

#include "platform.h"
#include "plc_platform.h"
#include "plc_io.h"
#include "plc_comm.h"

/* Arduino 头文件由 .ino 自动包含 */

/* ========== 平台初始化 ========== */

int plc_platform_init(void)
{
  /* Arduino 的 init() 由 main() 自动调用 */
  return 0;
}

/* ========== 时间 ========== */

uint64_t plc_platform_tick_ms(void)
{
  return (uint64_t)millis();
}

uint64_t plc_platform_tick_us(void)
{
  return (uint64_t)micros();
}

void plc_platform_delay_ms(uint32_t ms)
{
  delay(ms);
}

void plc_platform_delay_us(uint32_t us)
{
  delayMicroseconds(us);
}

/* ========== 临界区 ========== */

void plc_platform_critical_enter(void)
{
  noInterrupts();
}

void plc_platform_critical_exit(void)
{
  interrupts();
}

/* ========== 内存 ========== */

void* plc_platform_malloc(size_t size)
{
  return malloc(size);
}

void plc_platform_free(void* ptr)
{
  free(ptr);
}

/* ========== 日志 ========== */

void plc_platform_log(const char* tag, const char* msg)
{
  Serial.print("[");
  Serial.print(tag);
  Serial.print("] ");
  Serial.println(msg);
}

/* ========== HAL I/O ========== */

int plc_hal_read_input(uint32_t pin, uint32_t* value)
{
  *value = (uint32_t)digitalRead((int)pin);
  return 0;
}

int plc_hal_write_output(uint32_t pin, uint32_t value)
{
  digitalWrite((int)pin, value ? HIGH : LOW);
  return 0;
}

int plc_hal_read_adc(uint32_t channel, uint32_t* value)
{
  *value = (uint32_t)analogRead((int)channel);
  return 0;
}

int plc_hal_write_pwm(uint32_t pin, uint32_t duty)
{
  analogWrite((int)pin, (int)duty);
  return 0;
}

/* ========== TCP（未实现 - Arduino 需要 WiFi/Ethernet 库） ========== */

int plc_hal_tcp_connect(const char* host, uint16_t port, uint32_t timeout_ms)
{
  (void)host; (void)port; (void)timeout_ms;
  return -1;
}

int plc_hal_tcp_close(int fd)
{
  (void)fd;
  return -1;
}

int plc_hal_tcp_send(int fd, const uint8_t* data, uint32_t len, uint32_t timeout_ms)
{
  (void)fd; (void)data; (void)len; (void)timeout_ms;
  return -1;
}

int plc_hal_tcp_recv(int fd, uint8_t* buf, uint32_t max_len, uint32_t timeout_ms)
{
  (void)fd; (void)buf; (void)max_len; (void)timeout_ms;
  return -1;
}

/* ========== 串口 ========== */

int plc_hal_serial_open(const char* port, uint32_t baud)
{
  (void)port;
  Serial.begin(baud);
  return 0;
}

int plc_hal_serial_close(int fd)
{
  (void)fd;
  Serial.end();
  return 0;
}

int plc_hal_serial_send(int fd, const uint8_t* data, uint32_t len, uint32_t timeout_ms)
{
  (void)fd; (void)timeout_ms;
  return (int)Serial.write(data, (size_t)len);
}

int plc_hal_serial_recv(int fd, uint8_t* buf, uint32_t max_len, uint32_t timeout_ms)
{
  (void)fd;
  uint32_t start = millis();
  uint32_t count = 0;
  while (count < max_len) {
    if (Serial.available()) {
      buf[count++] = (uint8_t)Serial.read();
    } else if (millis() - start > timeout_ms) {
      break;
    }
  }
  return (int)count;
}
`
}

function generateArduinoPlatformH(): string {
  return `#ifndef PLC_ARDUINO_PLATFORM_H
#define PLC_ARDUINO_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

int plc_hal_read_adc(uint32_t channel, uint32_t* value);
int plc_hal_write_pwm(uint32_t pin, uint32_t duty);

#ifdef __cplusplus
}
#endif

#endif /* PLC_ARDUINO_PLATFORM_H */
`
}
