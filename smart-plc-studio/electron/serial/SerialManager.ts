import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { BrowserWindow, ipcMain } from 'electron';

export interface PortInfo {
  path: string;
  manufacturer?: string;
  serialNumber?: string;
  pnpId?: string;
  locationId?: string;
  vendorId?: string;
  productId?: string;
}

export interface SerialOptions {
  path: string;
  baudRate: number;
  dataBits?: 5 | 6 | 7 | 8;
  stopBits?: 1 | 1.5 | 2;
  parity?: 'none' | 'odd' | 'even' | 'mark' | 'space';
  rtscts?: boolean;
  xon?: boolean;
  xoff?: boolean;
}

const instances = new Map<string, SerialPort>();

function getConfigKey(port: string) {
  return port;
}

export function registerSerialHandlers() {
  ipcMain.handle('serial:list-ports', async () => {
    try {
      const ports = await SerialPort.list();
      return { success: true, data: ports as PortInfo[] };
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('serial:open', async (_event, options: SerialOptions) => {
    const key = getConfigKey(options.path);
    try {
      if (instances.has(key)) {
        const old = instances.get(key)!;
        old.close();
        instances.delete(key);
      }

      const port = new SerialPort({
        path: options.path,
        baudRate: options.baudRate,
        dataBits: options.dataBits ?? 8,
        stopBits: options.stopBits ?? 1,
        parity: options.parity ?? 'none',
        rtscts: options.rtscts ?? false,
        xon: options.xon ?? false,
        xoff: options.xoff ?? false,
      });

      const parser = port.pipe(new ReadlineParser({ delimiter: Buffer.from('\n') }));

      port.on('open', () => {
        const win = BrowserWindow.getAllWindows()[0];
        if (win) {
          win.webContents.send('serial:status', { port: options.path, connected: true });
        }
      });

      port.on('close', () => {
        instances.delete(key);
        const win = BrowserWindow.getAllWindows()[0];
        if (win) {
          win.webContents.send('serial:status', { port: options.path, connected: false });
        }
      });

      port.on('error', (err) => {
        const win = BrowserWindow.getAllWindows()[0];
        if (win) {
          win.webContents.send('serial:error', { port: options.path, error: err.message });
        }
      });

      port.on('data', (data: Buffer) => {
        const win = BrowserWindow.getAllWindows()[0];
        if (win) {
          win.webContents.send('serial:data', {
            port: options.path,
            data: Array.from(data),
          });
        }
      });

      instances.set(key, port);
      return { success: true };
    } catch (err: any) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('serial:close', async (_event, path: string) => {
    const key = getConfigKey(path);
    const port = instances.get(key);
    if (!port) {
      return { success: false, error: '串口未打开' };
    }
    return new Promise((resolve) => {
      port.close((err) => {
        instances.delete(key);
        if (err) {
          resolve({ success: false, error: err.message });
        } else {
          resolve({ success: true });
        }
      });
    });
  });

  ipcMain.handle('serial:write', async (_event, path: string, data: number[]) => {
    const key = getConfigKey(path);
    const port = instances.get(key);
    if (!port) {
      return { success: false, error: '串口未打开' };
    }
    return new Promise((resolve) => {
      port.write(Buffer.from(data), (err) => {
        if (err) {
          resolve({ success: false, error: err.message });
        } else {
          resolve({ success: true });
        }
      });
    });
  });

  ipcMain.handle('serial:set-dtr', async (_event, path: string, value: boolean) => {
    const key = getConfigKey(path);
    const port = instances.get(key);
    if (!port) return { success: false, error: '串口未打开' };
    return new Promise((resolve) => {
      port.set({ dtr: value }, (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('serial:set-rts', async (_event, path: string, value: boolean) => {
    const key = getConfigKey(path);
    const port = instances.get(key);
    if (!port) return { success: false, error: '串口未打开' };
    return new Promise((resolve) => {
      port.set({ rts: value }, (err) => {
        resolve(err ? { success: false, error: err.message } : { success: true });
      });
    });
  });

  ipcMain.handle('serial:is-open', async (_event, path: string) => {
    const key = getConfigKey(path);
    const port = instances.get(key);
    return port?.isOpen ?? false;
  });
}

export function closeAllSerialPorts() {
  for (const [key, port] of instances) {
    if (port.isOpen) {
      port.close();
    }
  }
  instances.clear();
}
