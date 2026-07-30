import { exec, spawn } from 'child_process'
import * as path from 'path'
import * as os from 'os'
import { promisify } from 'util'

const execAsync = promisify(exec)

const RUNTIME_DIR = path.resolve(process.cwd(), '../runtime')

const OPENOCD_PATH = 'C:\\msys64\\mingw64\\bin\\openocd.exe'

type BuildResult = { success: boolean; output: string; error?: string }

export class BuildService {
  private async cmakeConfigure(platform: string, buildDir: string): Promise<BuildResult> {
    try {
      const stubsHal = path.resolve(RUNTIME_DIR, 'stm32-stubs')
      const stubsFreeRtos = path.resolve(RUNTIME_DIR, 'stm32-stubs/Middlewares/Third_Party/FreeRTOS')
      const toolchainFlag = platform === 'stm32' ? ` -DCMAKE_TOOLCHAIN_FILE=cmake/stm32_gcc.cmake` : ''
      const { stdout, stderr } = await execAsync(
        `cmake -G "MinGW Makefiles"` +
        ` -DCMAKE_BUILD_TYPE=Release` +
        ` -DPLATFORM=${platform}` +
        ` -DSTM32_HAL_DIR:PATH="${stubsHal}"` +
        ` -DFREERTOS_DIR:PATH="${stubsFreeRtos}"` +
        ` -DBUILD_HAL=OFF -DBUILD_PROTOCOL=OFF -DBUILD_HMI=OFF -DBUILD_MOTION=ON` +
        ` -DBUILD_SIMULATION=OFF -DBUILD_DEVICE=OFF` +
        toolchainFlag +
        ` -S . -B ${buildDir}`,
        { cwd: RUNTIME_DIR, timeout: 120000 }
      )
      return { success: true, output: stdout + stderr }
    } catch (err: any) {
      return { success: false, output: String(err.stdout || ''), error: String(err.stderr || err.message || err).trim() }
    }
  }

  async compile(platform: string): Promise<BuildResult> {
    const buildDir = platform === 'stm32' ? 'build-stm32' : `build-${platform}`
    const jobs = os.cpus().length

    // 先重新配置 CMake（确保桩路径正确）
    const config = await this.cmakeConfigure(platform, buildDir)
    if (!config.success) return config

    try {
      const { stdout, stderr } = await execAsync(
        `cmake --build ${buildDir} -- -j${jobs}`,
        { cwd: RUNTIME_DIR, timeout: 300000 }
      )
      return { success: true, output: stdout + stderr }
    } catch (err: any) {
      return { success: false, output: String(err.stdout || ''), error: String(err.stderr || err.message || err).trim() }
    }
  }

  async flashStlink(): Promise<BuildResult> {
    return new Promise((resolve) => {
      // 正斜杠避免 \r 被 OpenOCD TCL 解释为回车
      const hex = path.resolve(RUNTIME_DIR, 'build-stm32/platform/stm32/plc-runtime.hex').replace(/\\/g, '/')
      const proc = spawn(OPENOCD_PATH, [
        '-f', 'interface/stlink.cfg',
        '-f', 'target/stm32f4x.cfg',
        '-c', `program ${hex} verify reset exit`,
      ], { cwd: RUNTIME_DIR })

      let output = ''
      proc.stdout.on('data', (d: Buffer) => { output += d.toString() })
      proc.stderr.on('data', (d: Buffer) => { output += d.toString() })

      proc.on('close', (code) => {
        if (code === 0) {
          resolve({ success: true, output })
        } else {
          resolve({ success: false, output, error: `openocd 退出码: ${code}` })
        }
      })

      proc.on('error', (err) => {
        resolve({ success: false, output, error: err.message })
      })
    })
  }

  async flashJlink(): Promise<BuildResult> {
    return new Promise((resolve) => {
      const bin = path.resolve(RUNTIME_DIR, 'build-stm32/platform/stm32/plc-runtime.bin').replace(/\\/g, '/')
      const proc = spawn('JLinkExe', [
        '-device', 'STM32F407VG',
        '-if', 'SWD',
        '-speed', '4000',
        '-autoconnect', '1',
        '-CommanderScript', '-',
      ], { cwd: RUNTIME_DIR })

      let output = ''
      proc.stdout.on('data', (d: Buffer) => { output += d.toString() })
      proc.stderr.on('data', (d: Buffer) => { output += d.toString() })

      // 发送 JLink 烧录命令
      const commands = [
        'loadbin ' + bin + ', 0x08000000',
        'r',
        'g',
        'exit',
      ]
      proc.stdin.write(commands.join('\n'))
      proc.stdin.end()

      proc.on('close', (code) => {
        if (code === 0) {
          resolve({ success: true, output })
        } else {
          resolve({ success: false, output, error: `JLinkExe 退出码: ${code}` })
        }
      })

      proc.on('error', (err) => {
        resolve({ success: false, output, error: err.message })
      })
    })
  }

  async flashEsp32(port: string): Promise<BuildResult> {
    return new Promise((resolve) => {
      const proc = spawn('esptool.py', [
        '--chip', 'esp32',
        '--port', port,
        '--baud', '921600',
        'write_flash', '-z',
        '0x1000', 'build-esp32/plc-runtime.bin',
      ], { cwd: RUNTIME_DIR })

      let output = ''
      proc.stdout.on('data', (d: Buffer) => { output += d.toString() })
      proc.stderr.on('data', (d: Buffer) => { output += d.toString() })

      proc.on('close', (code) => {
        if (code === 0) {
          resolve({ success: true, output })
        } else {
          resolve({ success: false, output, error: `esptool.py 退出码: ${code}` })
        }
      })

      proc.on('error', (err) => {
        resolve({ success: false, output, error: err.message })
      })
    })
  }

  async sshTransfer(host: string, port: number): Promise<BuildResult> {
    return new Promise((resolve) => {
      const proc = spawn('scp', [
        '-P', String(port),
        'build-linux/plc-runtime',
        `root@${host}:/usr/local/bin/plc-runtime`,
      ], { cwd: RUNTIME_DIR })

      let output = ''
      proc.stdout.on('data', (d: Buffer) => { output += d.toString() })
      proc.stderr.on('data', (d: Buffer) => { output += d.toString() })

      proc.on('close', (code) => {
        if (code === 0) {
          resolve({ success: true, output })
        } else {
          resolve({ success: false, output, error: `scp 退出码: ${code}` })
        }
      })

      proc.on('error', (err) => {
        resolve({ success: false, output, error: err.message })
      })
    })
  }

  async getBuildInfo(): Promise<{ configured: boolean; compiler: string }> {
    try {
      const { stdout } = await execAsync('arm-none-eabi-gcc --version', { timeout: 5000 })
      return { configured: true, compiler: stdout.split('\n')[0] }
    } catch {
      return { configured: false, compiler: '' }
    }
  }
}
