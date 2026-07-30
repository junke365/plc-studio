import { SerialPort } from 'serialport'

/* 调试协议常量（与 plc_debug_protocol.h 一致）*/
const DBG_HEADER0 = 0xDB
const DBG_HEADER1 = 0xDC

/* 命令码 PC → STM32 */
const CMD_PING = 0x01
const CMD_GET_STATUS = 0x02
const CMD_READ_VAR = 0x03
const CMD_WRITE_VAR = 0x04
const CMD_SET_BP = 0x05
const CMD_REMOVE_BP = 0x06
const CMD_STEP = 0x07
const CMD_RUN = 0x08
const CMD_PAUSE = 0x09
const CMD_GET_BPS = 0x0A

/* 响应码 STM32 → PC */
const RSP_PONG = 0x81
const RSP_STATUS = 0x82
const RSP_VAR_VALUE = 0x83
const RSP_VAR_WRITTEN = 0x84
const RSP_BP_HIT = 0x85
const RSP_STEPPED = 0x86
const RSP_LOG = 0x87
const RSP_ERROR = 0x88

export type TransportEventHandler = {
  onStatus?: (status: Record<string, unknown>) => void
  onVarValue?: (path: string, value: unknown) => void
  onVarWritten?: (path: string, success: boolean) => void
  onBpHit?: (id: string) => void
  onStepped?: () => void
  onLog?: (level: string, message: string) => void
  onError?: (message: string) => void
  onConnected?: () => void
  onDisconnected?: () => void
}

export class DebugTransportService {
  private port: SerialPort | null = null
  private rxBuf: Buffer = Buffer.alloc(0)
  private handlers: TransportEventHandler = {}
  private _connected = false

  get connected(): boolean { return this._connected }

  on(handlers: TransportEventHandler): void {
    this.handlers = handlers
  }

  async connect(comPort: string, baudRate: number): Promise<void> {
    return new Promise((resolve, reject) => {
      try {
        this.port = new SerialPort({
          path: comPort,
          baudRate,
          dataBits: 8,
          stopBits: 1,
          parity: 'none',
          autoOpen: false,
        })

        this.port.open((err) => {
          if (err) {
            this._connected = false
            reject(new Error(`串口打开失败: ${err.message}`))
            return
          }
          this._connected = true
          this.port!.on('data', (data: Buffer) => this.onData(data))
          this.port!.on('close', () => {
            this._connected = false
            this.handlers.onDisconnected?.()
          })
          this.handlers.onConnected?.()
          resolve()
        })
      } catch (e) {
        reject(e)
      }
    })
  }

  disconnect(): void {
    if (this.port) {
      this.port.close()
      this.port = null
    }
    this._connected = false
  }

  /* 发送调试命令 */
  async ping(): Promise<void> {
    this.sendFrame(CMD_PING)
  }

  async getStatus(): Promise<void> {
    this.sendFrame(CMD_GET_STATUS)
  }

  async readVar(path: string): Promise<void> {
    const payload = Buffer.from(path, 'utf-8')
    this.sendFrame(CMD_READ_VAR, payload)
  }

  async writeVar(path: string, value: unknown): Promise<void> {
    const pathBuf = Buffer.from(path, 'utf-8')
    const valStr = String(value)
    const valBuf = Buffer.from(valStr, 'utf-8')
    const payload = Buffer.alloc(2 + pathBuf.length + valBuf.length)
    payload.writeUInt16LE(pathBuf.length, 0)
    pathBuf.copy(payload, 2)
    valBuf.copy(payload, 2 + pathBuf.length)
    this.sendFrame(CMD_WRITE_VAR, payload)
  }

  async setBreakpoint(id: string, path: string, line: number): Promise<void> {
    const idBuf = Buffer.from(id, 'utf-8')
    const pathBuf = Buffer.from(path, 'utf-8')
    const payload = Buffer.alloc(4 + idBuf.length + pathBuf.length)
    payload.writeUInt16LE(idBuf.length, 0)
    idBuf.copy(payload, 2)
    payload.writeUInt16LE(pathBuf.length, 2 + idBuf.length)
    pathBuf.copy(payload, 4 + idBuf.length)
    payload.writeUInt16LE(line, 4 + idBuf.length + pathBuf.length)
    this.sendFrame(CMD_SET_BP, payload)
  }

  async removeBreakpoint(id: string): Promise<void> {
    const payload = Buffer.from(id, 'utf-8')
    this.sendFrame(CMD_REMOVE_BP, payload)
  }

  async step(): Promise<void> {
    this.sendFrame(CMD_STEP)
  }

  async run(): Promise<void> {
    this.sendFrame(CMD_RUN)
  }

  async pause(): Promise<void> {
    this.sendFrame(CMD_PAUSE)
  }

  async getBreakpoints(): Promise<void> {
    this.sendFrame(CMD_GET_BPS)
  }

  /* 构建并发送帧 */
  private sendFrame(cmd: number, payload?: Buffer): void {
    if (!this.port || !this.port.isOpen) return
    const payLen = payload ? payload.length : 0
    const frame = Buffer.alloc(4 + payLen)
    frame[0] = DBG_HEADER0
    frame[1] = DBG_HEADER1
    frame[2] = payLen
    frame[3] = cmd
    if (payload) payload.copy(frame, 4)
    this.port.write(frame)
  }

  /* 接收数据处理 */
  private onData(data: Buffer): void {
    this.rxBuf = Buffer.concat([this.rxBuf, data])
    this.tryParseFrames()
  }

  private tryParseFrames(): void {
    while (this.rxBuf.length >= 4) {
      /* 查找帧头 */
      const h0 = this.rxBuf.indexOf(DBG_HEADER0)
      if (h0 < 0 || h0 + 1 >= this.rxBuf.length) break
      if (this.rxBuf[h0 + 1] !== DBG_HEADER1) {
        /* 跳过错误字节 */
        this.rxBuf = this.rxBuf.slice(h0 + 1)
        continue
      }
      const payLen = this.rxBuf[h0 + 2]
      const frameLen = 4 + payLen
      if (h0 + frameLen > this.rxBuf.length) break /* 不完整的帧 */
      const cmd = this.rxBuf[h0 + 3]
      const payload = this.rxBuf.slice(h0 + 4, h0 + frameLen)
      this.handleFrame(cmd, payload)
      this.rxBuf = this.rxBuf.slice(h0 + frameLen)
    }
  }

  private handleFrame(cmd: number, payload: Buffer): void {
    switch (cmd) {
      case RSP_PONG:
        break
      case RSP_STATUS:
        try {
          const status = JSON.parse(payload.toString('utf-8'))
          this.handlers.onStatus?.(status)
        } catch { /* ignore */ }
        break
      case RSP_VAR_VALUE: {
        const pathLen = payload.readUInt16LE(0)
        const path = payload.slice(2, 2 + pathLen).toString('utf-8')
        const valStr = payload.slice(2 + pathLen).toString('utf-8')
        this.handlers.onVarValue?.(path, valStr)
        break
      }
      case RSP_VAR_WRITTEN: {
        const pathLen = payload.readUInt16LE(0)
        const path = payload.slice(2, 2 + pathLen).toString('utf-8')
        const success = payload[2 + pathLen] !== 0
        this.handlers.onVarWritten?.(path, success)
        break
      }
      case RSP_BP_HIT: {
        const id = payload.toString('utf-8')
        this.handlers.onBpHit?.(id)
        break
      }
      case RSP_STEPPED:
        this.handlers.onStepped?.()
        break
      case RSP_LOG: {
        const levelLen = payload[0]
        const level = payload.slice(1, 1 + levelLen).toString('utf-8')
        const message = payload.slice(1 + levelLen).toString('utf-8')
        this.handlers.onLog?.(level, message)
        break
      }
      case RSP_ERROR: {
        const message = payload.toString('utf-8')
        this.handlers.onError?.(message)
        break
      }
    }
  }
}