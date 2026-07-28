import type { FastifyInstance } from 'fastify'
import { ProjectService } from './project.service.js'

const projectService = new ProjectService()

export async function createProjectRoutes(fastify: FastifyInstance) {
  // 获取项目列表
  fastify.get('/list', async () => {
    return { projects: [] }
  })

  // 打开项目
  fastify.post<{
    Body: { path: string }
  }>('/open', async (request, reply) => {
    const { path } = request.body
    try {
      const project = await projectService.openProject(path)
      return { success: true, project }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  // 保存项目
  fastify.post<{
    Body: { path: string; project: unknown }
  }>('/save', async (request, reply) => {
    const { path, project } = request.body
    try {
      await projectService.saveProject(path, project)
      return { success: true }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  // 创建新项目
  fastify.post<{
    Body: { path: string; name: string }
  }>('/create', async (request, reply) => {
    const { path, name } = request.body
    try {
      const project = await projectService.createProject(path, name)
      return { success: true, project }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  // 获取 POU 列表
  fastify.get<{
    Params: { projectName: string }
  }>('/:projectName/pous', async (request) => {
    const { projectName } = request.params
    const pous = await projectService.getPous(projectName)
    return { pous }
  })

  // 添加 POU
  fastify.post<{
    Params: { projectName: string }
    Body: { name: string; type: string }
  }>('/:projectName/pou', async (request, reply) => {
    const { projectName } = request.params
    const { name, type } = request.body
    try {
      await projectService.addPou(projectName, name, type)
      return { success: true }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  // 删除 POU
  fastify.delete<{
    Params: { projectName: string; pouName: string }
  }>('/:projectName/pou/:pouName', async (request, reply) => {
    const { projectName, pouName } = request.params
    try {
      await projectService.removePou(projectName, pouName)
      return { success: true }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })
}
