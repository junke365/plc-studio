// 开发模式由 Vite 代理转发到后端；Electron 打包后页面以 file:// 加载，
// 此时直接连接本地 Fastify 后端 (127.0.0.1:3000)
const isHttpPage =
  window.location.protocol === "http:" || window.location.protocol === "https:";
const API_ORIGIN = isHttpPage ? "" : "http://127.0.0.1:3000";
const API_BASE = `${API_ORIGIN}/api/tools`;
const WS_ORIGIN = isHttpPage ? `ws://${window.location.host}` : "ws://127.0.0.1:3000";
const WS_URL = `${WS_ORIGIN}/ws/tools`;

// ===== WebSocket 连接管理 =====
let ws: WebSocket | null = null
let wsConnecting = false
let wsOpenResolve: (() => void) | null = null
let wsReconnectTimer: ReturnType<typeof setTimeout> | null = null
const eventListeners = new Map<string, Set<(data: any) => void>>()

function getWs(): WebSocket {
  if (ws && ws.readyState === WebSocket.OPEN) return ws
  if (ws && ws.readyState === WebSocket.CONNECTING && wsConnecting) return ws

  console.log('[WS] 创建 WebSocket 连接: ' + WS_URL)

  wsConnecting = true
  ws = new WebSocket(WS_URL)

  ws.onopen = () => {
    console.log('[WS] 连接成功')
    wsConnecting = false
    if (wsOpenResolve) {
      wsOpenResolve()
      wsOpenResolve = null
    }
  }

  ws.onmessage = (e) => {
    try {
      const { event, data } = JSON.parse(e.data)
      console.log('[WS] 收到事件:', event, data)
      const listeners = eventListeners.get(event)
      if (listeners) {
        for (const cb of listeners) cb(data)
      } else {
        console.warn('[WS] 没有监听器处理事件:', event)
      }
    } catch (err) {
      console.error('[WS] 消息解析失败:', err)
    }
  }

  ws.onclose = (evt) => {
    console.warn('[WS] 连接关闭, code=' + evt.code + ' reason=' + evt.reason)
    ws = null
    wsConnecting = false
    wsOpenResolve = null
    wsReconnectTimer = setTimeout(() => getWs(), 2000)
  }

  ws.onerror = (err) => {
    console.error('[WS] 连接错误:', err)
  }

  return ws
}

/** 等待 WebSocket 连接就绪（最多等 5 秒） */
function waitForWs(): Promise<boolean> {
  return new Promise((resolve) => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      resolve(true)
      return
    }
    const ws2 = getWs()
    if (ws2.readyState === WebSocket.OPEN) {
      resolve(true)
      return
    }
    wsOpenResolve = () => {
      wsOpenResolve = null
      resolve(true)
    }
    // 5 秒超时
    setTimeout(() => {
      if (wsOpenResolve) {
        wsOpenResolve = null
        console.error('[WS] 连接超时')
        resolve(false)
      }
    }, 5000)
  })
}

/** 等待 WS 连接就绪（给外部调用） */
export async function ensureWsConnected(): Promise<boolean> {
  return waitForWs()
}

function onEvent(event: string, callback: (data: any) => void): () => void {
  getWs()
  if (!eventListeners.has(event)) {
    eventListeners.set(event, new Set())
  }
  eventListeners.get(event)!.add(callback)
  return () => { eventListeners.get(event)?.delete(callback) }
}

function offEvent(event: string, callback: (data: any) => void) {
  eventListeners.get(event)?.delete(callback)
}

// ===== HTTP 请求工具 =====
async function api<T = any>(path: string, body?: any): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined,
  })
  return res.json()
}

async function apiGet<T = any>(path: string): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`)
  return res.json()
}

// ===== 串口 API =====
export interface PortInfo {
  path: string
  manufacturer?: string
  serialNumber?: string
  pnpId?: string
  locationId?: string
  vendorId?: string
  productId?: string
}

export interface SerialOptions {
  path: string
  baudRate: number
  dataBits?: 5 | 6 | 7 | 8
  stopBits?: 1 | 1.5 | 2
  parity?: 'none' | 'odd' | 'even' | 'mark' | 'space'
}

export async function serialListPorts(): Promise<PortInfo[]> {
  const result = await apiGet<{ success: boolean; data: PortInfo[] }>('/serial/ports')
  return result.success ? result.data : []
}

export async function serialOpen(options: SerialOptions): Promise<{ success: boolean; error?: string }> {
  return api('/serial/open', options)
}

export async function serialClose(path: string): Promise<{ success: boolean; error?: string }> {
  return api('/serial/close', { path })
}

export async function serialWrite(path: string, data: number[]): Promise<{ success: boolean; error?: string }> {
  return api('/serial/write', { path, data })
}

export async function serialSetDtr(path: string, value: boolean): Promise<{ success: boolean; error?: string }> {
  return api('/serial/dtr', { path, value })
}

export async function serialSetRts(path: string, value: boolean): Promise<{ success: boolean; error?: string }> {
  return api('/serial/rts', { path, value })
}

export function onSerialData(_port: string, callback: (data: { port: string; data: number[] }) => void): () => void {
  return onEvent('serial:data', callback)
}

export function onSerialStatus(callback: (data: { port: string; connected: boolean }) => void): () => void {
  return onEvent('serial:status', callback)
}

export function onSerialError(callback: (data: { port: string; error: string }) => void): () => void {
  return onEvent('serial:error', callback)
}

// ===== TCP API =====
export async function tcpConnect(id: string, host: string, port: number): Promise<{ success: boolean; error?: string }> {
  return api('/tcp/connect', { id, host, port })
}

export async function tcpDisconnect(id: string) {
  return api('/tcp/disconnect', { id })
}

export async function tcpSend(id: string, data: number[]): Promise<{ success: boolean; error?: string }> {
  return api('/tcp/send', { id, data })
}

export async function tcpCreateServer(id: string, port: number, host?: string) {
  return api('/tcp/create-server', { id, port, host })
}

export async function tcpCloseServer(id: string) {
  return api('/tcp/close-server', { id })
}

export async function tcpServerSend(serverId: string, clientId: string, data: number[]) {
  return api('/tcp/server-send', { serverId, clientId, data })
}

export async function tcpServerBroadcast(serverId: string, data: number[]) {
  return api('/tcp/server-broadcast', { serverId, data })
}

export function onTcpData(callback: (data: { id: string; data: number[] }) => void): () => void {
  return onEvent('tcp:data', callback)
}

export function onTcpClosed(callback: (data: { id: string }) => void): () => void {
  return onEvent('tcp:closed', callback)
}

export function onTcpError(callback: (data: { id: string; error: string }) => void): () => void {
  return onEvent('tcp:error', callback)
}

export function onTcpServerData(callback: (data: { serverId: string; clientId: string; remoteAddress: string; data: number[] }) => void): () => void {
  return onEvent('tcp:server-data', callback)
}

export function onTcpServerClientConnected(callback: (data: { serverId: string; clientId: string; remoteAddress: string; remotePort: number }) => void): () => void {
  return onEvent('tcp:server-client-connected', callback)
}

export function onTcpServerClientDisconnected(callback: (data: { serverId: string; clientId: string }) => void): () => void {
  return onEvent('tcp:server-client-disconnected', callback)
}

// ===== UDP API =====
export async function udpCreateClient(id: string) {
  return api('/udp/create-client', { id })
}

export async function udpClientBind(id: string, port: number, address?: string) {
  return api('/udp/client-bind', { id, port, address })
}

export async function udpClientSend(id: string, data: number[], remotePort: number, remoteAddress?: string) {
  return api('/udp/client-send', { id, data, remotePort, remoteAddress })
}

export async function udpClientClose(id: string) {
  return api('/udp/client-close', { id })
}

export async function udpCreateServer(id: string, port: number, address?: string) {
  return api('/udp/create-server', { id, port, address })
}

export async function udpServerSend(id: string, data: number[], remotePort: number, remoteAddress: string) {
  return api('/udp/server-send', { id, data, remotePort, remoteAddress })
}

export async function udpServerClose(id: string) {
  return api('/udp/server-close', { id })
}

export function onUdpClientData(callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void): () => void {
  return onEvent('udp:client-data', callback)
}

export function onUdpClientError(callback: (data: { id: string; error: string }) => void): () => void {
  return onEvent('udp:client-error', callback)
}

export function onUdpClientClosed(callback: (data: { id: string }) => void): () => void {
  return onEvent('udp:client-closed', callback)
}

export function onUdpServerData(callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void): () => void {
  return onEvent('udp:server-data', callback)
}

export function onUdpServerError(callback: (data: { id: string; error: string }) => void): () => void {
  return onEvent('udp:server-error', callback)
}

// ===== WebSocket API =====
export async function wsConnect(id: string, url: string) {
  return api('/ws/connect', { id, url })
}

export async function wsSend(id: string, data: string) {
  return api('/ws/send', { id, data })
}

export async function wsDisconnect(id: string) {
  return api('/ws/disconnect', { id })
}

export async function wsCreateServer(id: string, port: number) {
  return api('/ws/create-server', { id, port })
}

export async function wsCloseServer(id: string) {
  return api('/ws/close-server', { id })
}

export function onWsConnected(callback: (data: { id: string }) => void): () => void {
  return onEvent('ws:connected', callback)
}

export function onWsDisconnected(callback: (data: { id: string; code: number; reason?: string }) => void): () => void {
  return onEvent('ws:disconnected', callback)
}

export function onWsMessage(callback: (data: { id: string; data: string | number[] }) => void): () => void {
  return onEvent('ws:message', callback)
}

export function onWsError(callback: (data: { id: string; error: string }) => void): () => void {
  return onEvent('ws:error', callback)
}
