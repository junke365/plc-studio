import { SerialPort } from 'serialport'

export interface SerialOptions {
  path: string
  baudRate: number
  dataBits?: 5 | 6 | 7 | 8
  stopBits?: 1 | 1.5 | 2
  parity?: 'none' | 'odd' | 'even' | 'mark' | 'space'
  rtscts?: boolean
  xon?: boolean
  xoff?: boolean
}

type EventHandler = (event: string, data: any) => void

const instances = new Map<string, SerialPort>()
let eventHandler: EventHandler | null = null

export function setSerialEventHandler(handler: EventHandler) {
  eventHandler = handler
}

function emit(event: string, data: any) {
  eventHandler?.(event, data)
}

export async function listPorts() {
  const ports = await SerialPort.list()
  return ports.map(p => ({
    path: p.path,
    manufacturer: p.manufacturer,
    serialNumber: p.serialNumber,
    pnpId: p.pnpId,
    locationId: p.locationId,
    vendorId: p.vendorId,
    productId: p.productId,
  }))
}

export async function openPort(options: SerialOptions) {
  const key = options.path
  if (instances.has(key)) {
    const old = instances.get(key)!
    old.close()
    instances.delete(key)
  }

  return new Promise<{ success: boolean; error?: string }>((resolve) => {
    const port = new SerialPort({
      path: options.path,
      baudRate: options.baudRate,
      dataBits: options.dataBits ?? 8,
      stopBits: options.stopBits ?? 1,
      parity: options.parity ?? 'none',
      rtscts: options.rtscts ?? false,
      xon: options.xon ?? false,
      xoff: options.xoff ?? false,
    })

    port.on('open', () => {
      instances.set(key, port)
      emit('serial:status', { port: options.path, connected: true })
      resolve({ success: true })
    })

    port.on('close', () => {
      instances.delete(key)
      emit('serial:status', { port: options.path, connected: false })
    })

    port.on('error', (err: Error) => {
      emit('serial:error', { port: options.path, error: err.message })
    })

    port.on('data', (data: Buffer) => {
      const arr = Array.from(data)
      console.log(`[Serial] 收到数据 ${options.path}: ${arr.length} bytes [${arr.slice(0, 20).join(',')}${arr.length > 20 ? '...' : ''}]`)
      emit('serial:data', {
        port: options.path,
        data: arr,
      })
    })
  })
}

export async function closePort(path: string) {
  const port = instances.get(path)
  if (!port) {
    return { success: false, error: '串口未打开' }
  }
  return new Promise<{ success: boolean; error?: string }>((resolve) => {
    port.close((err: Error | null) => {
      instances.delete(path)
      if (err) {
        resolve({ success: false, error: err.message })
      } else {
        resolve({ success: true })
      }
    })
  })
}

export async function writeToPort(path: string, data: number[]) {
  const port = instances.get(path)
  if (!port) {
    return { success: false, error: '串口未打开' }
  }
  return new Promise<{ success: boolean; error?: string }>((resolve) => {
    port.write(Buffer.from(data), (err: Error | null | undefined) => {
      if (err) {
        resolve({ success: false, error: err.message })
      } else {
        resolve({ success: true })
      }
    })
  })
}

export async function setDtr(path: string, value: boolean) {
  const port = instances.get(path)
  if (!port) return { success: false, error: '串口未打开' }
  return new Promise<{ success: boolean; error?: string }>((resolve) => {
    port.set({ dtr: value }, (err: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export async function setRts(path: string, value: boolean) {
  const port = instances.get(path)
  if (!port) return { success: false, error: '串口未打开' }
  return new Promise<{ success: boolean; error?: string }>((resolve) => {
    port.set({ rts: value }, (err: Error | null) => {
      resolve(err ? { success: false, error: err.message } : { success: true })
    })
  })
}

export function isPortOpen(path: string) {
  return instances.get(path)?.isOpen ?? false
}

export function closeAllSerialPorts() {
  for (const [, port] of instances) {
    if (port.isOpen) port.close()
  }
  instances.clear()
}
