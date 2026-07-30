import type { Breakpoint, DebugVariable } from '@smart-plc/shared'

interface VariableValue {
  value: unknown
  type: string
  quality: 'good' | 'bad' | 'uncertain'
}

export class RuntimeService {
  private static _instance: RuntimeService

  static getInstance(): RuntimeService {
    if (!RuntimeService._instance) {
      RuntimeService._instance = new RuntimeService()
    }
    return RuntimeService._instance
  }

  private _status: string = 'stopped'
  private _projectPath: string = ''
  private _variables: Map<string, VariableValue> = new Map()
  private _breakpoints: Map<string, Breakpoint> = new Map()
  private _taskTime: number = 0
  private _interval: ReturnType<typeof setInterval> | null = null

  getStatus() {
    return {
      status: this._status,
      projectPath: this._projectPath,
      taskTime: this._taskTime,
      variableCount: this._variables.size,
      breakpointCount: this._breakpoints.size
    }
  }

  async start(projectPath: string): Promise<void> {
    this._projectPath = projectPath
    this._status = 'running'

    // 模拟 PLC 周期性扫描：每 10ms 更新一次变量（仿真模式）
    this._interval = setInterval(() => {
      this._taskTime++
      // 模拟变量值变化（用于演示调试监视）
      this._variables.forEach((v, key) => {
        if (v.type === 'BOOL' && typeof v.value === 'boolean') {
          // 保持布尔值不变
        } else if (v.type === 'INT' && typeof v.value === 'number') {
          this._variables.set(key, { ...v, value: v.value + Math.floor(Math.random() * 3) - 1 })
        }
      })
    }, 10)

    console.log(`运行时已启动: ${projectPath}`)
  }

  stop(): void {
    this._status = 'stopped'
    if (this._interval) {
      clearInterval(this._interval)
      this._interval = null
    }
    console.log('运行时已停止')
  }

  pause(): void {
    this._status = 'paused'
    if (this._interval) {
      clearInterval(this._interval)
      this._interval = null
    }
    console.log('运行时已暂停')
  }

  resume(): void {
    this._status = 'running'
    this._interval = setInterval(() => {
      this._taskTime++
    }, 10)
    console.log('运行时已恢复')
  }

  // 变量管理
  setVariable(path: string, value: unknown, type: string): void {
    this._variables.set(path, { value, type, quality: 'good' })
  }

  registerVariables(vars: { path: string; type: string; initialValue?: unknown }[]): void {
    for (const v of vars) {
      if (!this._variables.has(v.path)) {
        this._variables.set(v.path, {
          value: v.initialValue ?? 0,
          type: v.type,
          quality: 'good'
        })
      }
    }
  }

  async readVariable(path: string): Promise<unknown> {
    const v = this._variables.get(path)
    if (v) {
      return v.value
    }
    // TODO: 从 C 运行时（串口/网络）读取真实变量
    return null
  }

  async readAllVariables(): Promise<DebugVariable[]> {
    return Array.from(this._variables.entries()).map(([path, v]) => ({
      path,
      name: path.split('.').pop() || path,
      value: v.value,
      type: v.type,
      forced: false
    }))
  }

  async writeVariable(path: string, value: unknown): Promise<void> {
    const existing = this._variables.get(path)
    if (existing) {
      this._variables.set(path, { ...existing, value })
    }
    // TODO: 通过串口/网络写入 C 运行时
    console.log(`写入变量: ${path} = ${value}`)
  }

  // 断点管理
  addBreakpoint(bp: Breakpoint): void {
    this._breakpoints.set(bp.id, bp)
  }

  removeBreakpoint(id: string): void {
    this._breakpoints.delete(id)
  }

  toggleBreakpoint(id: string): Breakpoint | undefined {
    const bp = this._breakpoints.get(id)
    if (bp) {
      bp.enabled = !bp.enabled
      this._breakpoints.set(id, bp)
    }
    return bp
  }

  getBreakpoints(): Breakpoint[] {
    return Array.from(this._breakpoints.values())
  }

  get status(): string { return this._status }
  get projectPath(): string { return this._projectPath }
  get taskTime(): number { return this._taskTime }
}
