import type { FastifyInstance } from 'fastify'
import { setSerialEventHandler } from './serial.manager.js'
import { setTcpEventHandler } from './tcp.manager.js'
import { setUdpEventHandler } from './udp.manager.js'
import { setWsEventHandler } from './ws.manager.js'

const clients = new Set<any>()

export function broadcast(event: string, data: any) {
  const msg = JSON.stringify({ event, data })
  let sent = 0
  const dead: any[] = []
  for (const conn of clients) {
    try {
      // conn 是 @fastify/websocket 创建的 Duplex 流，底层 ws 在 conn.socket 上
      const ws = conn.socket || conn
      if (typeof ws.send === 'function') {
        ws.send(msg)
        sent++
      } else if (typeof conn.write === 'function') {
        conn.write(msg)
        sent++
      } else {
        dead.push(conn)
      }
    } catch (err) {
      console.warn(`[Tools WS] 发送失败，移除客户端:`, err)
      try { conn.destroy?.() } catch {}
      dead.push(conn)
    }
  }
  for (const d of dead) clients.delete(d)
  if (event === 'serial:data' || event === 'tcp:server-data') {
    console.log(`[Tools WS] broadcast 事件=${event} 客户端数=${clients.size} 已发送=${sent}`)
  }
}

export function createToolsGateway(fastify: FastifyInstance) {
  setSerialEventHandler(broadcast)
  setTcpEventHandler(broadcast)
  setUdpEventHandler(broadcast)
  setWsEventHandler(broadcast)

  fastify.get('/ws/tools', { websocket: true }, (socket) => {
    clients.add(socket)
    console.log('[Tools WS] 客户端已连接，当前连接数:', clients.size)

    socket.on('message', (message) => {
      try {
        const msg = JSON.parse(message.toString())
        console.log('[Tools WS] 收到消息:', msg.event)
      } catch {
        console.error('[Tools WS] 无效的消息格式')
      }
    })

    socket.on('close', () => {
      clients.delete(socket)
      console.log('[Tools WS] 客户端已断开，当前连接数:', clients.size)
    })
  })
}
