export class RuntimeService {
  private status: string = 'stopped'
  private projectPath: string = ''

  getStatus() {
    return {
      status: this.status,
      projectPath: this.projectPath
    }
  }

  async start(projectPath: string): Promise<void> {
    this.projectPath = projectPath
    this.status = 'running'
    console.log(`运行时已启动: ${projectPath}`)
  }

  stop(): void {
    this.status = 'stopped'
    console.log('运行时已停止')
  }

  pause(): void {
    this.status = 'paused'
    console.log('运行时已暂停')
  }

  resume(): void {
    this.status = 'running'
    console.log('运行时已恢复')
  }

  async readVariable(path: string): Promise<unknown> {
    // TODO: 从 C 运行时读取变量
    return null
  }

  async writeVariable(path: string, value: unknown): Promise<void> {
    // TODO: 写入变量到 C 运行时
    console.log(`写入变量: ${path} = ${value}`)
  }
}
