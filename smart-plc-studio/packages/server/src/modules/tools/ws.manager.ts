import WebSocket from 'ws'

type EventHandler = (event: string, data: any) => void

const wsClients = new Map<string, WebSocket>()
const wsServers = new Map<string, WebSocket.Server>()

let eventHandler: EventHandler | null = null

export function setWsEventHandler(handler: EventHandler) {
  eventHandler = handler
}

function emit(event: string, data: any) {
  eventHandler?.(event, data)
}

export function wsConnect(id: string, url: string): Promise<{ success: boolean; error?: string }> {
  if (wsClients.has(id)) {
    wsClients.get(id)!.close()
    wsClients.delete(id)
  }

  const ws = new WebSocket(url)

  return new Promise((resolve) => {
    ws.on('open', () => {
      wsClients.set(id, ws)
      emit('ws:connected', { id })
      resolve({ success: true })
    })

    ws.on('message', (data: WebSocket.Data) => {
      const msg = typeof data === 'string' ? data : Array.from(data as Buffer)
      emit('ws:message', { id, data: msg })
    })

    ws.on('close', (code: number, reason?: Buffer) => {
      wsClients.delete(id)
      emit('ws:disconnected', { id, code, reason: reason?.toString() })
    })

    ws.on('error', (err: Error) => {
      emit('ws:error', { id, error: err.message })
      resolve({ success: false, error: err.message })
    })

    setTimeout(() => {
      if (!wsClients.has(id)) {
        ws.close()
        resolve({ success: false, error: '连接超时' })
      }
    }, 5000)
  })
}

export function wsSend(id: string, data: string): Promise<{ success: boolean; error?: string }> {
  const ws = wsClients.get(id)
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    return Promise.resolve({ success: false, error: '未连接' })
  }
  try {
    ws.send(data)
    return Promise.resolve({ success: true })
  } catch (err: any) {
    return Promise.resolve({ success: false, error: err.message })
  }
}

export function wsDisconnect(id: string) {
  const ws = wsClients.get(id)
  if (ws) {
    ws.close()
    wsClients.delete(id)
  }
  return { success: true }
}

export function wsCreateServer(id: string, port: number): Promise<{ success: boolean; port?: number; error?: string }> {
  if (wsServers.has(id)) {
    wsServers.get(id)!.close()
    wsServers.delete(id)
  }

  const server = new WebSocket.Server({ port })

  return new Promise((resolve) => {
    server.on('listening', () => {
      wsServers.set(id, server)
      resolve({ success: true, port })
    })

    server.on('connection', (ws, req) => {
      const clientId = `client_${Date.now()}`
      const ip = req.socket.remoteAddress

      emit('ws:server-client-connected', { serverId: id, clientId, ip })

      ws.on('message', (data) => {
        const msg = typeof data === 'string' ? data : Array.from(data as Buffer)
        emit('ws:server-data', { serverId: id, clientId, data: msg })
      })
      ws.on('close', () => {
        emit('ws:server-client-disconnected', { serverId: id, clientId })
      })
    })

    server.on('error', (err: Error) => {
      wsServers.delete(id)
      resolve({ success: false, error: err.message })
    })
  })
}

export function wsCloseServer(id: string) {
  const server = wsServers.get(id)
  if (server) {
    server.close()
    wsServers.delete(id)
  }
  return { success: true }
}

export function closeAllWebSocket() {
  for (const [, ws] of wsClients) ws.close()
  wsClients.clear()
  for (const [, server] of wsServers) server.close()
  wsServers.clear()
}
