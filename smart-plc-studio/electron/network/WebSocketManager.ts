import * as WebSocket from 'ws';
import { BrowserWindow, ipcMain } from 'electron';

const wsClients = new Map<string, WebSocket.WebSocket>();
const wsServers = new Map<string, WebSocket.Server>();

export function registerWebSocketHandlers() {
  ipcMain.handle('ws:connect', async (_event, id: string, url: string) => {
    try {
      if (wsClients.has(id)) {
        wsClients.get(id)!.close();
        wsClients.delete(id);
      }

      const win = BrowserWindow.getAllWindows()[0];
      const ws = new WebSocket.WebSocket(url);

      return new Promise((resolve) => {
        ws.on('open', () => {
          wsClients.set(id, ws);
          if (win) win.webContents.send('ws:connected', { id });
          resolve({ success: true });
        });

        ws.on('message', (data: WebSocket.Data) => {
          if (win) {
            const msg = typeof data === 'string' ? data : Array.from(data as Buffer);
            win.webContents.send('ws:message', { id, data: msg });
          }
        });

        ws.on('close', (code, reason) => {
          wsClients.delete(id);
          if (win) {
            win.webContents.send('ws:disconnected', { id, code, reason: reason?.toString() });
          }
        });

        ws.on('error', (err) => {
          if (win) {
            win.webContents.send('ws:error', { id, error: err.message });
          }
          resolve({ success: false, error: err.message });
        });

        setTimeout(() => {
          if (!wsClients.has(id)) {
            ws.close();
            resolve({ success: false, error: '连接超时' });
          }
        }, 5000);
      });
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('ws:send', async (_event, id: string, data: string | number[]) => {
    const ws = wsClients.get(id);
    if (!ws || ws.readyState !== WebSocket.WebSocket.OPEN) {
      return { success: false, error: '未连接' };
    }
    try {
      const buf = typeof data === 'string' ? data : Buffer.from(data);
      ws.send(buf);
      return { success: true };
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('ws:disconnect', async (_event, id: string) => {
    const ws = wsClients.get(id);
    if (ws) {
      ws.close();
      wsClients.delete(id);
    }
    return { success: true };
  });

  ipcMain.handle('ws:create-server', async (_event, id: string, port: number) => {
    try {
      if (wsServers.has(id)) {
        wsServers.get(id)!.close();
        wsServers.delete(id);
      }

      const win = BrowserWindow.getAllWindows()[0];
      const server = new WebSocket.Server({ port });

      return new Promise((resolve) => {
        server.on('listening', () => {
          wsServers.set(id, server);
          resolve({ success: true, port });
        });

        server.on('connection', (ws, req) => {
          const clientId = `client_${Date.now()}`;
          const ip = req.socket.remoteAddress;

          if (win) {
            win.webContents.send('ws:server-client-connected', {
              serverId: id,
              clientId,
              ip,
            });
          }

          ws.on('message', (data) => {
            if (win) {
              const msg = typeof data === 'string' ? data : Array.from(data as Buffer);
              win.webContents.send('ws:server-data', { serverId: id, clientId, data: msg });
            }
          });

          ws.on('close', () => {
            if (win) {
              win.webContents.send('ws:server-client-disconnected', { serverId: id, clientId });
            }
          });
        });

        server.on('error', (err) => {
          wsServers.delete(id);
          resolve({ success: false, error: err.message });
        });
      });
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('ws:close-server', async (_event, id: string) => {
    const server = wsServers.get(id);
    if (server) {
      server.close();
      wsServers.delete(id);
    }
    return { success: true };
  });
}

export function closeAllWebSocket() {
  for (const [, ws] of wsClients) {
    ws.close();
  }
  wsClients.clear();
  for (const [, server] of wsServers) {
    server.close();
  }
  wsServers.clear();
}
