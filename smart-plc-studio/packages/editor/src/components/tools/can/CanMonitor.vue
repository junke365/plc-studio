<script setup lang="ts">
import { ref, reactive, computed, onUnmounted } from 'vue'

interface CanFrame {
  id: number
  timestamp: string
  canId: string
  idType: 'std' | 'ext'
  frameType: 'data' | 'remote' | 'error'
  dlc: number
  data: number[]
  period: number
}

interface IdStats {
  canId: string
  count: number
}

const canInterface = ref('CAN0')
const baudRate = ref(500000)
const running = ref(false)
const paused = ref(false)
const frames = ref<CanFrame[]>([])
const filterIdMin = ref('')
const filterIdMax = ref('')
const filterFrameType = ref<'all' | 'std' | 'ext'>('all')
const autoScroll = ref(true)
let timer: ReturnType<typeof setInterval> | null = null
let frameCounter = 0

const baudOptions = [
  { value: 125000, label: '125k' },
  { value: 250000, label: '250k' },
  { value: 500000, label: '500k' },
  { value: 1000000, label: '1M' },
]

const idColors: Record<string, string> = {}
const colorPalette = ['#42a5f5', '#66bb6a', '#ff9800', '#ef5350', '#ab47bc', '#26c6da', '#ec407a', '#78909c']

function getIdColor(canId: string): string {
  if (!idColors[canId]) {
    const idx = Object.keys(idColors).length % colorPalette.length
    idColors[canId] = colorPalette[idx]
  }
  return idColors[canId]
}

const filteredFrames = computed(() => {
  return frames.value.filter(f => {
    if (filterFrameType.value === 'std' && f.idType !== 'std') return false
    if (filterFrameType.value === 'ext' && f.idType !== 'ext') return false
    if (filterIdMin.value) {
      const id = parseInt(f.canId, 16)
      if (id < parseInt(filterIdMin.value, 16)) return false
    }
    if (filterIdMax.value) {
      const id = parseInt(f.canId, 16)
      if (id > parseInt(filterIdMax.value, 16)) return false
    }
    return true
  })
})

const totalFrames = computed(() => frames.value.length)

const frameRate = computed(() => {
  if (frames.value.length < 2) return 0
  const now = Date.now()
  const recent = frames.value.filter(f => {
    const t = new Date(f.timestamp).getTime()
    return now - t < 1000
  })
  return recent.length
})

const idStats = computed<IdStats[]>(() => {
  const map: Record<string, number> = {}
  frames.value.forEach(f => {
    map[f.canId] = (map[f.canId] || 0) + 1
  })
  return Object.entries(map)
    .map(([canId, count]) => ({ canId, count }))
    .sort((a, b) => b.count - a.count)
})

function getTimestamp(): string {
  const now = new Date()
  return now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

function randomHex(len: number): number[] {
  const arr: number[] = []
  for (let i = 0; i < len; i++) {
    arr.push(Math.floor(Math.random() * 256))
  }
  return arr
}

function generateFrame(): CanFrame {
  frameCounter++
  const isExt = Math.random() > 0.7
  const idVal = isExt
    ? Math.floor(Math.random() * 0x1FFFFFFF)
    : Math.floor(Math.random() * 0x7FF)
  const dlc = Math.floor(Math.random() * 8) + 1
  return {
    id: frameCounter,
    timestamp: getTimestamp(),
    canId: isExt
      ? idVal.toString(16).toUpperCase().padStart(8, '0')
      : idVal.toString(16).toUpperCase().padStart(3, '0'),
    idType: isExt ? 'ext' : 'std',
    frameType: Math.random() > 0.95 ? 'remote' : Math.random() > 0.98 ? 'error' : 'data',
    dlc,
    data: randomHex(dlc),
    period: Math.floor(Math.random() * 100) + 1,
  }
}

function startMonitor() {
  running.value = true
  paused.value = false
  addFrame()
  timer = setInterval(addFrame, 50)
}

function addFrame() {
  if (paused.value) return
  const frame = generateFrame()
  frames.value.push(frame)
  if (frames.value.length > 2000) {
    frames.value = frames.value.slice(-1500)
  }
}

function stopMonitor() {
  running.value = false
  if (timer) {
    clearInterval(timer)
    timer = null
  }
}

function togglePause() {
  paused.value = !paused.value
}

function clearFrames() {
  frames.value = []
  frameCounter = 0
  Object.keys(idColors).forEach(k => delete idColors[k])
}

function formatData(data: number[]): string {
  return data.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
}

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<template>
  <div class="can-monitor">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">接口:</label>
        <select v-model="canInterface" class="select-sm">
          <option value="CAN0">CAN0</option>
          <option value="CAN1">CAN1</option>
        </select>
        <label class="toolbar-label">波特率:</label>
        <select v-model.number="baudRate" class="select-sm">
          <option v-for="opt in baudOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
        </select>
      </div>
      <div class="toolbar-group">
        <button v-if="!running" class="btn btn-success" @click="startMonitor">
          <span class="material-symbols-outlined" style="font-size:14px">play_arrow</span>
          启动
        </button>
        <button v-else class="btn btn-danger" @click="stopMonitor">
          <span class="material-symbols-outlined" style="font-size:14px">stop</span>
          停止
        </button>
        <button v-if="running" class="btn" @click="togglePause">
          <span class="material-symbols-outlined" style="font-size:14px">{{ paused ? 'play_arrow' : 'pause' }}</span>
          {{ paused ? '恢复' : '暂停' }}
        </button>
        <button class="btn" @click="clearFrames">
          <span class="material-symbols-outlined" style="font-size:14px">delete</span>
          清空
        </button>
      </div>
      <div class="toolbar-group">
        <label class="toolbar-label">ID最小:</label>
        <input v-model="filterIdMin" class="input-sm" style="width:60px" placeholder="0x000" />
        <label class="toolbar-label">ID最大:</label>
        <input v-model="filterIdMax" class="input-sm" style="width:60px" placeholder="0x7FF" />
        <select v-model="filterFrameType" class="select-sm">
          <option value="all">全部帧</option>
          <option value="std">标准帧</option>
          <option value="ext">扩展帧</option>
        </select>
      </div>
      <div class="toolbar-spacer" />
      <label class="checkbox-label">
        <input type="checkbox" v-model="autoScroll" /> 自动滚动
      </label>
    </div>

    <div class="frame-list" ref="frameListRef">
      <table class="data-table">
        <thead>
          <tr>
            <th style="width:100px">时间戳</th>
            <th style="width:90px">CAN ID</th>
            <th style="width:60px">帧类型</th>
            <th style="width:50px">DLC</th>
            <th>数据 (HEX)</th>
            <th style="width:70px">周期 ms</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="frame in filteredFrames" :key="frame.id">
            <td class="mono">{{ frame.timestamp }}</td>
            <td class="mono">
              <span class="id-badge" :style="{ background: getIdColor(frame.canId) }">
                {{ frame.canId }}
              </span>
            </td>
            <td>
              <span :class="['frame-type-tag', frame.frameType]">
                {{ frame.frameType === 'data' ? '数据' : frame.frameType === 'remote' ? '远程' : '错误' }}
              </span>
            </td>
            <td class="mono">{{ frame.dlc }}</td>
            <td class="mono data-cell">{{ formatData(frame.data) }}</td>
            <td class="mono">{{ frame.period }}</td>
          </tr>
          <tr v-if="filteredFrames.length === 0">
            <td colspan="6" class="empty-cell">无报文数据</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="stats-panel">
      <div class="stat-item">
        <span class="stat-label">总帧数</span>
        <span class="stat-value">{{ totalFrames }}</span>
      </div>
      <div class="stat-item">
        <span class="stat-label">帧率 fps</span>
        <span class="stat-value">{{ frameRate }}</span>
      </div>
      <div class="stat-separator" />
      <div class="id-stats">
        <div v-for="s in idStats.slice(0, 8)" :key="s.canId" class="id-stat-item">
          <span class="id-dot" :style="{ background: getIdColor(s.canId) }"></span>
          <span class="mono">{{ s.canId }}</span>
          <span class="stat-count">{{ s.count }}</span>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ canInterface }} @ {{ baudOptions.find(b => b.value === baudRate)?.label }}</span>
      <span>{{ paused ? '已暂停' : running ? '监控中' : '已停止' }}</span>
      <span>报文: {{ frames.length }}</span>
    </div>
  </div>
</template>

<style scoped>
.can-monitor {
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
.checkbox-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.frame-list {
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
.mono { font-family: 'JetBrains Mono', monospace; }
.id-badge {
  display: inline-block;
  padding: 1px 6px;
  border-radius: 3px;
  color: #fff;
  font-size: 10px;
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
}
.frame-type-tag {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 10px;
  font-weight: 600;
}
.frame-type-tag.data { background: rgba(66, 165, 245, 0.15); color: #42a5f5; }
.frame-type-tag.remote { background: rgba(255, 152, 0, 0.15); color: #ff9800; }
.frame-type-tag.error { background: rgba(244, 67, 54, 0.15); color: #f44336; }
.data-cell { font-size: 11px; letter-spacing: 0.3px; }
.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 40px !important;
}
.stats-panel {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 6px 12px;
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
  overflow-x: auto;
}
.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}
.stat-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}
.stat-value {
  font-size: 14px;
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
}
.stat-separator {
  width: 1px;
  height: 24px;
  background: var(--outline-variant);
}
.id-stats {
  display: flex;
  gap: 12px;
  overflow-x: auto;
}
.id-stat-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  white-space: nowrap;
}
.id-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;
}
.stat-count {
  font-family: 'JetBrains Mono', monospace;
  color: var(--on-surface-variant);
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
