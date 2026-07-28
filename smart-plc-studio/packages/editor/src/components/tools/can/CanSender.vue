<script setup lang="ts">
import { ref, reactive, onUnmounted } from 'vue'

interface SendHistory {
  id: number
  timestamp: string
  canId: string
  data: string
  status: 'ok' | 'error' | 'pending'
}

const canId = ref('001')
const idType = ref<'std' | 'ext'>('std')
const dlc = ref(8)
const bytes = reactive<string[]>(['00', '00', '00', '00', '00', '00', '00', '00'])
const sendMode = ref<'once' | 'periodic'>('once')
const period = ref(100)
const sending = ref(false)
let periodicTimer: ReturnType<typeof setInterval> | null = null
const sendCount = ref(0)
const history = ref<SendHistory[]>([])
let nextHistoryId = 1

function formatTimestamp(): string {
  const now = new Date()
  return now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

function getHexString(): string {
  return bytes.slice(0, dlc.value).join(' ')
}

function sendOnce() {
  const hexStr = getHexString()
  history.value.unshift({
    id: nextHistoryId++,
    timestamp: formatTimestamp(),
    canId: canId.value.toUpperCase(),
    data: hexStr,
    status: 'ok',
  })
  sendCount.value++
  if (history.value.length > 200) {
    history.value = history.value.slice(0, 200)
  }
}

function togglePeriodicSend() {
  if (sending.value) {
    stopPeriodic()
  } else {
    startPeriodic()
  }
}

function startPeriodic() {
  sending.value = true
  sendOnce()
  periodicTimer = setInterval(sendOnce, period.value)
}

function stopPeriodic() {
  sending.value = false
  if (periodicTimer) {
    clearInterval(periodicTimer)
    periodicTimer = null
  }
}

function onByteInput(index: number, event: Event) {
  const input = event.target as HTMLInputElement
  let val = input.value.replace(/[^0-9a-fA-F]/g, '').toUpperCase()
  if (val.length > 2) val = val.slice(-2)
  bytes[index] = val.padStart(2, '0')
}

function handleByteKeydown(index: number, event: KeyboardEvent) {
  if (event.key === ' ' || event.key === 'Tab') {
    event.preventDefault()
    const nextIdx = index + 1
    if (nextIdx < bytes.length) {
      const nextInput = document.querySelector(`[data-byte-index="${nextIdx}"]`) as HTMLInputElement
      if (nextInput) nextInput.focus()
    }
  }
}

function clearHistory() {
  history.value = []
  sendCount.value = 0
}

function clearData() {
  for (let i = 0; i < 8; i++) {
    bytes[i] = '00'
  }
}

onUnmounted(() => {
  if (periodicTimer) clearInterval(periodicTimer)
})
</script>

<template>
  <div class="can-sender">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">CAN ID:</label>
        <input v-model="canId" class="input-sm" style="width:80px" placeholder="0x001" />
        <select v-model="idType" class="select-sm">
          <option value="std">标准帧</option>
          <option value="ext">扩展帧</option>
        </select>
        <label class="toolbar-label">DLC:</label>
        <select v-model.number="dlc" class="select-sm">
          <option v-for="n in 8" :key="n" :value="n">{{ n }}</option>
        </select>
      </div>
      <div class="toolbar-group">
        <label class="toolbar-label">发送模式:</label>
        <select v-model="sendMode" class="select-sm">
          <option value="once">单次发送</option>
          <option value="periodic">周期发送</option>
        </select>
        <template v-if="sendMode === 'periodic'">
          <label class="toolbar-label">间隔:</label>
          <input v-model.number="period" type="number" class="input-sm" style="width:60px" min="10" max="1000" />
          <span class="toolbar-label">ms</span>
        </template>
      </div>
    </div>

    <div class="data-input-section">
      <div class="section-title">数据输入</div>
      <div class="byte-row">
        <div v-for="(_, idx) in 8" :key="idx" class="byte-item">
          <span class="byte-label">B{{ idx }}</span>
          <input
            :value="bytes[idx]"
            :data-byte-index="idx"
            class="input-sm byte-input"
            maxlength="2"
            :disabled="idx >= dlc"
            @input="onByteInput(idx, $event)"
            @keydown="handleByteKeydown(idx, $event)"
          />
        </div>
      </div>
      <div class="hex-preview">
        <span class="toolbar-label">HEX:</span>
        <span class="mono">{{ getHexString() }}</span>
      </div>
    </div>

    <div class="send-actions">
      <button v-if="sendMode === 'once'" class="btn btn-primary" @click="sendOnce">
        <span class="material-symbols-outlined" style="font-size:14px">send</span>
        单次发送
      </button>
      <button v-else :class="['btn', sending ? 'btn-danger' : 'btn-success']" @click="togglePeriodicSend">
        <span class="material-symbols-outlined" style="font-size:14px">{{ sending ? 'stop' : 'play_arrow' }}</span>
        {{ sending ? '停止发送' : '开始周期发送' }}
      </button>
      <button class="btn" @click="clearData">清空数据</button>
      <div class="toolbar-spacer" />
      <span class="send-counter">发送计数: <strong>{{ sendCount }}</strong></span>
    </div>

    <div class="history-section">
      <div class="section-title">
        发送历史
        <div class="toolbar-spacer" />
        <button class="btn" @click="clearHistory" style="font-size:11px">
          <span class="material-symbols-outlined" style="font-size:12px">delete</span>
          清空
        </button>
      </div>
      <div class="history-table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th style="width:120px">时间</th>
              <th style="width:90px">CAN ID</th>
              <th>数据</th>
              <th style="width:60px">状态</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in history" :key="item.id">
              <td class="mono">{{ item.timestamp }}</td>
              <td class="mono">{{ item.canId }}</td>
              <td class="mono">{{ item.data }}</td>
              <td>
                <span :class="['status-tag', item.status]">
                  {{ item.status === 'ok' ? '成功' : item.status === 'error' ? '失败' : '等待' }}
                </span>
              </td>
            </tr>
            <tr v-if="history.length === 0">
              <td colspan="4" class="empty-cell">无发送记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div class="status-bar">
      <span>CAN ID: 0x{{ canId.toUpperCase() }} ({{ idType === 'std' ? '标准' : '扩展' }})</span>
      <span>DLC: {{ dlc }}</span>
      <span>模式: {{ sendMode === 'once' ? '单次' : period + 'ms 周期' }}</span>
      <span>已发送: {{ sendCount }}</span>
    </div>
  </div>
</template>

<style scoped>
.can-sender {
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
.toolbar-label {
  font-size: 11px;
  color: var(--on-surface-variant);
}
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
.mono { font-family: 'JetBrains Mono', monospace; }
.data-input-section {
  padding: 10px 12px;
  border-bottom: 1px solid var(--outline-variant);
}
.section-title {
  display: flex;
  align-items: center;
  font-weight: 600;
  font-size: 12px;
  margin-bottom: 8px;
}
.byte-row {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}
.byte-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}
.byte-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}
.byte-input {
  width: 42px;
  text-align: center;
  text-transform: uppercase;
}
.byte-input:disabled {
  opacity: 0.3;
}
.hex-preview {
  margin-top: 6px;
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: var(--on-surface-variant);
}
.send-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
}
.send-counter {
  font-size: 12px;
  color: var(--on-surface-variant);
}
.send-counter strong {
  color: var(--primary);
  font-size: 14px;
}
.history-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.history-section .section-title {
  padding: 6px 12px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
}
.history-table-wrap {
  flex: 1;
  overflow-y: auto;
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
.status-tag {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 10px;
  font-weight: 600;
}
.status-tag.ok { background: rgba(46, 125, 50, 0.15); color: #2e7d32; }
.status-tag.error { background: rgba(198, 40, 40, 0.15); color: #c62828; }
.status-tag.pending { background: rgba(255, 152, 0, 0.15); color: #ff9800; }
.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 30px !important;
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
