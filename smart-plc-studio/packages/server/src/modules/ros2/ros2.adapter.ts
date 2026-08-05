import { execFile } from 'child_process'
import { promisify } from 'util'
import * as fs from 'fs'

const execFileAsync = promisify(execFile)

export type Ros2Strategy = 'rclnodejs' | 'cli' | 'bridge' | 'none'

export interface Ros2Status {
  strategy: Ros2Strategy
  rosDistro: string | null
  rosPrefix: string | null
  ros2Cli: boolean
  rclnodejs: boolean
  bridgeUrl: string | null
  description: string
}

interface Ros2Env {
  distro: string | null
  prefix: string | null
}

function detectEnv(): Ros2Env {
  const env = process.env
  if (env.ROS_DISTRO) {
    return { distro: env.ROS_DISTRO, prefix: env.AMENT_PREFIX_PATH ?? null }
  }
  // 常见安装路径探测
  const candidates = [
    '/opt/ros/humble', '/opt/ros/jazzy', '/opt/ros/kilted', '/opt/ros/rolling',
    '/opt/ros/lyrical',
  ]
  for (const p of candidates) {
    if (fs.existsSync(p)) {
      return { distro: p.split('/').pop()!, prefix: p }
    }
  }
  return { distro: null, prefix: null }
}

function hasCli(): boolean {
  return process.env.ROS_DISTRO !== undefined || process.env.AMENT_PREFIX_PATH !== undefined
}

export async function getStatus(): Promise<Ros2Status> {
  const env = detectEnv()
  const cli = hasCli()
  const bridgeUrl = process.env.ROS2_BRIDGE_URL ?? null
  let rclnodejs = false
  if (env.distro) {
    try {
      await import('rclnodejs')
      rclnodejs = true
    } catch {
      rclnodejs = false
    }
  }

  const strategy: Ros2Strategy = bridgeUrl ? 'bridge'
    : rclnodejs ? 'rclnodejs'
    : cli ? 'cli'
    : 'none'

  const description =
    strategy === 'rclnodejs'
      ? `使用 rclnodejs 原生节点直连 ROS 2 (${env.distro})`
      : strategy === 'cli'
        ? `使用 ros2 命令行通道连接 ROS 2 (${env.distro})`
        : strategy === 'bridge'
          ? `通过远程能力桥接 ${bridgeUrl} 连接 ROS 2`
          : '未检测到 ROS 2 环境（Linux 安装 ROS 2，或 macOS 使用社区 brew 包并 source setup）'

  return {
    strategy,
    rosDistro: env.distro,
    rosPrefix: env.prefix,
    ros2Cli: cli,
    rclnodejs,
    bridgeUrl,
    description,
  }
}

// ===== CLI 策略实现（跨平台，依赖 ros2 命令） =====

function runRos2(args: string[]): Promise<string> {
  return execFileAsync('ros2', args, { maxBuffer: 4 * 1024 * 1024 })
    .then((r) => r.stdout)
    .catch((e: any) => {
      throw new Error(`ros2 ${args.join(' ')} 执行失败: ${e?.stderr || e?.message || e}`)
    })
}

export async function listNodes(): Promise<string[]> {
  const out = await runRos2(['node', 'list'])
  return out.split('\n').map((s) => s.trim()).filter(Boolean)
}

export async function listTopics(withTypes = true): Promise<Array<{ name: string; types: string[] }>> {
  const out = await runRos2(['topic', 'list', ...(withTypes ? ['-t'] : [])])
  const topics: Array<{ name: string; types: string[] }> = []
  for (const line of out.split('\n')) {
    const m = line.match(/^([^\s]+)\s*(?:\[([^\]]+)\])?\s*$/)
    if (m && m[1].startsWith('/')) {
      topics.push({ name: m[1], types: m[2] ? m[2].split(',').map((s) => s.trim()) : [] })
    }
  }
  return topics
}

export async function listServices(): Promise<string[]> {
  const out = await runRos2(['service', 'list'])
  return out.split('\n').map((s) => s.trim()).filter(Boolean)
}

export async function listActions(): Promise<string[]> {
  const out = await runRos2(['action', 'list'])
  return out.split('\n').map((s) => s.trim()).filter(Boolean)
}

export async function getTopicType(topic: string): Promise<string[]> {
  const out = await runRos2(['topic', 'type', topic])
  return out.split('\n').map((s) => s.trim()).filter(Boolean)
}

export async function publishOnce(topic: string, type: string, message: Record<string, unknown>): Promise<void> {
  const json = JSON.stringify(message)
  await runRos2(['topic', 'pub', '--once', topic, type, json])
}

export async function callService(
  service: string, type: string, request: Record<string, unknown>,
): Promise<string> {
  return await runRos2(['service', 'call', service, type, JSON.stringify(request)])
}

// ===== rclnodejs 原生策略（Linux / 已安装 ROS2 的 macOS） =====

export async function initRclnodejs(): Promise<any> {
  const rclnodejs = await import('rclnodejs')
  if (!rclnodejs.isInitialized?.()) {
    await rclnodejs.init()
  }
  return rclnodejs
}

export async function createNode(name = 'smart_plc_studio'): Promise<{ node: any; rclnodejs: any }> {
  const rclnodejs = await initRclnodejs()
  const node = new rclnodejs.Node(name)
  return { node, rclnodejs }
}

// ===== 远程能力桥（rclnodejs/web rosocket，机器人电脑上运行） =====

export async function bridgeRequest(bridgeUrl: string, path: string, body?: unknown): Promise<any> {
  const url = `${bridgeUrl.replace(/\/$/, '')}/${path.replace(/^\//, '')}`
  const res = await fetch(url, {
    method: body !== undefined ? 'POST' : 'GET',
    headers: body !== undefined ? { 'Content-Type': 'application/json' } : undefined,
    body: body !== undefined ? JSON.stringify(body) : undefined,
  })
  if (!res.ok) {
    throw new Error(`桥接请求失败 ${res.status}: ${await res.text()}`)
  }
  return await res.json()
}
