import Fastify, { type FastifyInstance } from 'fastify'
import cors from '@fastify/cors'
import websocket from '@fastify/websocket'
import { createProjectRoutes } from './modules/project/project.routes.js'
import { createBuildRoutes } from './modules/build/build.routes.js'
import { createRuntimeRoutes } from './modules/runtime/runtime.routes.js'
import { RuntimeService } from './modules/runtime/runtime.service.js'
import { createDebugGateway } from './modules/debug/debug.gateway.js'
import { createToolsRoutes } from './modules/tools/tools.routes.js'
import { createToolsGateway } from './modules/tools/tools.gateway.js'
import { createHmiRoutes } from './modules/hmi/hmi.routes.js'
import { createVisionRoutes } from './modules/vision/vision.routes.js'
import { createRos2Routes } from './modules/ros2/ros2.routes.js'
import { createRos2Gateway } from './modules/ros2/ros2.gateway.js'
import { pathToFileURL } from 'url'

export interface ServerOptions {
  port?: number
  host?: string
}

export async function startServer(opts: ServerOptions = {}): Promise<FastifyInstance> {
  const port = opts.port ?? parseInt(process.env.PORT ?? '3000', 10)
  const host = opts.host ?? process.env.HOST ?? '127.0.0.1'

  const server = Fastify({
    logger: true
  })

  // 注册插件
  await server.register(cors, {
    origin: true
  })

  await server.register(websocket)

  // 注册路由
  await server.register(createProjectRoutes, { prefix: '/api/project' })
  await server.register(createBuildRoutes, { prefix: '/api/build' })
  await server.register(createRuntimeRoutes, { prefix: '/api/runtime' })
  await server.register(createToolsRoutes)
  await server.register(createHmiRoutes, { prefix: '/api/hmi' })
  await server.register(createVisionRoutes)
  await server.register(createRos2Routes)

  // 注册 WebSocket（共享 RuntimeService 实例）
  const runtime = RuntimeService.getInstance()
  createDebugGateway(server, runtime)
  createToolsGateway(server)
  createRos2Gateway(server)

  // 健康检查
  server.get('/api/health', async () => {
    return { status: 'ok', timestamp: Date.now() }
  })

  // 启动服务器
  await server.listen({ port, host })
  console.log(`服务器已启动: http://${host}:${port}`)
  return server
}

// 直接运行时自动启动（被 Electron 主进程 import 时不触发）
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  startServer().catch((err) => {
    console.error('服务器启动失败:', err)
    process.exit(1)
  })
}
