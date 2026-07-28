<template>
  <div class="uart-logger">
    <div class="toolbar">
      <button class="btn" :class="{ active: isRecording }" @click="toggleRecording">
        <span class="material-symbols-outlined">{{ isRecording ? 'stop_circle' : 'radio_button_checked' }}</span>
        {{ isRecording ? '停止记录' : '开始记录' }}
      </button>
      <button class="btn" @click="exportLog">
        <span class="material-symbols-outlined">download</span>
        导出日志
      </button>
      <button class="btn" @click="clearLog">
        <span class="material-symbols-outlined">delete</span>
        清空
      </button>
      <div class="spacer"></div>
      <label class="checkbox-label">
        <input type="checkbox" v-model="showTimestamp" /> 显示时间戳
      </label>
    </div>
    <div class="log-container" ref="logRef">
      <table class="log-table">
        <thead>
          <tr>
            <th width="60">序号</th>
            <th width="90" v-if="showTimestamp">时间</th>
            <th width="40">方向</th>
            <th>数据</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(entry, idx) in logEntries" :key="idx" :class="entry.direction">
            <td>{{ entry.index }}</td>
            <td v-if="showTimestamp" class="mono">{{ entry.timestamp }}</td>
            <td>
              <span :class="['dir-badge', entry.direction]">{{ entry.direction === 'tx' ? 'TX' : 'RX' }}</span>
            </td>
            <td class="mono">{{ entry.data }}</td>
          </tr>
        </tbody>
      </table>
    </div>
    <div class="status-bar">
      <span>记录条数: {{ logEntries.length }}</span>
      <span>{{ isRecording ? '录制中...' : '已停止' }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick, onUnmounted } from 'vue'
import { onSerialData } from '@/serial/serialClient'

interface LogEntry { index: number; timestamp: string; direction: 'tx' | 'rx'; data: string }

const isRecording = ref(false)
const showTimestamp = ref(true)
const logEntries = ref<LogEntry[]>([])
const logRef = ref<HTMLElement | null>(null)
let counter = 0
let cleanupData: (() => void) | null = null

function toggleRecording() {
  isRecording.value = !isRecording.value
  if (isRecording.value) {
    cleanupData = onSerialData('*', (evt) => {
      if (!isRecording.value) return
      const text = new TextDecoder().decode(new Uint8Array(evt.data))
      addEntry('rx', text)
    })
  } else if (!isRecording.value && cleanupData) {
    cleanupData(); cleanupData = null
  }
}

function clearLog() { logEntries.value = []; counter = 0 }

function exportLog() {
  const lines = logEntries.value.map(e => `${e.index}\t${e.timestamp}\t${e.direction}\t${e.data}`)
  const blob = new Blob([lines.join('\n')], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `uart_log_${new Date().toISOString().slice(0, 10)}.txt`
  a.click()
  URL.revokeObjectURL(url)
}

function addEntry(direction: 'tx' | 'rx', data: string) {
  if (!isRecording.value) return
  counter++
  const now = new Date()
  logEntries.value.push({
    index: counter,
    timestamp: now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(now.getMilliseconds()).padStart(3, '0'),
    direction, data,
  })
  if (logEntries.value.length > 2000) logEntries.value = logEntries.value.slice(-2000)
  nextTick(() => { if (logRef.value) logRef.value.scrollTop = logRef.value.scrollHeight })
}

defineExpose({ addEntry })

onUnmounted(() => { if (cleanupData) cleanupData() })
</script>

<style scoped>
.uart-logger { display: flex; flex-direction: column; height: 100%; background: var(--surface); }
.toolbar { display: flex; align-items: center; gap: 6px; padding: 8px 12px; border-bottom: 1px solid var(--outline-variant); }
.spacer { flex: 1; }
.btn { display: inline-flex; align-items: center; gap: 4px; padding: 4px 10px; border: 1px solid var(--outline-variant); border-radius: 4px; background: var(--surface-variant); color: var(--on-surface); font-size: 12px; cursor: pointer; }
.btn.active { background: #c62828; color: white; border-color: #c62828; }
.btn .material-symbols-outlined { font-size: 14px; }
.checkbox-label { display: flex; align-items: center; gap: 4px; font-size: 12px; color: var(--on-surface-variant); cursor: pointer; }
.log-container { flex: 1; overflow-y: auto; }
.log-table { width: 100%; border-collapse: collapse; font-size: 12px; }
.log-table th { background: var(--surface-container); padding: 6px 10px; text-align: left; font-weight: 600; position: sticky; top: 0; border-bottom: 1px solid var(--outline-variant); }
.log-table td { padding: 4px 10px; border-bottom: 1px solid var(--outline-variant); }
.mono { font-family: 'JetBrains Mono', monospace; }
.dir-badge { display: inline-block; padding: 1px 6px; border-radius: 3px; font-size: 10px; font-weight: 700; }
.dir-badge.tx { background: #1565c0; color: white; }
.dir-badge.rx { background: #2e7d32; color: white; }
tr.tx td { background: rgba(33, 150, 243, 0.05); }
tr.rx td { background: rgba(76, 175, 80, 0.05); }
.status-bar { display: flex; justify-content: space-between; padding: 4px 12px; font-size: 11px; color: var(--on-surface-variant); background: var(--surface-container); border-top: 1px solid var(--outline-variant); }
</style>
