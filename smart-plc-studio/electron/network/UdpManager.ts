import * as dgram from 'dgram';
import { BrowserWindow, ipcMain } from 'electron';

interface UdpClientInstance {
  socket: dgram.Socket;
  id: string;
  bound: boolean;
}

interface UdpServerInstance {
  socket: dgram.Socket;
  id: string;
}

const udpClients = new Map<string, UdpClientInstance>();
const udpServers = new Map<string, UdpServerInstance>();

export function registerUdpHandlers() {
  ipcMain.handle('udp:create-client', async (_event, id: string) => {
    try {
      if (udpClients.has(id)) {
        udpClients.get(id)!.socket.close();
        udpClients.delete(id);
      }

      const socket = dgram.createSocket('udp4');
      const win = BrowserWindow.getAllWindows()[0];

      socket.on('message', (msg: Buffer, rinfo: dgram.RemoteInfo) => {
        if (win) {
          win.webContents.send('udp:client-data', {
            id,
            data: Array.from(msg),
            remoteAddress: rinfo.address,
            remotePort: rinfo.port,
          });
        }
      });

      socket.on('error', (err) => {
        if (win) {
          win.webContents.send('udp:client-error', { id, error: err.message });
        }
      });

      socket.on('close', () => {
        udpClients.delete(id);
        if (win) {
          win.webContents.send('udp:client-closed', { id });
        }
      });

      udpClients.set(id, { socket, id, bound: false });
      return { success: true };
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('udp:client-bind', async (_event, id: string, port: number, address?: string) => {
    const client = udpClients.get(id);
    if (!client) return { success: false, error: '客户端未创建' };

    return new Promise((resolve) => {
      client.socket.bind(port, address ?? '0.0.0.0', () => {
        client.bound = true;
        const addr = client.socket.address();
        resolve({ success: true, address: addr.address, port: addr.port });
      });

      client.socket.once('error', (err) => {
        resolve({ success: false, error: err.message });
      });
    });
  });

  ipcMain.handle('udp:client-send', async (_event, id: string, data: number[], remotePort: number, remoteAddress?: string) => {
    const client = udpClients.get(id);
    if (!client) return { success: false, error: '客户端未创建' };

    return new Promise((resolve) => {
      const buf = Buffer.from(data);
      client.socket.send(buf, remotePort, remoteAddress ?? '127.0.0.1', (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('udp:client-close', async (_event, id: string) => {
    const client = udpClients.get(id);
    if (client) {
      client.socket.close();
      udpClients.delete(id);
    }
    return { success: true };
  });

  ipcMain.handle('udp:create-server', async (_event, id: string, port: number, address?: string) => {
    try {
      if (udpServers.has(id)) {
        udpServers.get(id)!.socket.close();
        udpServers.delete(id);
      }

      const socket = dgram.createSocket('udp4');
      const win = BrowserWindow.getAllWindows()[0];

      socket.on('message', (msg: Buffer, rinfo: dgram.RemoteInfo) => {
        if (win) {
          win.webContents.send('udp:server-data', {
            id,
            data: Array.from(msg),
            remoteAddress: rinfo.address,
            remotePort: rinfo.port,
          });
        }
      });

      socket.on('error', (err) => {
        if (win) {
          win.webContents.send('udp:server-error', { id, error: err.message });
        }
      });

      socket.on('close', () => {
        udpServers.delete(id);
      });

      return new Promise((resolve) => {
        socket.bind(port, address ?? '0.0.0.0', () => {
          udpServers.set(id, { socket, id });
          const addr = socket.address();
          resolve({ success: true, address: addr.address, port: addr.port });
        });

        socket.once('error', (err) => {
          resolve({ success: false, error: err.message });
        });
      });
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('udp:server-send', async (_event, id: string, data: number[], remotePort: number, remoteAddress: string) => {
    const server = udpServers.get(id);
    if (!server) return { success: false, error: '服务器未运行' };

    return new Promise((resolve) => {
      server.socket.send(Buffer.from(data), remotePort, remoteAddress, (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('udp:server-close', async (_event, id: string) => {
    const server = udpServers.get(id);
    if (server) {
      server.socket.close();
      udpServers.delete(id);
    }
    return { success: true };
  });
}

export function closeAllUdp() {
  for (const [, client] of udpClients) {
    client.socket.close();
  }
  udpClients.clear();
  for (const [, server] of udpServers) {
    server.socket.close();
  }
  udpServers.clear();
}
