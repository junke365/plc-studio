import { app, BrowserWindow } from 'electron';
import * as path from 'path';
import { registerSerialHandlers, closeAllSerialPorts } from './serial/SerialManager';
import { registerTcpHandlers, closeAllTcp } from './network/TcpManager';
import { registerUdpHandlers, closeAllUdp } from './network/UdpManager';
import { registerWebSocketHandlers, closeAllWebSocket } from './network/WebSocketManager';

let mainWindow: BrowserWindow | null = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1024,
    minHeight: 700,
    title: 'PLC Studio',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
  });

  const isDev = process.env.ELECTRON_DEV === 'true';
  console.log('[Electron] isDev:', isDev, '__dirname:', __dirname);

  if (isDev) {
    const devUrl = 'http://localhost:5173';
    console.log('[Electron] 加载开发服务器:', devUrl);
    mainWindow.loadURL(devUrl).catch((err) => {
      console.error('[Electron] 加载开发服务器失败:', err.message);
    });
    mainWindow.webContents.openDevTools();
  } else {
    const filePath = path.join(__dirname, '../../packages/editor/dist/index.html');
    console.log('[Electron] 加载本地文件:', filePath);
    mainWindow.loadFile(filePath).catch((err) => {
      console.error('[Electron] 加载本地文件失败:', err.message);
    });
  }

  mainWindow.webContents.on('did-fail-load', (_event, errorCode, errorDescription, validatedURL) => {
    console.error('[Electron] 页面加载失败:', errorCode, errorDescription, validatedURL);
  });

  mainWindow.webContents.on('console-message', (_event, level, message) => {
    console.log('[渲染进程]', message);
  });

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

registerSerialHandlers();
registerTcpHandlers();
registerUdpHandlers();
registerWebSocketHandlers();

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  closeAllSerialPorts();
  closeAllTcp();
  closeAllUdp();
  closeAllWebSocket();
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', () => {
  closeAllSerialPorts();
  closeAllTcp();
  closeAllUdp();
  closeAllWebSocket();
});
