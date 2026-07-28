<script setup lang="ts">
import { ref, reactive, onUnmounted } from 'vue'
import {
  serialOpen,
  serialClose,
  serialWrite,
  onSerialData,
  serialListPorts,
  type PortInfo,
} from '@/serial/serialClient'

interface ModbusRegister {
  address: string
  value: string
  hex: string
}

interface RawFrame {
  dir: 'TX' | 'RX'
  hex: string
  time: string
}

const connected = ref(false)
const running = ref(false)
const frameIndex = ref(0)

const connConfig = reactive({
  port: 'COM1',
  baudRate: 115200,
  station: 1,
})

const request = reactive({
  funcCode: 3,
  startAddr: 0,
  quantity: 10,
})

const registers = ref<ModbusRegister[]>([])
const rawFrames = ref<RawFrame[]>([])
const logMessages = ref<string[]>([])
const serialPorts = ref<PortInfo[]>([])

const baudRates = [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400]

const funcCodes = [
  { code: 1, name: '01 - 读线圈', desc: 'Read Coils' },
  { code: 2, name: '02 - 读离散输入', desc: 'Read Discrete Inputs' },
  { code: 3, name: '03 - 读保持寄存器', desc: 'Read Holding Registers' },
  { code: 4, name: '04 - 读输入寄存器', desc: 'Read Input Registers' },
  { code: 5, name: '05 - 写单个线圈', desc: 'Write Single Coil' },
  { code: 6, name: '06 - 写单个寄存器', desc: 'Write Single Register' },
  { code: 15, name: '15 - 写多个线圈', desc: 'Write Multiple Coils' },
  { code: 16, name: '16 - 写多个寄存器', desc: 'Write Multiple Registers' },
]

// RTU 帧接收缓冲区和超时定时器
let rxBuffer: number[] = []
let rxTimer: ReturnType<typeof setTimeout> | null = null
let responseTimer: ReturnType<typeof setTimeout> | null = null
let cleanupSerialData: (() => void) | null = null

// 加载串口列表
async function loadPorts() {
  try {
    serialPorts.value = await serialListPorts()
    if (serialPorts.value.length > 0) {
      connConfig.port = serialPorts.value[0].path
    }
  } catch {
    // 忽略加载失败
  }
}

function isWriteCode(code: number): boolean {
  return [5, 6, 15, 16].includes(code)
}

function addLog(msg: string) {
  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  logMessages.value.push(`[${time}] ${msg}`)
  if (logMessages.value.length > 200) logMessages.value.shift()
}

function addFrame(dir: 'TX' | 'RX', hex: string) {
  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  rawFrames.value.push({ dir, hex, time })
  if (rawFrames.value.length > 500) rawFrames.value.shift()
}

function generateCrc16(data: number[]): number {
  let crc = 0xffff
  for (const byte of data) {
    crc ^= byte
    for (let i = 0; i < 8; i++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xa001
      } else {
        crc >>= 1
      }
    }
  }
  return crc
}

function buildRequestFrame(): number[] {
  const frame: number[] = [connConfig.station, request.funcCode]
  const hi = (request.startAddr >> 8) & 0xff
  const lo = request.startAddr & 0xff
  if (isWriteCode(request.funcCode)) {
    frame.push(hi, lo)
    frame.push((request.quantity >> 8) & 0xff, request.quantity & 0xff)
    if (request.funcCode === 5 || request.funcCode === 6) {
      frame.pop()
      frame.pop()
      frame.push(0xff, 0x00)
    } else {
      frame.push(Math.ceil(request.quantity / 8))
      for (let i = 0; i < Math.ceil(request.quantity / 8); i++) {
        frame.push(0x00)
      }
    }
  } else {
    frame.push(hi, lo, (request.quantity >> 8) & 0xff, request.quantity & 0xff)
  }
  const crc = generateCrc16(frame)
  frame.push(crc & 0xff, (crc >> 8) & 0xff)
  return frame
}

function parseResponse(frame: number[]) {
  registers.value = []
  if (isWriteCode(request.funcCode)) {
    for (let i = 0; i < request.quantity; i++) {
      registers.value.push({
        address: `0x${(request.startAddr + i).toString(16).toUpperCase().padStart(4, '0')}`,
        value: String(request.funcCode <= 5 ? (request.quantity ? 1 : 0) : request.quantity),
        hex: isWriteCode(request.funcCode)
          ? (request.quantity >> 8).toString(16).toUpperCase().padStart(2, '0') + (request.quantity & 0xff).toString(16).toUpperCase().padStart(2, '0')
          : '0000',
      })
    }
  } else {
    const dataStart = 3
    for (let i = 0; i < request.quantity; i++) {
      const hi = frame[dataStart + i * 2] || 0
      const lo = frame[dataStart + i * 2 + 1] || 0
      const val = (hi << 8) | lo
      registers.value.push({
        address: `0x${(request.startAddr + i).toString(16).toUpperCase().padStart(4, '0')}`,
        value: String(val),
        hex: hi.toString(16).toUpperCase().padStart(2, '0') + lo.toString(16).toUpperCase().padStart(2, '0'),
      })
    }
  }
}

function frameToHex(frame: number[]): string {
  return frame.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
}

// 处理完整的RTU响应帧（含CRC校验）
function processRtuFrame(frame: number[]) {
  if (frame.length < 4) {
    addLog(`RX 帧过短: ${frame.length} 字节`)
    return
  }
  // 校验CRC16
  const payload = frame.slice(0, -2)
  const receivedCrc = frame[frame.length - 2] | (frame[frame.length - 1] << 8)
  const calculatedCrc = generateCrc16(payload)
  if (receivedCrc !== calculatedCrc) {
    addLog(`RX CRC校验失败: 期望 0x${calculatedCrc.toString(16).toUpperCase()} 收到 0x${receivedCrc.toString(16).toUpperCase()}`)
    return
  }
  const rxHex = frameToHex(frame)
  addLog(`RX [${frameIndex.value}]: ${rxHex}`)
  addFrame('RX', rxHex)
  parseResponse(frame)
}

function sendRequest() {
  if (!connected.value) {
    addLog('错误: 串口未连接')
    return
  }
  const txFrame = buildRequestFrame()
  const txHex = frameToHex(txFrame)
  frameIndex.value++
  addLog(`TX [${frameIndex.value}]: ${txHex}`)
  addFrame('TX', txHex)

  // 清空缓冲区，设置响应超时
  rxBuffer = []
  if (rxTimer) { clearTimeout(rxTimer); rxTimer = null }
  if (responseTimer) { clearTimeout(responseTimer) }

  // 响应总超时 1000ms
  responseTimer = setTimeout(() => {
    addLog(`RX 响应超时 (帧 #${frameIndex.value})`)
    rxBuffer = []
    responseTimer = null
  }, 1000)

  // 通过串口发送
  serialWrite(connConfig.port, txFrame).then(result => {
    if (!result.success) {
      addLog(`发送失败: ${result.error}`)
      if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
    }
  })
}

function startPolling() {
  if (!connected.value) {
    addLog('错误: 串口未连接')
    return
  }
  running.value = true
  addLog('开始轮询...')
  doPoll()
}

function doPoll() {
  if (!running.value) return
  sendRequest()
  setTimeout(() => {
    if (running.value) doPoll()
  }, 500)
}

function stopPolling() {
  running.value = false
  addLog('停止轮询')
}

async function toggleConnection() {
  if (!connected.value) {
    const result = await serialOpen({
      path: connConfig.port,
      baudRate: connConfig.baudRate,
    })
    if (!result.success) {
      addLog(`串口打开失败: ${result.error}`)
      return
    }
    // 注册串口数据监听，缓冲接收RTU帧
    rxBuffer = []
    cleanupSerialData = onSerialData(connConfig.port, (data: number[]) => {
      rxBuffer.push(...data)
      // 重置帧超时定时器（3.5字符时间）
      if (rxTimer) clearTimeout(rxTimer)
      rxTimer = setTimeout(() => {
        if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
        processRtuFrame(rxBuffer)
        rxBuffer = []
      }, Math.max(5, Math.ceil(11 / connConfig.baudRate * 3.5 * 1000)))
    })
    connected.value = true
    addLog(`已连接 ${connConfig.port} @ ${connConfig.baudRate}`)
    addLog(`站地址: ${connConfig.station}`)
  } else {
    running.value = false
    if (cleanupSerialData) { cleanupSerialData(); cleanupSerialData = null }
    if (rxTimer) { clearTimeout(rxTimer); rxTimer = null }
    if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
    rxBuffer = []
    await serialClose(connConfig.port)
    connected.value = false
    addLog('已断开连接')
  }
}

function clearLog() {
  logMessages.value = []
}

function clearFrames() {
  rawFrames.value = []
  registers.value = []
  frameIndex.value = 0
}

function toHex(val: string): string {
  const n = parseInt(val)
  if (isNaN(n)) return val
  return '0x' + (n & 0xffff).toString(16).toUpperCase().padStart(4, '0')
}

// 初始化：加载串口列表
loadPorts()

// 组件卸载时清理资源
onUnmounted(() => {
  if (cleanupSerialData) cleanupSerialData()
  if (rxTimer) clearTimeout(rxTimer)
  if (responseTimer) clearTimeout(responseTimer)
  if (connected.value) {
    serialClose(connConfig.port)
  }
})
</script>

<template>
  <div class="modbus-rtu">
    <div class="toolbar">
      <label class="toolbar-label">串口:</label>
      <select v-model="connConfig.port" class="select-sm">
        <option v-if="serialPorts.length === 0" v-for="i in 8" :key="i" :value="'COM' + i">COM{{ i }}</option>
        <option v-for="p in serialPorts" :key="p.path" :value="p.path">{{ p.path }}</option>
      </select>
      <label class="toolbar-label">波特率:</label>
      <select v-model.number="connConfig.baudRate" class="select-sm">
        <option v-for="br in baudRates" :key="br" :value="br">{{ br }}</option>
      </select>
      <label class="toolbar-label">站地址:</label>
      <input v-model.number="connConfig.station" type="number" class="input-sm" style="width:56px" min="1" max="247" />
      <button class="btn" :class="{ 'btn-primary': !connected }" @click="toggleConnection">
        <span class="material-symbols-outlined" style="font-size:16px">{{ connected ? 'link_off' : 'link' }}</span>
        {{ connected ? '断开' : '连接' }}
      </button>
      <div class="toolbar-spacer" />
      <span v-if="running" class="running-indicator">
        <span class="material-symbols-outlined blink" style="font-size:14px">fiber_manual_record</span>
        运行中
      </span>
    </div>

    <div class="main-content">
      <div class="content-left">
        <!-- 功能码选择 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">code</span>
            功能码选择
          </div>
          <div class="panel-body">
            <div class="func-grid">
              <button
                v-for="fc in funcCodes"
                :key="fc.code"
                class="func-btn"
                :class="{ active: request.funcCode === fc.code, write: isWriteCode(fc.code) }"
                @click="request.funcCode = fc.code"
              >
                <span class="func-code">{{ fc.code }}</span>
                <span class="func-name">{{ fc.name.split(' - ')[1] }}</span>
                <span v-if="isWriteCode(fc.code)" class="write-badge">W</span>
              </button>
            </div>
          </div>
        </div>

        <!-- 请求参数 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">edit_note</span>
            请求参数
          </div>
          <div class="panel-body">
            <div class="form-row">
              <label>起始地址:</label>
              <input v-model.number="request.startAddr" type="number" class="input-sm" min="0" max="65535" />
              <span class="unit">({{ toHex(String(request.startAddr)) }})</span>
            </div>
            <div class="form-row">
              <label>数量:</label>
              <input v-model.number="request.quantity" type="number" class="input-sm" min="1" max="125" />
            </div>
            <div class="send-row">
              <button class="btn btn-primary" :disabled="!connected" @click="sendRequest">
                <span class="material-symbols-outlined" style="font-size:16px">send</span>
                发送请求
              </button>
              <button v-if="!running" class="btn" :disabled="!connected" @click="startPolling">
                <span class="material-symbols-outlined" style="font-size:16px">play_circle</span>
                轮询
              </button>
              <button v-else class="btn" @click="stopPolling">
                <span class="material-symbols-outlined" style="font-size:16px">stop_circle</span>
                停止
              </button>
            </div>
          </div>
        </div>

        <!-- 寄存器数据 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">table_chart</span>
            寄存器数据
          </div>
          <div class="panel-body table-wrap">
            <table class="data-table">
              <thead>
                <tr>
                  <th>地址</th>
                  <th>十进制</th>
                  <th>十六进制</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(reg, i) in registers" :key="i">
                  <td class="mono">{{ reg.address }}</td>
                  <td class="mono">{{ reg.value }}</td>
                  <td class="mono">{{ reg.hex }}</td>
                </tr>
                <tr v-if="registers.length === 0">
                  <td colspan="3" class="empty-cell">无数据</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <div class="content-right">
        <!-- 原始报文 -->
        <div class="panel raw-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">data_object</span>
            原始报文
            <div class="toolbar-spacer" />
            <button class="btn" @click="clearFrames" style="padding:2px 6px">
              <span class="material-symbols-outlined" style="font-size:14px">delete_sweep</span>
              清空
            </button>
          </div>
          <div class="panel-body raw-frames">
            <div v-for="(frame, i) in rawFrames" :key="i" class="frame-line" :class="frame.dir.toLowerCase()">
              <span class="frame-idx">{{ frame.time }}</span>
              <span class="frame-dir">{{ frame.dir }}</span>
              <span class="frame-hex">{{ frame.hex }}</span>
            </div>
            <div v-if="rawFrames.length === 0" class="log-empty">暂无报文</div>
          </div>
        </div>

        <!-- 日志 -->
        <div class="panel log-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">terminal</span>
            日志
            <div class="toolbar-spacer" />
            <button class="btn" @click="clearLog" style="padding:2px 6px">
              <span class="material-symbols-outlined" style="font-size:14px">delete_sweep</span>
              清空
            </button>
          </div>
          <div class="panel-body log-area">
            <div v-for="(msg, i) in logMessages" :key="i" class="log-line">{{ msg }}</div>
            <div v-if="logMessages.length === 0" class="log-empty">暂无日志</div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ connConfig.port }} | {{ connConfig.baudRate }} | 站 {{ connConfig.station }}</span>
      <span>状态: {{ connected ? '已连接' : '未连接' }}{{ running ? ' | 轮询中' : '' }}</span>
      <span>帧序号: {{ frameIndex }}</span>
    </div>
  </div>
</template>

<style scoped>
.modbus-rtu {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--surface);
  color: var(--on-surface);
  font-size: 12px;
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
  flex-wrap: wrap;
}

.toolbar-label {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.toolbar-spacer { flex: 1; }

.running-indicator {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: #f44336;
  font-size: 11px;
}

.blink {
  animation: blink-anim 1s infinite;
}

@keyframes blink-anim {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.2; }
}

.btn {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 4px 10px;
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  background: var(--surface-variant);
  color: var(--on-surface);
  font-size: 12px;
  cursor: pointer;
}

.btn-primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.btn:hover { opacity: 0.85; }
.btn:disabled { opacity: 0.4; cursor: not-allowed; }

.select-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
}

.input-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
  font-family: 'JetBrains Mono', monospace;
}

.main-content {
  display: flex;
  flex: 1;
  overflow: hidden;
}

.content-left {
  flex: 0 0 360px;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
  border-right: 1px solid var(--outline-variant);
}

.content-right {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel { border-bottom: 1px solid var(--outline-variant); }

.panel-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: var(--surface-container);
  font-weight: 600;
  font-size: 12px;
  border-bottom: 1px solid var(--outline-variant);
}

.panel-body { padding: 8px 10px; }

.func-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 4px;
}

.func-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 8px;
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  background: var(--surface-variant);
  color: var(--on-surface);
  font-size: 11px;
  cursor: pointer;
  position: relative;
}

.func-btn.active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.func-btn.write .func-code {
  color: #ff9800;
}

.func-btn.active.write .func-code {
  color: var(--on-primary);
}

.func-code {
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
  min-width: 20px;
}

.func-name { flex: 1; }

.write-badge {
  background: #ff9800;
  color: #000;
  font-size: 9px;
  padding: 1px 4px;
  border-radius: 3px;
  font-weight: 700;
}

.form-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
}

.form-row label:first-child {
  flex: 0 0 70px;
  text-align: right;
  color: var(--on-surface-variant);
  font-size: 12px;
}

.form-row .unit {
  color: var(--on-surface-variant);
  font-size: 11px;
  font-family: 'JetBrains Mono', monospace;
}

.send-row {
  display: flex;
  gap: 6px;
  margin-top: 8px;
}

.table-wrap {
  max-height: 200px;
  overflow-y: auto;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.data-table th {
  text-align: left;
  padding: 4px 8px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  position: sticky;
  top: 0;
}

.data-table td {
  padding: 3px 8px;
  border-bottom: 1px solid var(--outline-variant);
}

.mono {
  font-family: 'JetBrains Mono', monospace;
}

.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 16px !important;
}

.raw-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.raw-frames {
  flex: 1;
  overflow-y: auto;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  line-height: 1.6;
}

.frame-line {
  display: flex;
  gap: 8px;
  padding: 2px 4px;
}

.frame-line.tx {
  color: #42a5f5;
}

.frame-line.rx {
  color: #66bb6a;
}

.frame-idx {
  color: var(--on-surface-variant);
  min-width: 60px;
}

.frame-dir {
  font-weight: 700;
  min-width: 24px;
}

.frame-hex {
  word-break: break-all;
}

.log-panel {
  flex: 0 0 180px;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.log-area {
  flex: 1;
  overflow-y: auto;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  line-height: 1.6;
  padding: 6px 10px;
}

.log-line {
  white-space: pre-wrap;
  word-break: break-all;
}

.log-empty {
  color: var(--on-surface-variant);
  text-align: center;
  padding: 20px;
}

.status-bar {
  display: flex;
  justify-content: space-between;
  padding: 4px 12px;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
}
</style>
