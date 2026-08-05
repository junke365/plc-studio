import { app, BrowserWindow, session } from 'electron';
import * as path from 'path';
import { registerSerialHandlers, closeAllSerialPorts } from './serial/SerialManager';
import { registerTcpHandlers, closeAllTcp } from './network/TcpManager';
import { registerUdpHandlers, closeAllUdp } from './network/UdpManager';
import { registerWebSocketHandlers, closeAllWebSocket } from './network/WebSocketManager';

let mainWindow: BrowserWindow | null = null;

const isDev = process.env.ELECTRON_DEV === 'true';

async function startBackendServer() {
  try {
    const serverMain = path.join(__dirname, '../../packages/server/dist-server/main.cjs');
    console.log('[Electron] 启动后端服务器:', serverMain);
    // 使用 require 加载（asar-aware），bundle 为 CJS 自包含产物
    const mod = require(serverMain);
    if (typeof mod.startServer === 'function') {
      await mod.startServer();
      console.log('[Electron] 后端服务器已就绪');
    }
  } catch (err) {
    console.error('[Electron] 后端服务器启动失败:', (err as Error)?.message, err);
  }
}

async function createWindow() {
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

app.whenReady().then(async () => {
  // 允许渲染进程访问摄像头/麦克风（FBD 相机节点使用 getUserMedia）
  session.defaultSession.setPermissionRequestHandler((_webContents, permission, callback) => {
    if (permission === 'media' || permission.startsWith('media')) {
      callback(true);
    } else {
      callback(false);
    }
  });
  if (!isDev) {
    await startBackendServer();
  }
  await createWindow();
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
