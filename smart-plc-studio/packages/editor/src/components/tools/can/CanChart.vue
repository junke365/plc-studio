<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted, nextTick, watch } from 'vue'

interface CanIdConfig {
  id: string
  color: string
  enabled: boolean
  label: string
}

const selectedIds = reactive<CanIdConfig[]>([
  { id: '100', color: '#42a5f5', enabled: true, label: 'ID 0x100' },
  { id: '200', color: '#66bb6a', enabled: true, label: 'ID 0x200' },
  { id: '300', color: '#ff9800', enabled: true, label: 'ID 0x300' },
  { id: '400', color: '#ef5350', enabled: true, label: 'ID 0x400' },
])

const running = ref(false)
const paused = ref(false)
const canvasRef = ref<HTMLCanvasElement | null>(null)
let animFrame: number | null = null
let intervalTimer: ReturnType<typeof setInterval> | null = null

const timeWindow = 5000
interface DataPoint { time: number; value: number }
const chartData = reactive<Record<string, DataPoint[]>>([])
const maxPoints = 100

const stats = computed(() => {
  const result: Record<string, { max: number; min: number; avg: number; current: number }> = {}
  selectedIds.forEach(idConf => {
    const key = idConf.id
    const data = chartData.find(d => d[key])?.[key] || []
    if (data.length === 0) {
      result[key] = { max: 0, min: 0, avg: 0, current: 0 }
      return
    }
    const vals = data.map(d => d.value)
    const current = vals[vals.length - 1] || 0
    result[key] = {
      max: Math.max(...vals),
      min: Math.min(...vals),
      avg: Math.round(vals.reduce((a, b) => a + b, 0) / vals.length),
      current,
    }
  })
  return result
})

function initChartData() {
  chartData.length = 0
  selectedIds.forEach(idConf => {
    chartData.push({ [idConf.id]: [] })
  })
}

function addDataPoint() {
  const now = Date.now()
  selectedIds.forEach(idConf => {
    if (!idConf.enabled) return
    const entry = chartData.find(d => d[idConf.id])
    if (!entry) return
    const prev = entry[idConf.id].length > 0
      ? entry[idConf.id][entry[idConf.id].length - 1].value
      : 50
    const newVal = Math.max(0, Math.min(100, prev + (Math.random() - 0.5) * 10))
    entry[idConf.id].push({ time: now, value: Math.round(newVal * 10) / 10 })
    while (entry[idConf.id].length > maxPoints) {
      entry[idConf.id].shift()
    }
  })
  if (!paused.value) drawChart()
}

function drawChart() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return
  const rect = canvas.parentElement!.getBoundingClientRect()
  canvas.width = rect.width
  canvas.height = rect.height
  const w = canvas.width
  const h = canvas.height
  const pad = { top: 20, right: 60, bottom: 30, left: 10 }
  const plotW = w - pad.left - pad.right
  const plotH = h - pad.top - pad.bottom

  ctx.clearRect(0, 0, w, h)
  ctx.fillStyle = '#1e1e1e'
  ctx.fillRect(0, 0, w, h)

  // 网格线
  ctx.strokeStyle = 'rgba(255,255,255,0.06)'
  ctx.lineWidth = 1
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (plotH / 5) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(w - pad.right, y)
    ctx.stroke()
  }

  // Y轴标签
  ctx.fillStyle = '#666'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'right'
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (plotH / 5) * i
    const val = 100 - (100 / 5) * i
    ctx.fillText(String(Math.round(val)), w - pad.right + 30, y + 4)
  }

  const now = Date.now()

  // 数据曲线
  selectedIds.forEach(idConf => {
    if (!idConf.enabled) return
    const entry = chartData.find(d => d[idConf.id])
    if (!entry) return
    const data = entry[idConf.id].filter(d => now - d.time <= timeWindow)
    if (data.length < 2) return

    ctx.strokeStyle = idConf.color
    ctx.lineWidth = 1.5
    ctx.beginPath()
    data.forEach((point, i) => {
      const x = pad.left + ((now - (now - timeWindow) - (now - point.time)) / timeWindow) * plotW
      const y = pad.top + plotH - (point.value / 100) * plotH
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    })
    ctx.stroke()
  })

  // 时间轴标签
  ctx.fillStyle = '#666'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  for (let i = 0; i <= 5; i++) {
    const x = pad.left + (plotW / 5) * i
    const secAgo = 5 - i
    ctx.fillText(`-${secAgo}s`, x, h - pad.bottom + 15)
  }
}

function startChart() {
  running.value = true
  paused.value = false
  initChartData()
  addDataPoint()
  intervalTimer = setInterval(addDataPoint, 100)
}

function stopChart() {
  running.value = false
  if (intervalTimer) {
    clearInterval(intervalTimer)
    intervalTimer = null
  }
  if (animFrame) {
    cancelAnimationFrame(animFrame)
    animFrame = null
  }
}

function togglePause() {
  paused.value = !paused.value
}

function clearChart() {
  initChartData()
  drawChart()
}

function exportChart() {
  const canvas = canvasRef.value
  if (!canvas) return
  const link = document.createElement('a')
  link.download = `can_chart_${Date.now()}.png`
  link.href = canvas.toDataURL('image/png')
  link.click()
}

onMounted(() => {
  drawChart()
  window.addEventListener('resize', drawChart)
})

onUnmounted(() => {
  stopChart()
  window.removeEventListener('resize', drawChart)
})
</script>

<template>
  <div class="can-chart">
    <div class="toolbar">
      <div class="toolbar-group">
        <span class="toolbar-label">CAN ID选择 (最多4个):</span>
        <div v-for="(conf, idx) in selectedIds" :key="idx" class="id-selector">
          <span class="color-dot" :style="{ background: conf.color }"></span>
          <input v-model="conf.id" class="input-sm" style="width:60px" placeholder="0x100" />
          <label class="checkbox-label" style="font-size:11px">
            <input type="checkbox" v-model="conf.enabled" />
          </label>
        </div>
      </div>
      <div class="toolbar-spacer" />
      <button v-if="!running" class="btn btn-primary" @click="startChart">
        <span class="material-symbols-outlined" style="font-size:14px">play_arrow</span>
        启动
      </button>
      <button v-else class="btn btn-danger" @click="stopChart">
        <span class="material-symbols-outlined" style="font-size:14px">stop</span>
        停止
      </button>
      <button v-if="running" class="btn" @click="togglePause">
        <span class="material-symbols-outlined" style="font-size:14px">{{ paused ? 'play_arrow' : 'pause' }}</span>
        {{ paused ? '恢复' : '暂停' }}
      </button>
      <button class="btn" @click="clearChart">
        <span class="material-symbols-outlined" style="font-size:14px">delete</span>
        清空
      </button>
      <button class="btn" @click="exportChart">
        <span class="material-symbols-outlined" style="font-size:14px">download</span>
        导出
      </button>
    </div>

    <div class="chart-container">
      <canvas ref="canvasRef"></canvas>
    </div>

    <div class="stats-panel">
      <div v-for="conf in selectedIds" :key="conf.id" class="stat-card" v-show="conf.enabled">
        <div class="stat-card-header">
          <span class="color-dot" :style="{ background: conf.color }"></span>
          <span class="mono">0x{{ conf.id.toUpperCase() }}</span>
        </div>
        <div class="stat-values">
          <div class="stat-item">
            <span class="stat-label">当前</span>
            <span class="stat-val mono">{{ stats[conf.id]?.current ?? '--' }}</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">最大</span>
            <span class="stat-val mono">{{ stats[conf.id]?.max ?? '--' }}</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">最小</span>
            <span class="stat-val mono">{{ stats[conf.id]?.min ?? '--' }}</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">平均</span>
            <span class="stat-val mono">{{ stats[conf.id]?.avg ?? '--' }}</span>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>时间窗口: 5秒</span>
      <span>{{ running ? (paused ? '已暂停' : '采集中') : '已停止' }}</span>
    </div>
  </div>
</template>

<style scoped>
.can-chart {
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
.id-selector {
  display: flex;
  align-items: center;
  gap: 4px;
}
.color-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
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
.btn:hover { opacity: 0.85; }
.btn-primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-danger { background: #c62828; color: white; border-color: #c62828; }
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
.chart-container {
  flex: 1;
  min-height: 200px;
  position: relative;
  overflow: hidden;
}
.chart-container canvas {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
}
.stats-panel {
  display: flex;
  gap: 8px;
  padding: 8px 12px;
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
  overflow-x: auto;
}
.stat-card {
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  padding: 6px 10px;
  min-width: 180px;
}
.stat-card-header {
  display: flex;
  align-items: center;
  gap: 6px;
  font-weight: 600;
  font-size: 12px;
  margin-bottom: 4px;
}
.stat-values {
  display: flex;
  gap: 10px;
}
.stat-item {
  display: flex;
  flex-direction: column;
  gap: 1px;
}
.stat-label {
  font-size: 9px;
  color: var(--on-surface-variant);
}
.stat-val {
  font-size: 13px;
  font-weight: 700;
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
