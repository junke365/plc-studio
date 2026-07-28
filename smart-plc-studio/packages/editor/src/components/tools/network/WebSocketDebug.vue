<script setup lang="ts">
import { ref, onUnmounted } from 'vue'
import { wsConnect, wsSend, wsDisconnect, onWsConnected, onWsDisconnected, onWsMessage, onWsError } from '@/serial/serialClient'

interface WsMessage { id: number; timestamp: string; direction: 'tx' | 'rx'; type: 'text' | 'binary' | 'event'; content: string }
interface WsEvent { id: number; timestamp: string; type: string; detail: string }

const url = ref('ws://192.168.1.100:8080')
const connected = ref(false)
const connecting = ref(false)
const messageType = ref<'text' | 'binary'>('text')
const sendDataStr = ref('')
const messages = ref<WsMessage[]>([])
const events = ref<WsEvent[]>([])
const txCount = ref(0)
const rxCount = ref(0)
const txMsgCount = ref(0)
const rxMsgCount = ref(0)
let nextMsgId = 1
let nextEvtId = 1
let cleanupConnected: (() => void) | null = null
let cleanupDisconnected: (() => void) | null = null
let cleanupMessage: (() => void) | null = null
let cleanupError: (() => void) | null = null
const wsId = 'ws-client-' + Date.now()

function getTimestamp(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

function addEvent(type: string, detail: string) {
  events.value.unshift({ id: nextEvtId++, timestamp: getTimestamp(), type, detail })
  if (events.value.length > 200) events.value = events.value.slice(0, 200)
}

async function connectWs() {
  connecting.value = true
  addEvent('info', `正在连接 ${url.value}...`)
  cleanupConnected = onWsConnected((d) => {
    if (d.id !== wsId) return
    connected.value = true; connecting.value = false
    addEvent('open', `已连接到 ${url.value}`)
  })
  cleanupDisconnected = onWsDisconnected((d) => {
    if (d.id !== wsId) return
    connected.value = false
    addEvent('close', `连接已关闭 (${d.code})`)
  })
  cleanupMessage = onWsMessage((d) => {
    if (d.id !== wsId) return
    const content = typeof d.data === 'string' ? d.data : JSON.stringify(d.data)
    messages.value.unshift({ id: nextMsgId++, timestamp: getTimestamp(), direction: 'rx', type: 'text', content })
    rxCount.value += content.length; rxMsgCount.value++
    if (messages.value.length > 500) messages.value = messages.value.slice(0, 400)
  })
  cleanupError = onWsError((d) => {
    if (d.id !== wsId) return
    addEvent('error', d.error)
  })
  const result = await wsConnect(wsId, url.value)
  connecting.value = false
  if (!result.success) { addEvent('error', result.error || '连接失败') }
}

function disconnectWs() {
  wsDisconnect(wsId)
  connected.value = false; connecting.value = false
  if (cleanupConnected) { cleanupConnected(); cleanupConnected = null }
  if (cleanupDisconnected) { cleanupDisconnected(); cleanupDisconnected = null }
  if (cleanupMessage) { cleanupMessage(); cleanupMessage = null }
  if (cleanupError) { cleanupError(); cleanupError = null }
  addEvent('close', '连接已关闭')
}

async function sendMessage() {
  if (!sendDataStr.value.trim() || !connected.value) return
  messages.value.unshift({ id: nextMsgId++, timestamp: getTimestamp(), direction: 'tx', type: messageType.value, content: sendDataStr.value })
  txCount.value += sendDataStr.value.length; txMsgCount.value++
  await wsSend(wsId, sendDataStr.value)
  sendDataStr.value = ''
}

function clearMessages() { messages.value = [] }
function clearEvents() { events.value = [] }

onUnmounted(() => { wsDisconnect(wsId); if (cleanupConnected) cleanupConnected(); if (cleanupDisconnected) cleanupDisconnected(); if (cleanupMessage) cleanupMessage(); if (cleanupError) cleanupError() })
</script>

<template>
  <div class="ws-debug">
    <div class="toolbar">
      <div class="toolbar-group" style="flex:1">
        <label class="toolbar-label">URL:</label>
        <input v-model="url" class="input-sm" style="flex:1" placeholder="ws://host:port" />
      </div>
      <div class="toolbar-group">
        <button :class="['btn', connected ? 'btn-danger' : 'btn-success']" @click="connected ? disconnectWs() : connectWs()" :disabled="connecting">
          <span class="material-symbols-outlined" style="font-size:14px">{{ connected ? 'link_off' : 'link' }}</span>
          {{ connecting ? '连接中...' : connected ? '断开' : '连接' }}
        </button>
      </div>
    </div>

    <div class="main-area">
      <div class="messages-panel">
        <div class="panel-header">
          <span class="material-symbols-outlined" style="font-size:16px">chat</span>
          消息列表
          <div class="toolbar-spacer" />
          <button class="btn" @click="clearMessages" style="font-size:11px">清空</button>
        </div>
        <div class="message-list">
          <div v-for="msg in messages" :key="msg.id" :class="['msg-item', msg.direction]">
            <div class="msg-header">
              <span class="msg-time mono">{{ msg.timestamp }}</span>
              <span :class="['msg-dir', msg.direction]">{{ msg.direction === 'tx' ? 'TX' : 'RX' }}</span>
              <span class="msg-type-tag">{{ msg.type }}</span>
            </div>
            <div class="msg-content mono">{{ msg.content }}</div>
          </div>
          <div v-if="messages.length === 0" class="empty-hint">无消息</div>
        </div>
        <div class="input-area">
          <select v-model="messageType" class="select-sm">
            <option value="text">Text</option>
            <option value="binary">Binary</option>
          </select>
          <input
            v-model="sendDataStr"
            class="input-field"
            :disabled="!connected"
            placeholder="输入消息..."
            @keydown.enter="sendMessage"
          />
          <button class="btn btn-primary" @click="sendMessage" :disabled="!connected">
            <span class="material-symbols-outlined" style="font-size:14px">send</span>
            发送
          </button>
        </div>
      </div>

      <div class="events-panel">
        <div class="panel-header">
          <span class="material-symbols-outlined" style="font-size:16px">event_log</span>
          连接事件
          <div class="toolbar-spacer" />
          <button class="btn" @click="clearEvents" style="font-size:11px">清空</button>
        </div>
        <div class="event-list">
          <div v-for="evt in events" :key="evt.id" class="event-item">
            <span class="event-time mono">{{ evt.timestamp }}</span>
            <span :class="['event-type', evt.type]">{{ evt.type }}</span>
            <span class="event-detail">{{ evt.detail }}</span>
          </div>
          <div v-if="events.length === 0" class="empty-hint">无事件</div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ connected ? `已连接 ${url}` : '未连接' }}</span>
      <span>消息: TX={{ txMsgCount }} RX={{ rxMsgCount }}</span>
      <span>字节: TX={{ txCount }} RX={{ rxCount }}</span>
    </div>
  </div>
</template>

<style scoped>
.ws-debug {
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
.messages-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  border-right: 1px solid var(--outline-variant);
}
.events-panel {
  width: 260px;
  display: flex;
  flex-direction: column;
  overflow: hidden;
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
  flex-shrink: 0;
}
.message-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px 12px;
  background: #1e1e1e;
  min-height: 0;
}
.msg-item {
  margin-bottom: 6px;
  padding: 4px 8px;
  border-radius: 4px;
}
.msg-item.tx { background: rgba(33, 150, 243, 0.08); }
.msg-item.rx { background: rgba(76, 175, 80, 0.08); }
.msg-header {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 2px;
}
.msg-time { font-size: 10px; color: #666; }
.msg-dir { font-weight: 700; font-size: 10px; }
.msg-dir.tx { color: #2196f3; }
.msg-dir.rx { color: #4caf50; }
.msg-type-tag {
  font-size: 9px;
  padding: 0 4px;
  border-radius: 2px;
  background: rgba(255, 255, 255, 0.1);
  color: #aaa;
}
.msg-content {
  font-size: 11px;
  color: #d4d4d4;
  word-break: break-all;
}
.empty-hint { text-align: center; color: var(--on-surface-variant); padding: 30px; font-family: inherit; }
.input-area {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
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
.event-list {
  flex: 1;
  overflow-y: auto;
  padding: 4px 8px;
  min-height: 0;
}
.event-item {
  display: flex;
  flex-direction: column;
  gap: 2px;
  padding: 4px 6px;
  border-bottom: 1px solid var(--outline-variant);
}
.event-time { font-size: 10px; color: var(--on-surface-variant); }
.event-type {
  font-size: 10px;
  font-weight: 600;
}
.event-type.open { color: #4caf50; }
.event-type.close { color: #f44336; }
.event-type.info { color: #2196f3; }
.event-type.error { color: #f44336; }
.event-detail { font-size: 11px; color: var(--on-surface-variant); }
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
