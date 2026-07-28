import { contextBridge, ipcRenderer } from 'electron';

export interface ElectronSerialAPI {
  listPorts: () => Promise<any>;
  open: (options: any) => Promise<any>;
  close: (path: string) => Promise<any>;
  write: (path: string, data: number[]) => Promise<any>;
  setDtr: (path: string, value: boolean) => Promise<any>;
  setRts: (path: string, value: boolean) => Promise<any>;
  isOpen: (path: string) => Promise<boolean>;
  onData: (callback: (data: { port: string; data: number[] }) => void) => () => void;
  onStatus: (callback: (data: { port: string; connected: boolean }) => void) => () => void;
  onError: (callback: (data: { port: string; error: string }) => void) => () => void;
}

export interface ElectronTcpAPI {
  connect: (id: string, host: string, port: number) => Promise<any>;
  disconnect: (id: string) => Promise<any>;
  send: (id: string, data: number[]) => Promise<any>;
  createServer: (id: string, port: number, host?: string) => Promise<any>;
  closeServer: (id: string) => Promise<any>;
  serverSend: (serverId: string, clientId: string, data: number[]) => Promise<any>;
  serverBroadcast: (serverId: string, data: number[]) => Promise<any>;
  onData: (callback: (data: { id: string; data: number[] }) => void) => () => void;
  onClosed: (callback: (data: { id: string }) => void) => () => void;
  onError: (callback: (data: { id: string; error: string }) => void) => () => void;
  onServerData: (callback: (data: { serverId: string; clientId: string; remoteAddress: string; data: number[] }) => void) => () => void;
  onServerClientConnected: (callback: (data: { serverId: string; clientId: string; remoteAddress: string; remotePort: number }) => void) => () => void;
  onServerClientDisconnected: (callback: (data: { serverId: string; clientId: string }) => void) => () => void;
}

export interface ElectronUdpAPI {
  createClient: (id: string) => Promise<any>;
  clientBind: (id: string, port: number, address?: string) => Promise<any>;
  clientSend: (id: string, data: number[], remotePort: number, remoteAddress?: string) => Promise<any>;
  clientClose: (id: string) => Promise<any>;
  createServer: (id: string, port: number, address?: string) => Promise<any>;
  serverSend: (id: string, data: number[], remotePort: number, remoteAddress: string) => Promise<any>;
  serverClose: (id: string) => Promise<any>;
  onClientData: (callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void) => () => void;
  onClientError: (callback: (data: { id: string; error: string }) => void) => () => void;
  onClientClosed: (callback: (data: { id: string }) => void) => () => void;
  onServerData: (callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void) => () => void;
  onServerError: (callback: (data: { id: string; error: string }) => void) => () => void;
}

export interface ElectronWsAPI {
  connect: (id: string, url: string) => Promise<any>;
  send: (id: string, data: string | number[]) => Promise<any>;
  disconnect: (id: string) => Promise<any>;
  createServer: (id: string, port: number) => Promise<any>;
  closeServer: (id: string) => Promise<any>;
  onConnected: (callback: (data: { id: string }) => void) => () => void;
  onDisconnected: (callback: (data: { id: string; code: number; reason?: string }) => void) => () => void;
  onMessage: (callback: (data: { id: string; data: string | number[] }) => void) => () => void;
  onError: (callback: (data: { id: string; error: string }) => void) => () => void;
}

function makeListener<T>(channel: string, callback: (data: T) => void): () => void {
  const handler = (_event: any, data: T) => callback(data);
  ipcRenderer.on(channel, handler);
  return () => ipcRenderer.removeListener(channel, handler);
}

const serialAPI: ElectronSerialAPI = {
  listPorts: () => ipcRenderer.invoke('serial:list-ports'),
  open: (options) => ipcRenderer.invoke('serial:open', options),
  close: (path) => ipcRenderer.invoke('serial:close', path),
  write: (path, data) => ipcRenderer.invoke('serial:write', path, data),
  setDtr: (path, value) => ipcRenderer.invoke('serial:set-dtr', path, value),
  setRts: (path, value) => ipcRenderer.invoke('serial:set-rts', path, value),
  isOpen: (path) => ipcRenderer.invoke('serial:is-open', path),
  onData: (cb) => makeListener('serial:data', cb),
  onStatus: (cb) => makeListener('serial:status', cb),
  onError: (cb) => makeListener('serial:error', cb),
};

const tcpAPI: ElectronTcpAPI = {
  connect: (id, host, port) => ipcRenderer.invoke('tcp:connect', id, host, port),
  disconnect: (id) => ipcRenderer.invoke('tcp:disconnect', id),
  send: (id, data) => ipcRenderer.invoke('tcp:send', id, data),
  createServer: (id, port, host) => ipcRenderer.invoke('tcp:create-server', id, port, host),
  closeServer: (id) => ipcRenderer.invoke('tcp:close-server', id),
  serverSend: (serverId, clientId, data) => ipcRenderer.invoke('tcp:server-send', serverId, clientId, data),
  serverBroadcast: (serverId, data) => ipcRenderer.invoke('tcp:server-broadcast', serverId, data),
  onData: (cb) => makeListener('tcp:data', cb),
  onClosed: (cb) => makeListener('tcp:closed', cb),
  onError: (cb) => makeListener('tcp:error', cb),
  onServerData: (cb) => makeListener('tcp:server-data', cb),
  onServerClientConnected: (cb) => makeListener('tcp:server-client-connected', cb),
  onServerClientDisconnected: (cb) => makeListener('tcp:server-client-disconnected', cb),
};

const udpAPI: ElectronUdpAPI = {
  createClient: (id) => ipcRenderer.invoke('udp:create-client', id),
  clientBind: (id, port, address) => ipcRenderer.invoke('udp:client-bind', id, port, address),
  clientSend: (id, data, remotePort, remoteAddress) => ipcRenderer.invoke('udp:client-send', id, data, remotePort, remoteAddress),
  clientClose: (id) => ipcRenderer.invoke('udp:client-close', id),
  createServer: (id, port, address) => ipcRenderer.invoke('udp:create-server', id, port, address),
  serverSend: (id, data, remotePort, remoteAddress) => ipcRenderer.invoke('udp:server-send', id, data, remotePort, remoteAddress),
  serverClose: (id) => ipcRenderer.invoke('udp:server-close', id),
  onClientData: (cb) => makeListener('udp:client-data', cb),
  onClientError: (cb) => makeListener('udp:client-error', cb),
  onClientClosed: (cb) => makeListener('udp:client-closed', cb),
  onServerData: (cb) => makeListener('udp:server-data', cb),
  onServerError: (cb) => makeListener('udp:server-error', cb),
};

const wsAPI: ElectronWsAPI = {
  connect: (id, url) => ipcRenderer.invoke('ws:connect', id, url),
  send: (id, data) => ipcRenderer.invoke('ws:send', id, data),
  disconnect: (id) => ipcRenderer.invoke('ws:disconnect', id),
  createServer: (id, port) => ipcRenderer.invoke('ws:create-server', id, port),
  closeServer: (id) => ipcRenderer.invoke('ws:close-server', id),
  onConnected: (cb) => makeListener('ws:connected', cb),
  onDisconnected: (cb) => makeListener('ws:disconnected', cb),
  onMessage: (cb) => makeListener('ws:message', cb),
  onError: (cb) => makeListener('ws:error', cb),
};

contextBridge.exposeInMainWorld('electronAPI', {
  serial: serialAPI,
  tcp: tcpAPI,
  udp: udpAPI,
  ws: wsAPI,
});
