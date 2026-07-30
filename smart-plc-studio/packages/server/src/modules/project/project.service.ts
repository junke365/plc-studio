import * as fs from 'fs/promises'
import * as path from 'path'

const SAMPLES_DIR = path.resolve(process.cwd(), '../../../projects/samples')

export class ProjectService {
  private projects: Map<string, string> = new Map()

  async listSamples(): Promise<{ name: string; path: string; description: string }[]> {
    const entries = await fs.readdir(SAMPLES_DIR, { withFileTypes: true })
    const samples: { name: string; path: string; description: string }[] = []
    for (const entry of entries) {
      if (!entry.isDirectory()) continue
      const projectFile = path.join(SAMPLES_DIR, entry.name, 'project.json')
      try {
        const content = await fs.readFile(projectFile, 'utf-8')
        const project = JSON.parse(content)
        samples.push({
          name: project.name || entry.name,
          path: entry.name,
          description: project.pous?.[0]?.comment || '',
        })
      } catch { /* skip */ }
    }
    return samples
  }

  async openSample(name: string): Promise<unknown> {
    const projectFile = path.join(SAMPLES_DIR, name, 'project.json')
    const content = await fs.readFile(projectFile, 'utf-8')
    return JSON.parse(content)
  }

  async openProject(projectPath: string): Promise<unknown> {
    const projectFile = path.join(projectPath, 'project.plcproj')

    try {
      const content = await fs.readFile(projectFile, 'utf-8')
      const project = JSON.parse(content)
      this.projects.set(project.name, projectPath)
      return project
    } catch {
      throw new Error(`无法打开项目: ${projectPath}`)
    }
  }

  async saveProject(projectPath: string, project: unknown): Promise<void> {
    const projectFile = path.join(projectPath, 'project.plcproj')

    try {
      await fs.mkdir(path.dirname(projectFile), { recursive: true })
      await fs.writeFile(projectFile, JSON.stringify(project, null, 2))
    } catch {
      throw new Error(`无法保存项目: ${projectPath}`)
    }
  }

  async createProject(projectPath: string, name: string): Promise<unknown> {
    const project = {
      name,
      pous: [],
      dataTypes: [],
      configurations: [],
      fileHeader: '',
      contentHeader: ''
    }

    await this.saveProject(projectPath, project)
    this.projects.set(name, projectPath)
    return project
  }

  async getPous(projectName: string): Promise<unknown[]> {
    const projectPath = this.projects.get(projectName)
    if (!projectPath) {
      throw new Error(`项目不存在: ${projectName}`)
    }

    const projectFile = path.join(projectPath, 'project.plcproj')
    const content = await fs.readFile(projectFile, 'utf-8')
    const project = JSON.parse(content)
    return project.pous || []
  }

  async addPou(projectName: string, name: string, type: string): Promise<void> {
    const projectPath = this.projects.get(projectName)
    if (!projectPath) {
      throw new Error(`项目不存在: ${projectName}`)
    }

    const projectFile = path.join(projectPath, 'project.plcproj')
    const content = await fs.readFile(projectFile, 'utf-8')
    const project = JSON.parse(content)

    const pou = {
      name,
      pouType: type,
      variables: {
        inputVars: [],
        outputVars: [],
        inOutVars: [],
        localVars: [],
        tempVars: [],
        globalVars: [],
        externalVars: [],
        accessVars: []
      },
      body: '',
      bodyLanguage: 'LD'
    }

    project.pous.push(pou)
    await this.saveProject(projectPath, project)
  }

  async removePou(projectName: string, pouName: string): Promise<void> {
    const projectPath = this.projects.get(projectName)
    if (!projectPath) {
      throw new Error(`项目不存在: ${projectName}`)
    }

    const projectFile = path.join(projectPath, 'project.plcproj')
    const content = await fs.readFile(projectFile, 'utf-8')
    const project = JSON.parse(content)

    const index = project.pous.findIndex((p: { name: string }) => p.name === pouName)
    if (index >= 0) {
      project.pous.splice(index, 1)
      await this.saveProject(projectPath, project)
    }
  }
}
