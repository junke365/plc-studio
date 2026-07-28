import * as net from 'net'

type EventHandler = (event: string, data: any) => void

const tcpClients = new Map<string, { socket: net.Socket; id: string }>()
const tcpServers = new Map<string, { server: net.Server; clients: Map<string, net.Socket>; id: string }>()

let eventHandler: EventHandler | null = null

export function setTcpEventHandler(handler: EventHandler) {
  eventHandler = handler
}

function emit(event: string, data: any) {
  eventHandler?.(event, data)
}

export function tcpConnect(id: string, host: string, port: number): Promise<{ success: boolean; error?: string }> {
  if (tcpClients.has(id)) {
    tcpClients.get(id)!.socket.destroy()
    tcpClients.delete(id)
  }

  const socket = new net.Socket()

  return new Promise((resolve) => {
    socket.connect(port, host, () => {
      tcpClients.set(id, { socket, id })
      resolve({ success: true })
    })

    socket.on('data', (data: Buffer) => {
      emit('tcp:data', { id, data: Array.from(data) })
    })

    socket.on('close', () => {
      tcpClients.delete(id)
      emit('tcp:closed', { id })
    })

    socket.on('error', (err: Error) => {
      emit('tcp:error', { id, error: err.message })
      resolve({ success: false, error: err.message })
    })

    setTimeout(() => {
      if (!tcpClients.has(id)) {
        socket.destroy()
        resolve({ success: false, error: '连接超时' })
      }
    }, 5000)
  })
}

export function tcpDisconnect(id: string) {
  const client = tcpClients.get(id)
  if (client) {
    client.socket.destroy()
    tcpClients.delete(id)
  }
  return { success: true }
}

export function tcpSend(id: string, data: number[]): Promise<{ success: boolean; error?: string }> {
  const client = tcpClients.get(id)
  if (!client) return Promise.resolve({ success: false, error: '未连接' })
  return new Promise((resolve) => {
    client.socket.write(Buffer.from(data), (err?: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export function tcpCreateServer(id: string, port: number, host?: string): Promise<{ success: boolean; error?: string }> {
  if (tcpServers.has(id)) {
    for (const [, c] of tcpServers.get(id)!.clients) c.destroy()
    tcpServers.get(id)!.server.close()
    tcpServers.delete(id)
  }

  const server = net.createServer()
  const clients = new Map<string, net.Socket>()

  return new Promise((resolve) => {
    server.on('connection', (socket) => {
      const clientId = `${id}_client_${Date.now()}`
      clients.set(clientId, socket)
      emit('tcp:server-client-connected', {
        serverId: id, clientId,
        remoteAddress: socket.remoteAddress,
        remotePort: socket.remotePort,
      })
      socket.on('data', (data: Buffer) => {
        emit('tcp:server-data', {
          serverId: id, clientId,
          remoteAddress: `${socket.remoteAddress}:${socket.remotePort}`,
          data: Array.from(data),
        })
      })
      socket.on('close', () => {
        clients.delete(clientId)
        emit('tcp:server-client-disconnected', { serverId: id, clientId })
      })
      socket.on('error', () => clients.delete(clientId))
    })

    server.listen(port, host ?? '0.0.0.0', () => {
      tcpServers.set(id, { server, clients, id })
      resolve({ success: true })
    })

    server.on('error', (err: Error) => {
      tcpServers.delete(id)
      resolve({ success: false, error: err.message })
    })
  })
}

export function tcpCloseServer(id: string) {
  const s = tcpServers.get(id)
  if (s) {
    for (const [, c] of s.clients) c.destroy()
    s.server.close()
    tcpServers.delete(id)
  }
  return { success: true }
}

export function tcpServerSend(serverId: string, clientId: string, data: number[]): Promise<{ success: boolean; error?: string }> {
  const s = tcpServers.get(serverId)
  if (!s) return Promise.resolve({ success: false, error: '服务器未运行' })
  const c = s.clients.get(clientId)
  if (!c) return Promise.resolve({ success: false, error: '客户端未连接' })
  return new Promise((resolve) => {
    c.write(Buffer.from(data), (err?: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export function tcpServerBroadcast(serverId: string, data: number[]) {
  const s = tcpServers.get(serverId)
  if (!s) return { success: false, error: '服务器未运行' }
  const buf = Buffer.from(data)
  for (const [, c] of s.clients) c.write(buf)
  return { success: true }
}

export function closeAllTcp() {
  for (const [, c] of tcpClients) c.socket.destroy()
  tcpClients.clear()
  for (const [, s] of tcpServers) {
    for (const [, c] of s.clients) c.destroy()
    s.server.close()
  }
  tcpServers.clear()
}
