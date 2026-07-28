<script setup lang="ts">
import { ref, onUnmounted } from 'vue'
import { udpCreateClient, udpClientBind, udpClientSend, udpClientClose, onUdpClientData } from '@/serial/serialClient'

interface TerminalLine { timestamp: string; direction: 'tx' | 'rx'; data: string }

const targetAddr = ref('192.168.1.100')
const targetPort = ref(5000)
const localPort = ref(0)
const bound = ref(false)
const broadcastMode = ref(false)
const inputFormat = ref<'ascii' | 'hex'>('ascii')
const sendDataStr = ref('')
const terminalLines = ref<TerminalLine[]>([])
const txCount = ref(0)
const rxCount = ref(0)
let cleanupData: (() => void) | null = null
const clientId = 'udp-client-' + Date.now()

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

function addLine(direction: 'tx' | 'rx', data: string) {
  terminalLines.value.push({ timestamp: getTimestamp(), direction, data })
  if (terminalLines.value.length > 500) terminalLines.value = terminalLines.value.slice(-400)
}

async function bindSocket() {
  await udpCreateClient(clientId)
  cleanupData = onUdpClientData((d) => {
    if (d.id !== clientId) return
    const text = new TextDecoder().decode(new Uint8Array(d.data))
    addLine('rx', `[${d.remoteAddress}:${d.remotePort}] ${text}`)
    rxCount.value += text.length
  })
  const result = await udpClientBind(clientId, 0)
  if (!result.success) { addLine('rx', `绑定失败: ${result.error}`); return }
  localPort.value = result.port || 0
  bound.value = true
  addLine('rx', `[已绑定本地端口 ${localPort.value}]`)
}

async function unbindSocket() {
  await udpClientClose(clientId)
  if (cleanupData) { cleanupData(); cleanupData = null }
  bound.value = false
  localPort.value = 0
  addLine('rx', '[已取消绑定]')
}

async function sendData() {
  if (!sendDataStr.value.trim()) return
  const target = broadcastMode.value ? `广播 → :${targetPort.value}` : `${targetAddr.value}:${targetPort.value}`
  addLine('tx', `[${target}] ${sendDataStr.value}`)
  txCount.value += sendDataStr.value.length
  const bytes = Array.from(new TextEncoder().encode(sendDataStr.value))
  await udpClientSend(clientId, bytes, targetPort.value, targetAddr.value)
  sendDataStr.value = ''
}

function clearTerminal() { terminalLines.value = []; txCount.value = 0; rxCount.value = 0 }

onUnmounted(() => { udpClientClose(clientId); if (cleanupData) cleanupData() })
</script>

<template>
  <div class="udp-client">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">目标地址:</label>
        <input v-model="targetAddr" class="input-sm" style="width:120px" :disabled="broadcastMode" />
        <label class="toolbar-label">端口:</label>
        <input v-model.number="targetPort" type="number" class="input-sm" style="width:70px" />
      </div>
      <div class="toolbar-group">
        <label class="checkbox-label">
          <input type="checkbox" v-model="broadcastMode" :disabled="bound" />
          广播模式
        </label>
        <button :class="['btn', bound ? 'btn-danger' : 'btn-success']" @click="bound ? unbindSocket() : bindSocket()">
          <span class="material-symbols-outlined" style="font-size:14px">{{ bound ? 'link_off' : 'link' }}</span>
          {{ bound ? '取消绑定' : '绑定' }}
        </button>
      </div>
      <div class="toolbar-spacer" />
      <span class="toolbar-label" v-if="bound">本地端口: {{ localPort }}</span>
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
        v-model="sendDataStr"
        class="input-field"
        placeholder="输入发送数据..."
        @keydown.enter="sendData"
      />
      <button class="btn btn-primary" @click="sendData">
        <span class="material-symbols-outlined" style="font-size:14px">send</span>
        发送
      </button>
      <button class="btn" @click="clearTerminal">清空</button>
    </div>

    <div class="status-bar">
      <span>{{ bound ? `已绑定 本地:${localPort}` : '未绑定' }}</span>
      <span>TX: {{ txCount }} bytes</span>
      <span>RX: {{ rxCount }} bytes</span>
    </div>
  </div>
</template>

<style scoped>
.udp-client {
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
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.toolbar-label { font-size: 11px; color: var(--on-surface-variant); }
.toolbar-spacer { flex: 1; }
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
