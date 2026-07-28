<script setup lang="ts">
import { ref, onUnmounted } from 'vue'
import { udpCreateServer, udpServerSend, udpServerClose, onUdpServerData } from '@/serial/serialClient'

interface UdpPacket { id: number; timestamp: string; srcIp: string; srcPort: number; data: string }

const listenAddr = ref('0.0.0.0')
const listenPort = ref(5000)
const running = ref(false)
const packets = ref<UdpPacket[]>([])
const autoScroll = ref(true)
let nextPktId = 1
let cleanupData: (() => void) | null = null
const txCount = ref(0)
const rxCount = ref(0)
const targetAddr = ref('192.168.1.100')
const targetPort = ref(5000)
const sendFormat = ref<'ascii' | 'hex'>('ascii')
const sendDataStr = ref('')

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

async function startServer() {
  cleanupData = onUdpServerData((d) => {
    if (d.id !== 'udp-server') return
    const hexStr = Array.from(d.data).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
    packets.value.push({ id: nextPktId++, timestamp: getTimestamp(), srcIp: d.remoteAddress, srcPort: d.remotePort, data: hexStr })
    rxCount.value++
    if (packets.value.length > 500) packets.value = packets.value.slice(-400)
  })
  const result = await udpCreateServer('udp-server', listenPort.value, listenAddr.value)
  if (!result.success) return
  running.value = true
  rxCount.value = 0
  txCount.value = 0
}

async function stopServer() {
  await udpServerClose('udp-server')
  if (cleanupData) { cleanupData(); cleanupData = null }
  running.value = false
}

async function sendPacket() {
  if (!sendDataStr.value.trim()) return
  const bytes = Array.from(new TextEncoder().encode(sendDataStr.value))
  await udpServerSend('udp-server', bytes, targetPort.value, targetAddr.value)
  packets.value.push({ id: nextPktId++, timestamp: getTimestamp(), srcIp: '本机', srcPort: listenPort.value, data: `→ ${targetAddr.value}:${targetPort.value} | ${sendDataStr.value}` })
  txCount.value++
  sendDataStr.value = ''
}

function clearPackets() { packets.value = []; rxCount.value = 0; txCount.value = 0 }

onUnmounted(() => { udpServerClose('udp-server'); if (cleanupData) cleanupData() })
</script>

<template>
  <div class="udp-server">
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
        <button class="btn" @click="clearPackets">
          <span class="material-symbols-outlined" style="font-size:14px">delete</span>
          清空
        </button>
      </div>
      <div class="toolbar-spacer" />
      <label class="checkbox-label">
        <input type="checkbox" v-model="autoScroll" /> 自动滚动
      </label>
    </div>

    <div class="packet-list">
      <table class="data-table">
        <thead>
          <tr>
            <th style="width:120px">时间</th>
            <th style="width:130px">来源</th>
            <th>数据</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="pkt in packets" :key="pkt.id">
            <td class="mono">{{ pkt.timestamp }}</td>
            <td class="mono">{{ pkt.srcIp }}:{{ pkt.srcPort }}</td>
            <td class="mono" style="font-size:11px">{{ pkt.data }}</td>
          </tr>
          <tr v-if="packets.length === 0">
            <td colspan="3" class="empty-cell">无数据包</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="send-section">
      <div class="section-title">发送区域</div>
      <div class="send-row">
        <label class="toolbar-label">目标地址:</label>
        <input v-model="targetAddr" class="input-sm" style="width:120px" />
        <label class="toolbar-label">端口:</label>
        <input v-model.number="targetPort" type="number" class="input-sm" style="width:70px" />
        <select v-model="sendFormat" class="select-sm">
          <option value="ascii">ASCII</option>
          <option value="hex">HEX</option>
        </select>
        <input v-model="sendDataStr" class="input-sm" style="flex:1" placeholder="发送数据" @keydown.enter="sendPacket" />
        <button class="btn btn-primary" @click="sendPacket">
          <span class="material-symbols-outlined" style="font-size:14px">send</span>
          发送
        </button>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ running ? `监听 ${listenAddr}:${listenPort}` : '未启动' }}</span>
      <span>收包: {{ rxCount }}</span>
      <span>发包: {{ txCount }}</span>
      <span>列表: {{ packets.length }}</span>
    </div>
  </div>
</template>

<style scoped>
.udp-server {
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
.checkbox-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.mono { font-family: 'JetBrains Mono', monospace; }
.packet-list {
  flex: 1;
  overflow-y: auto;
  min-height: 0;
}
.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}
.data-table th {
  text-align: left;
  padding: 6px 8px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  position: sticky;
  top: 0;
  z-index: 1;
}
.data-table td {
  padding: 4px 8px;
  border-bottom: 1px solid var(--outline-variant);
}
.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 40px !important;
}
.send-section {
  padding: 8px 12px;
  border-top: 1px solid var(--outline-variant);
  background: var(--surface-container);
  flex-shrink: 0;
}
.section-title {
  font-weight: 600;
  font-size: 12px;
  margin-bottom: 6px;
}
.send-row {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
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
