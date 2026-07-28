<script setup lang="ts">
import { ref, computed, onUnmounted } from 'vue'
import { tcpCreateServer, tcpCloseServer, tcpServerSend, onTcpServerData, onTcpServerClientConnected, onTcpServerClientDisconnected } from '@/serial/serialClient'

interface Client { id: string; ip: string; port: number; connectTime: string; txBytes: number; rxBytes: number; active: boolean }
interface TerminalLine { timestamp: string; direction: 'tx' | 'rx'; clientId: string; data: string }

const listenAddr = ref('0.0.0.0')
const listenPort = ref(9000)
const running = ref(false)
const clients = ref<Client[]>([])
const selectedClientId = ref<string | null>(null)
const terminalLines = ref<TerminalLine[]>([])
const inputFormat = ref<'ascii' | 'hex'>('ascii')
const inputData = ref('')
const autoScroll = ref(true)
let cleanupData: (() => void) | null = null
let cleanupConnected: (() => void) | null = null
let cleanupDisconnected: (() => void) | null = null

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

async function startServer() {
  cleanupData = onTcpServerData((d) => {
    console.log('[TcpServer] 收到数据事件:', d)
    if (d.serverId !== 'tcp-server') return
    const text = new TextDecoder().decode(new Uint8Array(d.data))
    terminalLines.value.push({ timestamp: getTimestamp(), direction: 'rx', clientId: d.clientId, data: text })
    const c = clients.value.find(cv => cv.id === d.clientId)
    if (c) c.rxBytes += text.length
    if (terminalLines.value.length > 1000) terminalLines.value = terminalLines.value.slice(-800)
  })
  cleanupConnected = onTcpServerClientConnected((d) => {
    console.log('[TcpServer] 客户端连接事件:', d)
    if (d.serverId !== 'tcp-server') return
    clients.value.push({ id: d.clientId, ip: d.remoteAddress, port: d.remotePort, connectTime: getTimestamp(), txBytes: 0, rxBytes: 0, active: true })
  })
  cleanupDisconnected = onTcpServerClientDisconnected((d) => {
    console.log('[TcpServer] 客户端断开事件:', d)
    if (d.serverId !== 'tcp-server') return
    clients.value = clients.value.filter(c => c.id !== d.clientId)
    if (selectedClientId.value === d.clientId) selectedClientId.value = null
  })
  const result = await tcpCreateServer('tcp-server', listenPort.value, listenAddr.value)
  if (!result.success) { addLog('启动失败: ' + result.error); return }
  running.value = true
}

function addLog(msg: string) {
  terminalLines.value.push({ timestamp: getTimestamp(), direction: 'rx', clientId: 'system', data: msg })
}

async function stopServer() {
  await tcpCloseServer('tcp-server')
  running.value = false
  clients.value = []
  selectedClientId.value = null
  if (cleanupData) { cleanupData(); cleanupData = null }
  if (cleanupConnected) { cleanupConnected(); cleanupConnected = null }
  if (cleanupDisconnected) { cleanupDisconnected(); cleanupDisconnected = null }
}

async function sendToClient() {
  if (!selectedClientId.value || !inputData.value.trim()) return
  terminalLines.value.push({ timestamp: getTimestamp(), direction: 'tx', clientId: selectedClientId.value, data: inputData.value })
  const client = clients.value.find(c => c.id === selectedClientId.value)
  if (client) client.txBytes += inputData.value.length
  const bytes = Array.from(new TextEncoder().encode(inputData.value))
  await tcpServerSend('tcp-server', selectedClientId.value, bytes)
  inputData.value = ''
}

function disconnectClient(clientId: string) {
  clients.value = clients.value.filter(c => c.id !== clientId)
  if (selectedClientId.value === clientId) selectedClientId.value = null
}

function selectClient(clientId: string) { selectedClientId.value = clientId }

const selectedClient = computed(() => clients.value.find(c => c.id === selectedClientId.value) || null)
const filteredLines = computed(() => selectedClientId.value ? terminalLines.value.filter(l => l.clientId === selectedClientId.value) : terminalLines.value)

function clearTerminal() { terminalLines.value = [] }

onUnmounted(() => {
  tcpCloseServer('tcp-server')
  if (cleanupData) cleanupData()
  if (cleanupConnected) cleanupConnected()
  if (cleanupDisconnected) cleanupDisconnected()
})
</script>

<template>
  <div class="tcp-server">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">监听地址:</label>
        <input v-model="listenAddr" class="input-sm" style="width:100px" />
        <label class="toolbar-label">端口:</label>
        <input v-model.number="listenPort" type="number" class="input-sm" style="width:70px" min="1" max="65535" />
      </div>
      <div class="toolbar-group">
        <button :class="['btn', running ? 'btn-danger' : 'btn-success']" @click="running ? stopServer() : startServer()">
          <span class="material-symbols-outlined" style="font-size:14px">{{ running ? 'stop' : 'play_arrow' }}</span>
          {{ running ? '停止' : '启动' }}
        </button>
      </div>
    </div>

    <div class="main-area">
      <div class="client-panel">
        <div class="panel-header">
          <span class="material-symbols-outlined" style="font-size:16px">devices</span>
          已连接客户端 ({{ clients.length }})
        </div>
        <div class="client-list">
          <div
            v-for="c in clients"
            :key="c.id"
            :class="['client-item', { active: selectedClientId === c.id }]"
            @click="selectClient(c.id)"
          >
            <div class="client-info">
              <span class="client-ip mono">{{ c.ip }}:{{ c.port }}</span>
              <span class="client-time">{{ c.connectTime }}</span>
            </div>
            <div class="client-stats">
              <span class="stat-tx">TX {{ c.txBytes }}</span>
              <span class="stat-rx">RX {{ c.rxBytes }}</span>
            </div>
            <button class="btn-icon" @click.stop="disconnectClient(c.id)" title="断开">
              <span class="material-symbols-outlined" style="font-size:12px">link_off</span>
            </button>
          </div>
          <div v-if="clients.length === 0" class="empty-hint">无连接</div>
        </div>
      </div>

      <div class="terminal-panel">
        <div class="panel-header">
          <span class="material-symbols-outlined" style="font-size:16px">terminal</span>
          数据终端
          <div class="toolbar-spacer" />
          <button class="btn" @click="clearTerminal" style="font-size:11px">清空</button>
        </div>
        <div class="terminal-container">
          <div v-for="(line, idx) in filteredLines" :key="idx" :class="['terminal-line', line.direction]">
            <span class="timestamp">{{ line.timestamp }}</span>
            <span class="direction">{{ line.direction === 'tx' ? 'TX' : 'RX' }}</span>
            <span class="content">{{ line.data }}</span>
          </div>
          <div v-if="filteredLines.length === 0" class="empty-hint">选择客户端查看数据</div>
        </div>
        <div class="input-area">
          <select v-model="inputFormat" class="select-sm">
            <option value="ascii">ASCII</option>
            <option value="hex">HEX</option>
          </select>
          <input
            v-model="inputData"
            class="input-field"
            :disabled="!selectedClientId"
            :placeholder="selectedClient ? `发送到 ${selectedClient.ip}:${selectedClient.port}` : '选择客户端'"
            @keydown.enter="sendToClient"
          />
          <button class="btn btn-primary" @click="sendToClient" :disabled="!selectedClientId">
            <span class="material-symbols-outlined" style="font-size:14px">send</span>
            发送
          </button>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ running ? `监听 ${listenAddr}:${listenPort}` : '未启动' }}</span>
      <span>客户端: {{ clients.length }}</span>
      <span>日志: {{ terminalLines.length }}</span>
    </div>
  </div>
</template>

<style scoped>
.tcp-server {
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
.btn:disabled { opacity: 0.4; cursor: not-allowed; }
.btn-primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-danger { background: #c62828; color: white; border-color: #c62828; }
.btn-success { background: #2e7d32; color: white; border-color: #2e7d32; }
.btn-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.btn-icon:hover { background: rgba(244, 67, 54, 0.15); color: #f44336; }
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
.mono { font-family: 'JetBrains Mono', monospace; }
.main-area {
  flex: 1;
  display: flex;
  overflow: hidden;
  min-height: 0;
}
.client-panel {
  width: 240px;
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  flex-shrink: 0;
}
.panel-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  font-size: 12px;
}
.client-list {
  flex: 1;
  overflow-y: auto;
}
.client-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  cursor: pointer;
  border-bottom: 1px solid var(--outline-variant);
  transition: background 0.15s;
}
.client-item:hover { background: var(--surface-variant); }
.client-item.active { background: rgba(var(--primary-rgb, 33, 150, 243), 0.12); border-left: 3px solid var(--primary, #2196f3); }
.client-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.client-ip { font-size: 11px; font-weight: 600; }
.client-time { font-size: 10px; color: var(--on-surface-variant); }
.client-stats {
  display: flex;
  flex-direction: column;
  gap: 1px;
  font-size: 10px;
}
.stat-tx { color: #2196f3; }
.stat-rx { color: #4caf50; }
.terminal-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.terminal-container {
  flex: 1;
  overflow-y: auto;
  padding: 8px 12px;
  font-family: 'JetBrains Mono', monospace;
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
.terminal-line .timestamp { color: #666; flex-shrink: 0; }
.terminal-line .direction { font-weight: bold; flex-shrink: 0; width: 24px; }
.terminal-line.tx .direction { color: #2196f3; }
.terminal-line.rx .direction { color: #4caf50; }
.terminal-line .content { color: #d4d4d4; }
.terminal-line.tx .content { color: #2196f3; }
.terminal-line.rx .content { color: #4caf50; }
.empty-hint { text-align: center; color: var(--on-surface-variant); padding: 30px; font-family: inherit; }
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
