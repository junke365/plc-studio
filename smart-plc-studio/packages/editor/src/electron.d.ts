export interface ElectronSerialAPI {
  listPorts: () => Promise<{ success: boolean; data?: any[]; error?: string }>;
  open: (options: SerialOpenOptions) => Promise<{ success: boolean; error?: string }>;
  close: (path: string) => Promise<{ success: boolean; error?: string }>;
  write: (path: string, data: number[]) => Promise<{ success: boolean; error?: string }>;
  setDtr: (path: string, value: boolean) => Promise<{ success: boolean; error?: string }>;
  setRts: (path: string, value: boolean) => Promise<{ success: boolean; error?: string }>;
  isOpen: (path: string) => Promise<boolean>;
  onData: (callback: (data: { port: string; data: number[] }) => void) => () => void;
  onStatus: (callback: (data: { port: string; connected: boolean }) => void) => () => void;
  onError: (callback: (data: { port: string; error: string }) => void) => () => void;
}

export interface ElectronTcpAPI {
  connect: (id: string, host: string, port: number) => Promise<{ success: boolean; error?: string }>;
  disconnect: (id: string) => Promise<{ success: boolean }>;
  send: (id: string, data: number[]) => Promise<{ success: boolean; error?: string }>;
  createServer: (id: string, port: number, host?: string) => Promise<{ success: boolean; error?: string }>;
  closeServer: (id: string) => Promise<{ success: boolean }>;
  serverSend: (serverId: string, clientId: string, data: number[]) => Promise<{ success: boolean; error?: string }>;
  serverBroadcast: (serverId: string, data: number[]) => Promise<{ success: boolean; error?: string }>;
  onData: (callback: (data: { id: string; data: number[] }) => void) => () => void;
  onClosed: (callback: (data: { id: string }) => void) => () => void;
  onError: (callback: (data: { id: string; error: string }) => void) => () => void;
  onServerData: (callback: (data: { serverId: string; clientId: string; remoteAddress: string; data: number[] }) => void) => () => void;
  onServerClientConnected: (callback: (data: { serverId: string; clientId: string; remoteAddress: string; remotePort: number }) => void) => () => void;
  onServerClientDisconnected: (callback: (data: { serverId: string; clientId: string }) => void) => () => void;
}

export interface ElectronUdpAPI {
  createClient: (id: string) => Promise<{ success: boolean; error?: string }>;
  clientBind: (id: string, port: number, address?: string) => Promise<{ success: boolean; address?: string; port?: number; error?: string }>;
  clientSend: (id: string, data: number[], remotePort: number, remoteAddress?: string) => Promise<{ success: boolean; error?: string }>;
  clientClose: (id: string) => Promise<{ success: boolean }>;
  createServer: (id: string, port: number, address?: string) => Promise<{ success: boolean; address?: string; port?: number; error?: string }>;
  serverSend: (id: string, data: number[], remotePort: number, remoteAddress: string) => Promise<{ success: boolean; error?: string }>;
  serverClose: (id: string) => Promise<{ success: boolean }>;
  onClientData: (callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void) => () => void;
  onClientError: (callback: (data: { id: string; error: string }) => void) => () => void;
  onClientClosed: (callback: (data: { id: string }) => void) => () => void;
  onServerData: (callback: (data: { id: string; data: number[]; remoteAddress: string; remotePort: number }) => void) => () => void;
  onServerError: (callback: (data: { id: string; error: string }) => void) => () => void;
}

export interface ElectronWsAPI {
  connect: (id: string, url: string) => Promise<{ success: boolean; error?: string }>;
  send: (id: string, data: string | number[]) => Promise<{ success: boolean; error?: string }>;
  disconnect: (id: string) => Promise<{ success: boolean }>;
  createServer: (id: string, port: number) => Promise<{ success: boolean; port?: number; error?: string }>;
  closeServer: (id: string) => Promise<{ success: boolean }>;
  onConnected: (callback: (data: { id: string }) => void) => () => void;
  onDisconnected: (callback: (data: { id: string; code: number; reason?: string }) => void) => () => void;
  onMessage: (callback: (data: { id: string; data: string | number[] }) => void) => () => void;
  onError: (callback: (data: { id: string; error: string }) => () => void) => () => void;
}

export interface SerialOpenOptions {
  path: string;
  baudRate: number;
  dataBits?: 5 | 6 | 7 | 8;
  stopBits?: 1 | 1.5 | 2;
  parity?: 'none' | 'odd' | 'even' | 'mark' | 'space';
  rtscts?: boolean;
  xon?: boolean;
  xoff?: boolean;
}

export interface ElectronAPI {
  serial: ElectronSerialAPI;
  tcp: ElectronTcpAPI;
  udp: ElectronUdpAPI;
  ws: ElectronWsAPI;
}

declare global {
  interface Window {
    electronAPI?: ElectronAPI;
  }
}
