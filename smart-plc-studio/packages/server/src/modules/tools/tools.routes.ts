import type { FastifyInstance } from 'fastify'
import {
  listPorts, openPort, closePort, writeToPort, setDtr, setRts, isPortOpen,
} from './serial.manager.js'
import {
  tcpConnect, tcpDisconnect, tcpSend, tcpCreateServer, tcpCloseServer,
  tcpServerSend, tcpServerBroadcast,
} from './tcp.manager.js'
import {
  udpCreateClient, udpClientBind, udpClientSend, udpClientClose,
  udpCreateServer, udpServerSend, udpServerClose,
} from './udp.manager.js'
import {
  wsConnect, wsSend, wsDisconnect, wsCreateServer, wsCloseServer,
} from './ws.manager.js'

export async function createToolsRoutes(fastify: FastifyInstance) {
  // ===== 串口 API =====
  fastify.get('/api/tools/serial/ports', async () => {
    const data = await listPorts()
    return { success: true, data }
  })

  fastify.post('/api/tools/serial/open', async (req) => {
    return await openPort(req.body as any)
  })

  fastify.post('/api/tools/serial/close', async (req) => {
    const { path } = req.body as any
    return await closePort(path)
  })

  fastify.post('/api/tools/serial/write', async (req) => {
    const { path, data } = req.body as any
    return await writeToPort(path, data)
  })

  fastify.post('/api/tools/serial/dtr', async (req) => {
    const { path, value } = req.body as any
    return await setDtr(path, value)
  })

  fastify.post('/api/tools/serial/rts', async (req) => {
    const { path, value } = req.body as any
    return await setRts(path, value)
  })

  fastify.post('/api/tools/serial/is-open', async (req) => {
    const { path } = req.body as any
    return { success: true, open: isPortOpen(path) }
  })

  // ===== TCP API =====
  fastify.post('/api/tools/tcp/connect', async (req) => {
    const { id, host, port } = req.body as any
    return await tcpConnect(id, host, port)
  })

  fastify.post('/api/tools/tcp/disconnect', async (req) => {
    const { id } = req.body as any
    return tcpDisconnect(id)
  })

  fastify.post('/api/tools/tcp/send', async (req) => {
    const { id, data } = req.body as any
    return await tcpSend(id, data)
  })

  fastify.post('/api/tools/tcp/create-server', async (req) => {
    const { id, port, host } = req.body as any
    return await tcpCreateServer(id, port, host)
  })

  fastify.post('/api/tools/tcp/close-server', async (req) => {
    const { id } = req.body as any
    return tcpCloseServer(id)
  })

  fastify.post('/api/tools/tcp/server-send', async (req) => {
    const { serverId, clientId, data } = req.body as any
    return await tcpServerSend(serverId, clientId, data)
  })

  fastify.post('/api/tools/tcp/server-broadcast', async (req) => {
    const { serverId, data } = req.body as any
    return tcpServerBroadcast(serverId, data)
  })

  // ===== UDP API =====
  fastify.post('/api/tools/udp/create-client', async (req) => {
    const { id } = req.body as any
    return udpCreateClient(id)
  })

  fastify.post('/api/tools/udp/client-bind', async (req) => {
    const { id, port, address } = req.body as any
    return await udpClientBind(id, port, address)
  })

  fastify.post('/api/tools/udp/client-send', async (req) => {
    const { id, data, remotePort, remoteAddress } = req.body as any
    return await udpClientSend(id, data, remotePort, remoteAddress)
  })

  fastify.post('/api/tools/udp/client-close', async (req) => {
    const { id } = req.body as any
    return udpClientClose(id)
  })

  fastify.post('/api/tools/udp/create-server', async (req) => {
    const { id, port, address } = req.body as any
    return await udpCreateServer(id, port, address)
  })

  fastify.post('/api/tools/udp/server-send', async (req) => {
    const { id, data, remotePort, remoteAddress } = req.body as any
    return await udpServerSend(id, data, remotePort, remoteAddress)
  })

  fastify.post('/api/tools/udp/server-close', async (req) => {
    const { id } = req.body as any
    return udpServerClose(id)
  })

  // ===== WebSocket API =====
  fastify.post('/api/tools/ws/connect', async (req) => {
    const { id, url } = req.body as any
    return await wsConnect(id, url)
  })

  fastify.post('/api/tools/ws/send', async (req) => {
    const { id, data } = req.body as any
    return await wsSend(id, data)
  })

  fastify.post('/api/tools/ws/disconnect', async (req) => {
    const { id } = req.body as any
    return wsDisconnect(id)
  })

  fastify.post('/api/tools/ws/create-server', async (req) => {
    const { id, port } = req.body as any
    return await wsCreateServer(id, port)
  })

  fastify.post('/api/tools/ws/close-server', async (req) => {
    const { id } = req.body as any
    return wsCloseServer(id)
  })
}
