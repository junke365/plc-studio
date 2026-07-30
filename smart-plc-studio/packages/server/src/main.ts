import Fastify from 'fastify'
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

// 注册 WebSocket（共享 RuntimeService 实例）
const runtime = RuntimeService.getInstance()
createDebugGateway(server, runtime)
createToolsGateway(server)

// 健康检查
server.get('/api/health', async () => {
  return { status: 'ok', timestamp: Date.now() }
})

// 启动服务器
const start = async () => {
  try {
    const port = parseInt(process.env.PORT || '3000', 10)
    const host = process.env.HOST || '127.0.0.1'

    await server.listen({ port, host })
    console.log(`服务器已启动: http://${host}:${port}`)
  } catch (err) {
    server.log.error(err)
    process.exit(1)
  }
}

start()
