import * as net from 'net';
import { BrowserWindow, ipcMain } from 'electron';

interface TcpClientInstance {
  socket: net.Socket;
  id: string;
}

interface TcpServerInstance {
  server: net.Server;
  clients: Map<string, net.Socket>;
  id: string;
}

const tcpClients = new Map<string, TcpClientInstance>();
const tcpServers = new Map<string, TcpServerInstance>();
let clientCounter = 0;
let serverCounter = 0;

export function registerTcpHandlers() {
  ipcMain.handle('tcp:connect', async (_event, id: string, host: string, port: number) => {
    try {
      if (tcpClients.has(id)) {
        tcpClients.get(id)!.socket.destroy();
        tcpClients.delete(id);
      }

      const socket = new net.Socket();
      const win = BrowserWindow.getAllWindows()[0];

      return new Promise((resolve) => {
        socket.connect(port, host, () => {
          tcpClients.set(id, { socket, id });
          resolve({ success: true });
        });

        socket.on('data', (data: Buffer) => {
          if (win) {
            win.webContents.send('tcp:data', { id, data: Array.from(data) });
          }
        });

        socket.on('close', () => {
          tcpClients.delete(id);
          if (win) {
            win.webContents.send('tcp:closed', { id });
          }
        });

        socket.on('error', (err) => {
          if (win) {
            win.webContents.send('tcp:error', { id, error: err.message });
          }
          resolve({ success: false, error: err.message });
        });

        setTimeout(() => {
          if (!tcpClients.has(id)) {
            socket.destroy();
            resolve({ success: false, error: '连接超时' });
          }
        }, 5000);
      });
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('tcp:disconnect', async (_event, id: string) => {
    const client = tcpClients.get(id);
    if (client) {
      client.socket.destroy();
      tcpClients.delete(id);
    }
    return { success: true };
  });

  ipcMain.handle('tcp:send', async (_event, id: string, data: number[]) => {
    const client = tcpClients.get(id);
    if (!client) {
      return { success: false, error: '未连接' };
    }
    return new Promise((resolve) => {
      client.socket.write(Buffer.from(data), (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('tcp:create-server', async (_event, id: string, port: number, host?: string) => {
    try {
      if (tcpServers.has(id)) {
        tcpServers.get(id)!.server.close();
        tcpServers.delete(id);
      }

      const server = net.createServer();
      const clients = new Map<string, net.Socket>();
      const win = BrowserWindow.getAllWindows()[0];

      server.on('connection', (socket) => {
        const clientId = `${id}_client_${Date.now()}`;
        const remoteAddr = `${socket.remoteAddress}:${socket.remotePort}`;

        clients.set(clientId, socket);

        if (win) {
          win.webContents.send('tcp:server-client-connected', {
            serverId: id,
            clientId,
            remoteAddress: socket.remoteAddress,
            remotePort: socket.remotePort,
          });
        }

        socket.on('data', (data: Buffer) => {
          if (win) {
            win.webContents.send('tcp:server-data', {
              serverId: id,
              clientId,
              remoteAddress: remoteAddr,
              data: Array.from(data),
            });
          }
        });

        socket.on('close', () => {
          clients.delete(clientId);
          if (win) {
            win.webContents.send('tcp:server-client-disconnected', {
              serverId: id,
              clientId,
            });
          }
        });

        socket.on('error', () => {
          clients.delete(clientId);
        });
      });

      return new Promise((resolve) => {
        server.listen(port, host ?? '0.0.0.0', () => {
          tcpServers.set(id, { server, clients, id });
          resolve({ success: true });
        });

        server.on('error', (err) => {
          tcpServers.delete(id);
          resolve({ success: false, error: err.message });
        });
      });
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('tcp:close-server', async (_event, id: string) => {
    const server = tcpServers.get(id);
    if (server) {
      for (const [, client] of server.clients) {
        client.destroy();
      }
      server.server.close();
      tcpServers.delete(id);
    }
    return { success: true };
  });

  ipcMain.handle('tcp:server-send', async (_event, serverId: string, clientId: string, data: number[]) => {
    const server = tcpServers.get(serverId);
    if (!server) return { success: false, error: '服务器未运行' };
    const client = server.clients.get(clientId);
    if (!client) return { success: false, error: '客户端未连接' };
    return new Promise((resolve) => {
      client.write(Buffer.from(data), (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('tcp:server-broadcast', async (_event, serverId: string, data: number[]) => {
    const server = tcpServers.get(serverId);
    if (!server) return { success: false, error: '服务器未运行' };
    const buf = Buffer.from(data);
    for (const [, client] of server.clients) {
      client.write(buf);
    }
    return { success: true };
  });
}

export function closeAllTcp() {
  for (const [, client] of tcpClients) {
    client.socket.destroy();
  }
  tcpClients.clear();

  for (const [, server] of tcpServers) {
    for (const [, client] of server.clients) {
      client.destroy();
    }
    server.server.close();
  }
  tcpServers.clear();
}
