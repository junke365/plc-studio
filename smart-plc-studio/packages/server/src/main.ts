import Fastify from 'fastify'
import cors from '@fastify/cors'
import websocket from '@fastify/websocket'
import { createProjectRoutes } from './modules/project/project.routes.js'
import { createRuntimeRoutes } from './modules/runtime/runtime.routes.js'
import { createDebugGateway } from './modules/debug/debug.gateway.js'

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
await server.register(createRuntimeRoutes, { prefix: '/api/runtime' })

// 注册 WebSocket
createDebugGateway(server)

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
