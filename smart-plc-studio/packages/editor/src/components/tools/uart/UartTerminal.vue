<template>
  <div class="uart-terminal">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">串口:</label>
        <select v-model="portPath" class="select-sm" style="width:120px">
          <option v-if="portList.length === 0" value="" disabled>未检测到串口</option>
          <option v-for="p in portList" :key="p.path" :value="p.path">
            {{ p.path }}{{ p.manufacturer ? ' — ' + p.manufacturer : '' }}
          </option>
        </select>
      </div>
      <div class="toolbar-group">
        <select v-model="portConfig.baudRate" class="select-sm">
          <option v-for="rate in baudRates" :key="rate" :value="rate">{{ rate }}</option>
        </select>
        <select v-model="portConfig.dataBits" class="select-sm">
          <option value="5">5</option>
          <option value="6">6</option>
          <option value="7">7</option>
          <option value="8">8</option>
        </select>
        <select v-model="portConfig.stopBits" class="select-sm">
          <option value="1">1</option>
          <option value="1.5">1.5</option>
          <option value="2">2</option>
        </select>
        <select v-model="portConfig.parity" class="select-sm">
          <option value="none">None</option>
          <option value="even">Even</option>
          <option value="odd">Odd</option>
        </select>
      </div>
      <div class="toolbar-group">
        <button :class="['btn', isConnected ? 'btn-danger' : 'btn-success']" @click="toggleConnection">
          <span class="material-symbols-outlined">{{ isConnected ? 'link_off' : 'link' }}</span>
          {{ isConnected ? '断开' : '连接' }}
        </button>
        <button class="btn" @click="clearTerminal">
          <span class="material-symbols-outlined">delete</span>
          清空
        </button>
        <label class="checkbox-label">
          <input type="checkbox" v-model="autoScroll" /> 自动滚动
        </label>
      </div>
    </div>
    <div class="terminal-container" ref="terminalRef">
      <div v-for="(line, idx) in terminalLines" :key="idx" :class="['terminal-line', line.type]">
        <span class="timestamp">{{ line.timestamp }}</span>
        <span class="direction" :title="line.type === 'rx' ? '接收' : '发送'">
          {{ line.type === 'rx' ? '←' : '→' }}
        </span>
        <span class="content">{{ line.data }}</span>
      </div>
    </div>
    <div class="input-area">
      <select v-model="inputFormat" class="select-sm">
        <option value="ascii">ASCII</option>
        <option value="hex">HEX</option>
      </select>
      <input
        v-model="inputData"
        class="input-field"
        :placeholder="inputFormat === 'hex' ? '输入HEX数据，如 01 03 00 00 00 0A' : '输入ASCII数据'"
        @keydown.enter="sendData"
      />
      <button class="btn btn-primary" @click="sendData" :disabled="!isConnected">
        <span class="material-symbols-outlined">send</span>
        发送
      </button>
    </div>
    <div class="status-bar">
      <span>TX={{ txCount }} / RX={{ rxCount }}</span>
      <span>{{ isConnected ? '已连接' : '未连接' }}</span>
      <span style="color:#888;font-size:11px">{{ debugInfo }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick, onMounted, onUnmounted } from 'vue'
import { serialOpen, serialClose, serialWrite, serialListPorts, onSerialData, onSerialStatus, onSerialError, ensureWsConnected, type PortInfo } from '@/serial/serialClient'

interface TerminalLine { timestamp: string; type: 'rx' | 'tx'; data: string }

const baudRates = [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
const portConfig = ref({ baudRate: 115200, dataBits: '8', stopBits: '1', parity: 'none' })
const isConnected = ref(false)
const autoScroll = ref(true)
const terminalLines = ref<TerminalLine[]>([])
const inputData = ref('')
const inputFormat = ref<'ascii' | 'hex'>('ascii')
const txCount = ref(0)
const rxCount = ref(0)
const terminalRef = ref<HTMLElement | null>(null)
const portPath = ref('')
const portList = ref<PortInfo[]>([])
let cleanupData: (() => void) | null = null
let cleanupStatus: (() => void) | null = null
let cleanupError: (() => void) | null = null
const debugInfo = ref('')

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

async function toggleConnection() {
  if (!isConnected.value) {
    // 先注册数据、状态、错误监听
    cleanupData = onSerialData('*', (evt) => {
      console.log('[Terminal] 收到 serial:data', evt)
      const text = new TextDecoder().decode(new Uint8Array(evt.data))
      terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: text })
      rxCount.value += text.length
      scrollToBottom()
    })
    cleanupStatus = onSerialStatus((d) => {
      if (d.port === portPath.value && !d.connected) {
        isConnected.value = false
        terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: '[串口已断开]' })
      }
    })
    cleanupError = onSerialError((d) => {
      terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: `[串口错误: ${d.error}]` })
    })

    // 确保 WebSocket 已连接（接收通道）
    debugInfo.value = '正在连接 WebSocket...'
    terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: '[正在连接 WebSocket...]' })
    const wsOk = await ensureWsConnected()
    if (!wsOk) {
      terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: '[错误: WebSocket 连接失败，无法接收数据]' })
      debugInfo.value = 'WS 连接失败'
      if (cleanupData) { cleanupData(); cleanupData = null }
      if (cleanupStatus) { cleanupStatus(); cleanupStatus = null }
      if (cleanupError) { cleanupError(); cleanupError = null }
      return
    }
    debugInfo.value = 'WS 已连接'

    // 再开串口
    const result = await serialOpen({ path: portPath.value, baudRate: portConfig.value.baudRate, dataBits: parseInt(portConfig.value.dataBits) as any, stopBits: parseFloat(portConfig.value.stopBits) as any, parity: portConfig.value.parity as any })
    if (result.success) {
      isConnected.value = true
      terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: `[已连接 ${portPath.value} ${portConfig.value.baudRate}bps]` })
    } else {
      terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: `[连接失败: ${result.error}]` })
      debugInfo.value = `串口打开失败: ${result.error}`
      if (cleanupData) { cleanupData(); cleanupData = null }
      if (cleanupStatus) { cleanupStatus(); cleanupStatus = null }
      if (cleanupError) { cleanupError(); cleanupError = null }
    }
  } else {
    await serialClose(portPath.value)
    if (cleanupData) { cleanupData(); cleanupData = null }
    if (cleanupStatus) { cleanupStatus(); cleanupStatus = null }
    if (cleanupError) { cleanupError(); cleanupError = null }
    isConnected.value = false
    terminalLines.value.push({ timestamp: getTimestamp(), type: 'rx', data: '[已断开]' })
  }
  scrollToBottom()
}

function clearTerminal() { terminalLines.value = []; txCount.value = 0; rxCount.value = 0 }

async function sendData() {
  if (!inputData.value.trim()) return
  let data = inputData.value
  terminalLines.value.push({ timestamp: getTimestamp(), type: 'tx', data })
  txCount.value += data.length
  if (isConnected.value) {
    let bytes: number[]
    if (inputFormat.value === 'hex') {
      bytes = data.replace(/\s+/g, ' ').trim().split(' ').map(h => parseInt(h, 16)).filter(n => !isNaN(n))
    } else {
      bytes = Array.from(new TextEncoder().encode(data))
    }
    await serialWrite(portPath.value, bytes)
  }
  inputData.value = ''
  scrollToBottom()
}

function scrollToBottom() {
  if (autoScroll.value) {
    nextTick(() => { if (terminalRef.value) terminalRef.value.scrollTop = terminalRef.value.scrollHeight })
  }
}

async function loadPorts() {
  try {
    portList.value = await serialListPorts()
    if (portList.value.length > 0 && !portList.value.some((p) => p.path === portPath.value)) {
      portPath.value = portList.value[0].path
    }
  } catch {
    portList.value = []
  }
}

onMounted(loadPorts)

onUnmounted(() => {
  serialClose(portPath.value)
  if (cleanupData) cleanupData()
  if (cleanupStatus) cleanupStatus()
})
</script>

<style scoped>
.uart-terminal {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--surface);
}
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
  flex-wrap: wrap;
  gap: 8px;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.toolbar-label {
  font-size: 12px;
  color: var(--on-surface-variant);
}
.select-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
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
.btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.btn .material-symbols-outlined {
  font-size: 14px;
}
.btn-success {
  background: #2e7d32;
  color: white;
  border-color: #2e7d32;
}
.btn-danger {
  background: #c62828;
  color: white;
  border-color: #c62828;
}
.btn-primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}
.checkbox-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.terminal-container {
  flex: 1;
  overflow-y: auto;
  padding: 8px 12px;
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  font-size: 12px;
  background: #1e1e1e;
}
.terminal-line {
  display: flex;
  gap: 8px;
  padding: 2px 0;
  white-space: pre-wrap;
  word-break: break-all;
}
.terminal-line .timestamp {
  color: #666;
  flex-shrink: 0;
}
.terminal-line .direction {
  font-weight: bold;
  flex-shrink: 0;
  width: 14px;
  text-align: center;
}
.terminal-line.rx .direction {
  color: #4caf50;
}
.terminal-line.tx .direction {
  color: #2196f3;
}
.terminal-line .content {
  color: #d4d4d4;
}
.terminal-line.rx .content {
  color: #4caf50;
}
.terminal-line.tx .content {
  color: #2196f3;
}
.input-area {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  border-top: 1px solid var(--outline-variant);
}
.input-field {
  flex: 1;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 6px 10px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  outline: none;
}
.input-field:focus {
  border-color: var(--primary);
}
.status-bar {
  display: flex;
  justify-content: space-between;
  padding: 4px 12px;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
}
</style>
