import type { FastifyInstance } from 'fastify'
import type { RuntimeService } from '../runtime/runtime.service.js'
import type { Breakpoint, WsEvent } from '@smart-plc/shared'

export function createDebugGateway(fastify: FastifyInstance, runtime: RuntimeService) {
  fastify.get('/ws/debug', { websocket: true }, (socket) => {
    console.log('调试 WebSocket 已连接')

    socket.on('message', (message) => {
      try {
        const data = JSON.parse(message.toString())
        handleDebugMessage(socket, data, runtime)
      } catch {
        socket.send(JSON.stringify({
          event: 'error',
          payload: { message: '无效的消息格式' }
        }))
      }
    })

    socket.on('close', () => {
      console.log('调试 WebSocket 已断开')
    })
  })
}

function handleDebugMessage(
  socket: WebSocket,
  data: { event: string; payload: Record<string, unknown> },
  runtime: RuntimeService
) {
  switch (data.event) {
    case 'start':
      runtime.start((data.payload?.projectPath as string) || '').then(() => {
        socket.send(JSON.stringify({ event: 'started', payload: runtime.getStatus() }))
      })
      break

    case 'stop':
      runtime.stop()
      socket.send(JSON.stringify({ event: 'stopped', payload: runtime.getStatus() }))
      break

    case 'step':
      socket.send(JSON.stringify({ event: 'stepped', payload: { success: true } }))
      break

    case 'pause':
      runtime.pause()
      socket.send(JSON.stringify({ event: 'paused', payload: runtime.getStatus() }))
      break

    case 'resume':
      runtime.resume()
      socket.send(JSON.stringify({ event: 'resumed', payload: runtime.getStatus() }))
      break

    case 'readVariable': {
      const path = data.payload?.path as string
      runtime.readVariable(path).then((value) => {
        socket.send(JSON.stringify({
          event: 'debug:variable:update',
          payload: { path, value, quality: 'good' }
        }))
      })
      break
    }

    case 'writeVariable': {
      const path = data.payload?.path as string
      const value = data.payload?.value
      runtime.writeVariable(path, value).then(() => {
        socket.send(JSON.stringify({
          event: 'variable:update',
          payload: { path, value, success: true }
        }))
      })
      break
    }

    case 'readAllVariables':
      runtime.readAllVariables().then((vars) => {
        socket.send(JSON.stringify({
          event: 'debug:variable:update',
          payload: { variables: vars }
        }))
      })
      break

    case 'addBreakpoint': {
      const bp = data.payload as unknown as Breakpoint
      runtime.addBreakpoint(bp)
      socket.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'added', breakpoint: bp }
      }))
      break
    }

    case 'removeBreakpoint': {
      const id = data.payload?.id as string
      runtime.removeBreakpoint(id)
      socket.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'removed', id }
      }))
      break
    }

    case 'toggleBreakpoint': {
      const id = data.payload?.id as string
      const bp = runtime.toggleBreakpoint(id)
      socket.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'toggled', breakpoint: bp }
      }))
      break
    }

    case 'getStatus':
      socket.send(JSON.stringify({
        event: 'runtime:status',
        payload: runtime.getStatus()
      }))
      break

    default:
      socket.send(JSON.stringify({
        event: 'error',
        payload: { message: `未知事件: ${data.event}` }
      }))
  }
}
