import type { FastifyInstance } from 'fastify'
import { RuntimeService } from './runtime.service.js'

const runtimeService = new RuntimeService()

export async function createRuntimeRoutes(fastify: FastifyInstance) {
  // 获取运行时状态
  fastify.get('/status', async () => {
    return runtimeService.getStatus()
  })

  // 启动运行时
  fastify.post<{
    Body: { projectPath: string }
  }>('/start', async (request, reply) => {
    const { projectPath } = request.body
    try {
      await runtimeService.start(projectPath)
      return { success: true }
    } catch (error) {
      reply.status(400)
      return { success: false, error: (error as Error).message }
    }
  })

  // 停止运行时
  fastify.post('/stop', async () => {
    runtimeService.stop()
    return { success: true }
  })

  // 暂停运行时
  fastify.post('/pause', async () => {
    runtimeService.pause()
    return { success: true }
  })

  // 恢复运行时
  fastify.post('/resume', async () => {
    runtimeService.resume()
    return { success: true }
  })

  // 读取变量
  fastify.get<{
    Querystring: { path: string }
  }>('/variable', async (request) => {
    const { path } = request.query
    const value = await runtimeService.readVariable(path)
    return { path, value }
  })

  // 写入变量
  fastify.post<{
    Body: { path: string; value: unknown }
  }>('/variable', async (request) => {
    const { path, value } = request.body
    await runtimeService.writeVariable(path, value)
    return { success: true }
  })
}
