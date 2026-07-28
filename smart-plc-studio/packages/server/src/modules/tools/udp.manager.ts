import * as dgram from 'dgram'

type EventHandler = (event: string, data: any) => void

const udpClients = new Map<string, { socket: dgram.Socket; id: string }>()
const udpServers = new Map<string, { socket: dgram.Socket; id: string }>()

let eventHandler: EventHandler | null = null

export function setUdpEventHandler(handler: EventHandler) {
  eventHandler = handler
}

function emit(event: string, data: any) {
  eventHandler?.(event, data)
}

export function udpCreateClient(id: string) {
  if (udpClients.has(id)) {
    udpClients.get(id)!.socket.close()
    udpClients.delete(id)
  }

  const socket = dgram.createSocket('udp4')

  socket.on('message', (msg: Buffer, rinfo: dgram.RemoteInfo) => {
    emit('udp:client-data', {
      id, data: Array.from(msg),
      remoteAddress: rinfo.address, remotePort: rinfo.port,
    })
  })
  socket.on('error', (err: Error) => {
    emit('udp:client-error', { id, error: err.message })
  })
  socket.on('close', () => {
    udpClients.delete(id)
    emit('udp:client-closed', { id })
  })

  udpClients.set(id, { socket, id })
  return { success: true }
}

export function udpClientBind(id: string, port: number, address?: string): Promise<{ success: boolean; address?: string; port?: number; error?: string }> {
  const client = udpClients.get(id)
  if (!client) return Promise.resolve({ success: false, error: '客户端未创建' })
  return new Promise((resolve) => {
    const addr = address || '0.0.0.0'
    client.socket.bind(port, addr, () => {
      const addr = client.socket.address()
      resolve({ success: true, address: addr.address, port: addr.port })
    })
    client.socket.once('error', (err: Error) => {
      resolve({ success: false, error: err.message })
    })
  })
}

export function udpClientSend(id: string, data: number[], remotePort: number, remoteAddress?: string): Promise<{ success: boolean; error?: string }> {
  const client = udpClients.get(id)
  if (!client) return Promise.resolve({ success: false, error: '客户端未创建' })
  const addr = remoteAddress || '127.0.0.1'
  return new Promise((resolve) => {
    client.socket.send(Buffer.from(data), remotePort, addr, (err: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export function udpClientClose(id: string) {
  const client = udpClients.get(id)
  if (client) {
    client.socket.close()
    udpClients.delete(id)
  }
  return { success: true }
}

export function udpCreateServer(id: string, port: number, address?: string): Promise<{ success: boolean; address?: string; port?: number; error?: string }> {
  if (udpServers.has(id)) {
    udpServers.get(id)!.socket.close()
    udpServers.delete(id)
  }

  const socket = dgram.createSocket('udp4')

  socket.on('message', (msg: Buffer, rinfo: dgram.RemoteInfo) => {
    emit('udp:server-data', {
      id, data: Array.from(msg),
      remoteAddress: rinfo.address, remotePort: rinfo.port,
    })
  })
  socket.on('error', (err: Error) => {
    emit('udp:server-error', { id, error: err.message })
  })
  socket.on('close', () => udpServers.delete(id))

  return new Promise((resolve) => {
    const addr = address || '0.0.0.0'
    socket.bind(port, addr, () => {
      udpServers.set(id, { socket, id })
      const addr = socket.address()
      resolve({ success: true, address: addr.address, port: addr.port })
    })
    socket.once('error', (err: Error) => {
      resolve({ success: false, error: err.message })
    })
  })
}

export function udpServerSend(id: string, data: number[], remotePort: number, remoteAddress: string): Promise<{ success: boolean; error?: string }> {
  const server = udpServers.get(id)
  if (!server) return Promise.resolve({ success: false, error: '服务器未运行' })
  return new Promise((resolve) => {
    server.socket.send(Buffer.from(data), remotePort, remoteAddress, (err: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export function udpServerClose(id: string) {
  const server = udpServers.get(id)
  if (server) {
    server.socket.close()
    udpServers.delete(id)
  }
  return { success: true }
}

export function closeAllUdp() {
  for (const [, c] of udpClients) c.socket.close()
  udpClients.clear()
  for (const [, s] of udpServers) s.socket.close()
  udpServers.clear()
}
