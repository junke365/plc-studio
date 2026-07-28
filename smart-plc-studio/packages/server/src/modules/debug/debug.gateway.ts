import type { FastifyInstance } from 'fastify'

export function createDebugGateway(fastify: FastifyInstance) {
  fastify.get('/ws/debug', { websocket: true }, (socket) => {
    console.log('调试 WebSocket 已连接')

    socket.on('message', (message) => {
      try {
        const data = JSON.parse(message.toString())
        handleDebugMessage(socket, data)
      } catch {
        console.error('无效的消息格式')
      }
    })

    socket.on('close', () => {
      console.log('调试 WebSocket 已断开')
    })
  })
}

function handleDebugMessage(socket: WebSocket, data: { event: string; payload: unknown }) {
  switch (data.event) {
    case 'start':
      // 开始调试
      socket.send(JSON.stringify({ event: 'started', payload: {} }))
      break
    case 'stop':
      // 停止调试
      socket.send(JSON.stringify({ event: 'stopped', payload: {} }))
      break
    case 'step':
      // 单步执行
      socket.send(JSON.stringify({ event: 'stepped', payload: {} }))
      break
    case 'pause':
      // 暂停
      socket.send(JSON.stringify({ event: 'paused', payload: {} }))
      break
    case 'resume':
      // 恢复
      socket.send(JSON.stringify({ event: 'resumed', payload: {} }))
      break
    case 'readVariable':
      // 读取变量
      socket.send(JSON.stringify({
        event: 'variableUpdate',
        payload: { path: data.payload, value: null }
      }))
      break
    default:
      console.log('未知事件:', data.event)
  }
}
