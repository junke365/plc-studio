import type { FastifyInstance } from 'fastify'
import type { RuntimeService } from '../runtime/runtime.service.js'
import type { Breakpoint } from '@smart-plc/shared'
import type { SocketStream } from '@fastify/websocket'
import type { WebSocket } from 'ws'
import { DebugTransportService } from './debug-transport.service.js'

export function createDebugGateway(fastify: FastifyInstance, runtime: RuntimeService) {
  fastify.get('/ws/debug', { websocket: true }, (connection: SocketStream, req) => {
    const url = new URL(req.url, `http://${req.headers.host}`)
    const comPort = url.searchParams.get('port')
    const baudStr = url.searchParams.get('baud')
    const useUart = !!(comPort && baudStr)

    let transport: DebugTransportService | null = null

    if (useUart) {
      transport = new DebugTransportService()
      const baud = parseInt(baudStr, 10)
      transport.connect(comPort!, isNaN(baud) ? 115200 : baud)
        .then(() => {
          connection.socket.send(JSON.stringify({
            event: 'log:message',
            payload: { level: 'info', message: `串口 ${comPort} 已连接` }
          }))
        })
        .catch((err) => {
          connection.socket.send(JSON.stringify({
            event: 'log:message',
            payload: { level: 'error', message: `串口连接失败: ${err.message}` }
          }))
        })

      transport.on({
        onStatus: (status) => {
          connection.socket.send(JSON.stringify({ event: 'runtime:status', payload: status }))
        },
        onVarValue: (path, value) => {
          connection.socket.send(JSON.stringify({ event: 'debug:variable:update', payload: { path, value, quality: 'good' } }))
        },
        onVarWritten: (path, success) => {
          connection.socket.send(JSON.stringify({ event: 'variable:update', payload: { path, success } }))
        },
        onBpHit: (id) => {
          connection.socket.send(JSON.stringify({ event: 'debug:breakpoint', payload: { action: 'hit', id } }))
        },
        onStepped: () => {
          connection.socket.send(JSON.stringify({ event: 'stepped', payload: {} }))
        },
        onLog: (level, message) => {
          connection.socket.send(JSON.stringify({ event: 'log:message', payload: { level, message } }))
        },
        onError: (message) => {
          connection.socket.send(JSON.stringify({ event: 'error', payload: { message } }))
        },
        onDisconnected: () => {
          connection.socket.send(JSON.stringify({
            event: 'log:message',
            payload: { level: 'warning', message: '串口已断开' }
          }))
        },
      })
    }

    console.log(`调试 WebSocket 已连接 (mode=${useUart ? 'UART' : 'simulation'})`)

    connection.socket.on('message', (message) => {
      try {
        const data = JSON.parse(message.toString())
        handleDebugMessage(connection.socket, data, runtime, transport)
      } catch {
        connection.socket.send(JSON.stringify({
          event: 'error',
          payload: { message: '无效的消息格式' }
        }))
      }
    })

    connection.socket.on('close', () => {
      if (transport) transport.disconnect()
      console.log('调试 WebSocket 已断开')
    })
  })
}

async function handleDebugMessage(
  ws: WebSocket,
  data: { event: string; payload: Record<string, unknown> },
  runtime: RuntimeService,
  transport: DebugTransportService | null
) {
  /* UART 模式：通过串口转发到 STM32 */
  if (transport && transport.connected) {
    switch (data.event) {
      case 'start':
      case 'run':
        await transport.run()
        break
      case 'stop':
        await transport.pause()
        break
      case 'step':
        await transport.step()
        break
      case 'pause':
        await transport.pause()
        break
      case 'resume':
        await transport.run()
        break
      case 'readVariable':
        await transport.readVar(data.payload?.path as string)
        break
      case 'writeVariable':
        await transport.writeVar(data.payload?.path as string, data.payload?.value)
        break
      case 'readAllVariables':
        await transport.getStatus()
        break
      case 'addBreakpoint': {
        const bp = data.payload as unknown as Breakpoint
        await transport.setBreakpoint(bp.id, bp.path, bp.line)
        break
      }
      case 'removeBreakpoint':
        await transport.removeBreakpoint(data.payload?.id as string)
        break
      case 'toggleBreakpoint':
        await transport.removeBreakpoint(data.payload?.id as string)
        break
      case 'getStatus':
        await transport.getStatus()
        break
      default:
        ws.send(JSON.stringify({
          event: 'error',
          payload: { message: `未知事件: ${data.event}` }
        }))
    }
    return
  }

  /* 仿真模式：使用 RuntimeService 内存仿真 */
  switch (data.event) {
    case 'start':
      runtime.start((data.payload?.projectPath as string) || '').then(() => {
        ws.send(JSON.stringify({ event: 'started', payload: runtime.getStatus() }))
      })
      break

    case 'stop':
      runtime.stop()
      ws.send(JSON.stringify({ event: 'stopped', payload: runtime.getStatus() }))
      break

    case 'step':
      ws.send(JSON.stringify({ event: 'stepped', payload: { success: true } }))
      break

    case 'pause':
      runtime.pause()
      ws.send(JSON.stringify({ event: 'paused', payload: runtime.getStatus() }))
      break

    case 'resume':
      runtime.resume()
      ws.send(JSON.stringify({ event: 'resumed', payload: runtime.getStatus() }))
      break

    case 'readVariable': {
      const path = data.payload?.path as string
      runtime.readVariable(path).then((value) => {
        ws.send(JSON.stringify({
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
        ws.send(JSON.stringify({
          event: 'variable:update',
          payload: { path, value, success: true }
        }))
      })
      break
    }

    case 'readAllVariables':
      runtime.readAllVariables().then((vars) => {
        ws.send(JSON.stringify({
          event: 'debug:variable:update',
          payload: { variables: vars }
        }))
      })
      break

    case 'addBreakpoint': {
      const bp = data.payload as unknown as Breakpoint
      runtime.addBreakpoint(bp)
      ws.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'added', breakpoint: bp }
      }))
      break
    }

    case 'removeBreakpoint': {
      const id = data.payload?.id as string
      runtime.removeBreakpoint(id)
      ws.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'removed', id }
      }))
      break
    }

    case 'toggleBreakpoint': {
      const id = data.payload?.id as string
      const bp = runtime.toggleBreakpoint(id)
      ws.send(JSON.stringify({
        event: 'debug:breakpoint',
        payload: { action: 'toggled', breakpoint: bp }
      }))
      break
    }

    case 'getStatus':
      ws.send(JSON.stringify({
        event: 'runtime:status',
        payload: runtime.getStatus()
      }))
      break

    default:
      ws.send(JSON.stringify({
        event: 'error',
        payload: { message: `未知事件: ${data.event}` }
      }))
  }
}