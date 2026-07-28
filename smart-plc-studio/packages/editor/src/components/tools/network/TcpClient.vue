<script setup lang="ts">
import { ref, onUnmounted } from 'vue'
import { tcpConnect, tcpDisconnect, tcpSend, onTcpData, onTcpClosed, onTcpError } from '@/serial/serialClient'

interface TerminalLine { timestamp: string; direction: 'tx' | 'rx'; data: string }

const serverAddr = ref('192.168.1.100')
const serverPort = ref(9000)
const connected = ref(false)
const connecting = ref(false)
const inputFormat = ref<'ascii' | 'hex'>('ascii')
const inputData = ref('')
const terminalLines = ref<TerminalLine[]>([])
const txCount = ref(0)
const rxCount = ref(0)
const autoReconnect = ref(false)
const heartbeatEnabled = ref(false)
const heartbeatInterval = ref(1000)
const heartbeatContent = ref('PING')
let heartbeatTimer: ReturnType<typeof setInterval> | null = null
let reconnectTimer: ReturnType<typeof setTimeout> | null = null
let cleanupData: (() => void) | null = null
let cleanupClose: (() => void) | null = null
let cleanupError: (() => void) | null = null
const clientClientId = 'tcp-client-' + Date.now()

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

function addLine(direction: 'tx' | 'rx', data: string) {
  terminalLines.value.push({ timestamp: getTimestamp(), direction, data })
  if (terminalLines.value.length > 1000) terminalLines.value = terminalLines.value.slice(-800)
}

async function connect() {
  connecting.value = true
  cleanupData = onTcpData((d) => {
    if (d.id !== clientClientId) return
    const text = new TextDecoder().decode(new Uint8Array(d.data))
    addLine('rx', text)
    rxCount.value += text.length
  })
  cleanupClose = onTcpClosed((d) => {
    if (d.id !== clientClientId) return
    connected.value = false
    addLine('rx', '[连接已关闭]')
    if (autoReconnect.value) scheduleReconnect()
  })
  cleanupError = onTcpError((d) => {
    if (d.id !== clientClientId) return
    addLine('rx', `[错误: ${d.error}]`)
  })
  
  const result = await tcpConnect(clientClientId, serverAddr.value, serverPort.value)
  connecting.value = false
  if (result.success) {
    connected.value = true
    addLine('rx', `[已连接到 ${serverAddr.value}:${serverPort.value}]`)
    if (heartbeatEnabled.value) startHeartbeat()
  } else {
    addLine('rx', `[连接失败: ${result.error}]`)
    if (autoReconnect.value) scheduleReconnect()
  }
}

function disconnect() {
  tcpDisconnect(clientClientId)
  connected.value = false
  connecting.value = false
  stopHeartbeat()
  if (cleanupData) { cleanupData(); cleanupData = null }
  if (cleanupClose) { cleanupClose(); cleanupClose = null }
  if (cleanupError) { cleanupError(); cleanupError = null }
  addLine('rx', '[已断开连接]')
}

function scheduleReconnect() {
  reconnectTimer = setTimeout(() => {
    if (!connected.value && autoReconnect.value) {
      addLine('rx', '[自动重连中...]')
      connect()
    }
  }, 3000)
}

function startHeartbeat() {
  stopHeartbeat()
  heartbeatTimer = setInterval(() => {
    if (connected.value && heartbeatEnabled.value) sendData(heartbeatContent.value)
  }, heartbeatInterval.value)
}

function stopHeartbeat() {
  if (heartbeatTimer) { clearInterval(heartbeatTimer); heartbeatTimer = null }
}

async function sendData(data?: string) {
  const msg = data || inputData.value
  if (!msg.trim() || !connected.value) return
  addLine('tx', msg)
  txCount.value += msg.length
  if (!data) inputData.value = ''
  const bytes = Array.from(new TextEncoder().encode(msg))
  await tcpSend(clientClientId, bytes)
}

function clearTerminal() { terminalLines.value = []; txCount.value = 0; rxCount.value = 0 }

onUnmounted(() => {
  if (heartbeatTimer) clearInterval(heartbeatTimer)
  if (reconnectTimer) clearTimeout(reconnectTimer)
  tcpDisconnect(clientClientId)
  if (cleanupData) cleanupData()
  if (cleanupClose) cleanupClose()
  if (cleanupError) cleanupError()
})
</script>

<template>
  <div class="tcp-client">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">服务器地址:</label>
        <input v-model="serverAddr" class="input-sm" style="width:120px" />
        <label class="toolbar-label">端口:</label>
        <input v-model.number="serverPort" type="number" class="input-sm" style="width:70px" min="1" max="65535" />
      </div>
      <div class="toolbar-group">
        <span :class="['status-light', connected ? 'on' : connecting ? 'connecting' : 'off']"></span>
        <button :class="['btn', connected ? 'btn-danger' : 'btn-success']" @click="connected ? disconnect() : connect()" :disabled="connecting">
          <span class="material-symbols-outlined" style="font-size:14px">{{ connected ? 'link_off' : 'link' }}</span>
          {{ connecting ? '连接中...' : connected ? '断开' : '连接' }}
        </button>
        <label class="checkbox-label">
          <input type="checkbox" v-model="autoReconnect" /> 自动重连
        </label>
      </div>
    </div>

    <div class="heartbeat-bar">
      <label class="checkbox-label">
        <input type="checkbox" v-model="heartbeatEnabled" @change="connected && heartbeatEnabled ? startHeartbeat() : stopHeartbeat()" />
        心跳包
      </label>
      <label class="toolbar-label">间隔:</label>
      <input v-model.number="heartbeatInterval" type="number" class="input-sm" style="width:60px" min="100" max="10000" :disabled="!heartbeatEnabled" />
      <span class="toolbar-label">ms</span>
      <label class="toolbar-label">内容:</label>
      <input v-model="heartbeatContent" class="input-sm" style="width:80px" :disabled="!heartbeatEnabled" />
    </div>

    <div class="terminal-container">
      <div v-for="(line, idx) in terminalLines" :key="idx" :class="['terminal-line', line.direction]">
        <span class="timestamp">{{ line.timestamp }}</span>
        <span class="direction">{{ line.direction === 'tx' ? 'TX' : 'RX' }}</span>
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
        :disabled="!connected"
        placeholder="输入发送数据..."
        @keydown.enter="sendData()"
      />
      <button class="btn btn-primary" @click="sendData()" :disabled="!connected">
        <span class="material-symbols-outlined" style="font-size:14px">send</span>
        发送
      </button>
      <button class="btn" @click="clearTerminal">清空</button>
    </div>

    <div class="status-bar">
      <span>{{ connected ? `已连接 ${serverAddr}:${serverPort}` : '未连接' }}</span>
      <span>TX: {{ txCount }} bytes</span>
      <span>RX: {{ rxCount }} bytes</span>
    </div>
  </div>
</template>

<style scoped>
.tcp-client {
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
  gap: 8px;
  padding: 6px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
  flex-wrap: wrap;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.toolbar-label { font-size: 11px; color: var(--on-surface-variant); }
.toolbar-spacer { flex: 1; }
.heartbeat-bar {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}
.status-light {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}
.status-light.on { background: #4caf50; box-shadow: 0 0 6px #4caf50; }
.status-light.off { background: #9e9e9e; }
.status-light.connecting { background: #ff9800; animation: pulse 1s infinite; }
@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.3; } }
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
.btn:hover { opacity: 0.85; }
.btn:disabled { opacity: 0.4; cursor: not-allowed; }
.btn-primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-danger { background: #c62828; color: white; border-color: #c62828; }
.btn-success { background: #2e7d32; color: white; border-color: #2e7d32; }
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
.input-sm:focus { border-color: var(--primary); outline: none; }
.input-sm:disabled { opacity: 0.4; }
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
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  background: #1e1e1e;
  min-height: 0;
}
.terminal-line {
  display: flex;
  gap: 8px;
  padding: 2px 0;
  white-space: pre-wrap;
  word-break: break-all;
}
.terminal-line .timestamp { color: #666; flex-shrink: 0; }
.terminal-line .direction { font-weight: bold; flex-shrink: 0; width: 24px; }
.terminal-line.tx .direction { color: #2196f3; }
.terminal-line.rx .direction { color: #4caf50; }
.terminal-line .content { color: #d4d4d4; }
.terminal-line.tx .content { color: #2196f3; }
.terminal-line.rx .content { color: #4caf50; }
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
.input-field:focus { border-color: var(--primary); }
.input-field:disabled { opacity: 0.4; }
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
