<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, computed } from 'vue'

interface MonitorItem {
  id: number
  address: number
  type: 'coil' | 'discrete' | 'holding' | 'input'
  value: number
  prevValue: number | null
  lastUpdate: string
  highlight: boolean
}

const typeOptions = [
  { value: 'coil', label: '线圈 (0x)', mask: 0x0001 },
  { value: 'discrete', label: '离散输入 (1x)', mask: 0x0001 },
  { value: 'holding', label: '保持寄存器 (4x)', mask: 0xFFFF },
  { value: 'input', label: '输入寄存器 (3x)', mask: 0xFFFF },
]

const monitors = ref<MonitorItem[]>([])
const refreshInterval = ref(1000)
const valueFormat = ref<'hex' | 'dec' | 'bin'>('dec')
const running = ref(false)
let timer: ReturnType<typeof setInterval> | null = null

const newMonitor = reactive({
  address: 0,
  type: 'holding' as MonitorItem['type'],
})

let nextId = 1

const typeLabels: Record<string, string> = {
  coil: '线圈',
  discrete: '离散输入',
  holding: '保持寄存器',
  input: '输入寄存器',
}

const typeTags: Record<string, string> = {
  coil: 'COIL',
  discrete: 'DISC',
  holding: 'HREG',
  input: 'IREG',
}

function isBitType(type: string) {
  return type === 'coil' || type === 'discrete'
}

function formatValue(item: MonitorItem): string {
  if (isBitType(item.type)) {
    return item.value ? 'ON' : 'OFF'
  }
  if (valueFormat.value === 'hex') {
    return '0x' + (item.value & 0xFFFF).toString(16).toUpperCase().padStart(4, '0')
  }
  if (valueFormat.value === 'bin') {
    return '0b' + (item.value & 0xFFFF).toString(2).padStart(16, '0')
  }
  return String(item.value)
}

function addMonitor() {
  if (monitors.value.length >= 50) return
  monitors.value.push({
    id: nextId++,
    address: newMonitor.address,
    type: newMonitor.type,
    value: 0,
    prevValue: null,
    lastUpdate: '--',
    highlight: false,
  })
}

function removeMonitor(id: number) {
  monitors.value = monitors.value.filter(m => m.id !== id)
}

function refreshData() {
  monitors.value.forEach(m => {
    m.prevValue = m.value
    if (isBitType(m.type)) {
      m.value = Math.random() > 0.5 ? 1 : 0
    } else {
      m.value = Math.floor(Math.random() * 65536)
    }
    m.lastUpdate = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    if (m.prevValue !== null && m.prevValue !== m.value) {
      m.highlight = true
      setTimeout(() => { m.highlight = false }, 800)
    }
  })
}

function startMonitoring() {
  running.value = true
  refreshData()
  timer = setInterval(refreshData, refreshInterval.value)
}

function stopMonitoring() {
  running.value = false
  if (timer) {
    clearInterval(timer)
    timer = null
  }
}

function onIntervalChange() {
  if (running.value) {
    stopMonitoring()
    startMonitoring()
  }
}

function exportCSV() {
  if (monitors.value.length === 0) return
  const header = '地址,类型,值,上次更新'
  const rows = monitors.value.map(m =>
    `${m.address},${typeLabels[m.type]},${formatValue(m)},${m.lastUpdate}`
  )
  const csv = '\uFEFF' + header + '\n' + rows.join('\n')
  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `modbus_monitor_${Date.now()}.csv`
  a.click()
  URL.revokeObjectURL(url)
}

function addDemoItems() {
  monitors.value = [
    { id: nextId++, address: 0, type: 'coil', value: 1, prevValue: null, lastUpdate: '--', highlight: false },
    { id: nextId++, address: 1, type: 'coil', value: 0, prevValue: null, lastUpdate: '--', highlight: false },
    { id: nextId++, address: 100, type: 'holding', value: 25600, prevValue: null, lastUpdate: '--', highlight: false },
    { id: nextId++, address: 101, type: 'holding', value: 12345, prevValue: null, lastUpdate: '--', highlight: false },
    { id: nextId++, address: 102, type: 'input', value: 54321, prevValue: null, lastUpdate: '--', highlight: false },
  ]
}

onMounted(() => {
  addDemoItems()
})

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<template>
  <div class="modbus-monitor">
    <div class="toolbar">
      <label class="toolbar-label">地址:</label>
      <input v-model.number="newMonitor.address" type="number" class="input-sm" style="width:64px" min="0" max="65535" />
      <select v-model="newMonitor.type" class="select-sm">
        <option v-for="opt in typeOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
      </select>
      <button class="btn" @click="addMonitor">
        <span class="material-symbols-outlined" style="font-size:16px">add</span>
        添加监控
      </button>
      <div class="toolbar-separator" />
      <button class="btn" @click="addDemoItems">
        <span class="material-symbols-outlined" style="font-size:16px">science</span>
        示例数据
      </button>
      <button class="btn" @click="exportCSV">
        <span class="material-symbols-outlined" style="font-size:16px">download</span>
        导出CSV
      </button>
      <div class="toolbar-spacer" />
      <label class="toolbar-label">值格式:</label>
      <select v-model="valueFormat" class="select-sm">
        <option value="dec">十进制</option>
        <option value="hex">十六进制</option>
        <option value="bin">二进制</option>
      </select>
      <label class="toolbar-label">刷新:</label>
      <select v-model.number="refreshInterval" class="select-sm" @change="onIntervalChange">
        <option :value="100">100ms</option>
        <option :value="200">200ms</option>
        <option :value="500">500ms</option>
        <option :value="1000">1s</option>
        <option :value="2000">2s</option>
        <option :value="5000">5s</option>
      </select>
      <button v-if="!running" class="btn btn-primary" @click="startMonitoring">
        <span class="material-symbols-outlined" style="font-size:16px">play_arrow</span>
        开始监控
      </button>
      <button v-else class="btn" @click="stopMonitoring">
        <span class="material-symbols-outlined" style="font-size:16px">stop</span>
        停止
      </button>
    </div>

    <div class="main-content">
      <div class="panel monitor-panel">
        <div class="panel-header">
          <span class="material-symbols-outlined" style="font-size:16px">monitor_heart</span>
          监控列表
          <div class="toolbar-spacer" />
          <span v-if="running" class="running-indicator">
            <span class="material-symbols-outlined blink" style="font-size:14px">fiber_manual_record</span>
            监控中
          </span>
          <span class="item-count">{{ monitors.length }} 项</span>
        </div>
        <div class="panel-body table-wrap">
          <table class="data-table">
            <thead>
              <tr>
                <th style="width:32px"></th>
                <th>地址</th>
                <th>类型</th>
                <th>当前值</th>
                <th>上次更新</th>
                <th style="width:36px"></th>
              </tr>
            </thead>
            <tbody>
              <tr
                v-for="m in monitors"
                :key="m.id"
                :class="{ highlight: m.highlight }"
              >
                <td>
                  <span class="type-dot" :class="m.type" />
                </td>
                <td class="mono">{{ m.address }}</td>
                <td>
                  <span class="type-tag" :class="m.type">{{ typeTags[m.type] }}</span>
                </td>
                <td class="mono value-cell">
                  <span :class="{ 'val-on': isBitType(m.type) && m.value, 'val-off': isBitType(m.type) && !m.value }">
                    {{ formatValue(m) }}
                  </span>
                </td>
                <td class="mono">{{ m.lastUpdate }}</td>
                <td>
                  <button class="btn-icon" @click="removeMonitor(m.id)">
                    <span class="material-symbols-outlined" style="font-size:14px">close</span>
                  </button>
                </td>
              </tr>
              <tr v-if="monitors.length === 0">
                <td colspan="6" class="empty-cell">无监控项</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>监控项: {{ monitors.length }}</span>
      <span>值格式: {{ valueFormat === 'hex' ? '十六进制' : valueFormat === 'bin' ? '二进制' : '十进制' }}</span>
      <span>刷新间隔: {{ refreshInterval }}ms</span>
    </div>
  </div>
</template>

<style scoped>
.modbus-monitor {
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

.toolbar-separator {
  width: 1px;
  height: 20px;
  background: var(--outline-variant);
  margin: 0 2px;
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

.btn-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: var(--on-surface-variant);
  cursor: pointer;
}

.btn-icon:hover {
  background: rgba(244, 67, 54, 0.15);
  color: #f44336;
}

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
  flex: 1;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

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

.item-count {
  font-size: 11px;
  color: var(--on-surface-variant);
  font-weight: 400;
}

.running-indicator {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: #4caf50;
  font-size: 11px;
}

.blink { animation: blink-anim 1s infinite; }

@keyframes blink-anim {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.2; }
}

.panel-body {
  flex: 1;
  overflow: hidden;
}

.table-wrap {
  overflow-y: auto;
  height: 100%;
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
  padding: 5px 8px;
  border-bottom: 1px solid var(--outline-variant);
}

.mono {
  font-family: 'JetBrains Mono', monospace;
}

.type-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.type-dot.coil { background: #42a5f5; }
.type-dot.discrete { background: #9c27b0; }
.type-dot.holding { background: #66bb6a; }
.type-dot.input { background: #ff9800; }

.type-tag {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.3px;
}

.type-tag.coil { background: rgba(66, 165, 245, 0.15); color: #42a5f5; }
.type-tag.discrete { background: rgba(156, 39, 176, 0.15); color: #9c27b0; }
.type-tag.holding { background: rgba(102, 187, 106, 0.15); color: #66bb6a; }
.type-tag.input { background: rgba(255, 152, 0, 0.15); color: #ff9800; }

.value-cell {
  font-weight: 700;
  font-size: 13px;
}

.val-on { color: #4caf50; }
.val-off { color: var(--on-surface-variant); }

tr.highlight {
  animation: value-flash 0.8s ease-out;
}

@keyframes value-flash {
  0% { background: rgba(76, 175, 80, 0.35); }
  100% { background: transparent; }
}

.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 40px !important;
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
