<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'

const canvasRef = ref<HTMLCanvasElement | null>(null)
const running = ref(false)
const paused = ref(false)
const dataSource = ref('all')
const txData = ref<number[]>([])
const rxData = ref<number[]>([])
let intervalTimer: ReturnType<typeof setInterval> | null = null
const timeWindow = 60000

const currentTx = ref(0)
const currentRx = ref(0)
const peakTx = ref(0)
const peakRx = ref(0)
const avgTx = ref(0)
const avgRx = ref(0)
const totalTx = ref(0)
const totalRx = ref(0)

function getTimestamp(): number {
  return Date.now()
}

function addDataPoint() {
  const now = getTimestamp()
  const newTx = Math.floor(Math.random() * 5000 + 500)
  const newRx = Math.floor(Math.random() * 8000 + 1000)
  txData.value.push(newTx)
  rxData.value.push(newRx)

  // 只保留60秒内的数据
  const maxPoints = 300
  if (txData.value.length > maxPoints) txData.value = txData.value.slice(-maxPoints)
  if (rxData.value.length > maxPoints) rxData.value = rxData.value.slice(-maxPoints)

  currentTx.value = newTx
  currentRx.value = newRx
  peakTx.value = Math.max(peakTx.value, newTx)
  peakRx.value = Math.max(peakRx.value, newRx)
  totalTx.value += newTx
  totalRx.value += newRx

  const txArr = txData.value
  const rxArr = rxData.value
  avgTx.value = Math.round(txArr.reduce((a, b) => a + b, 0) / txArr.length)
  avgRx.value = Math.round(rxArr.reduce((a, b) => a + b, 0) / rxArr.length)

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

  // 网格
  ctx.strokeStyle = 'rgba(255,255,255,0.06)'
  ctx.lineWidth = 1
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (plotH / 5) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(w - pad.right, y)
    ctx.stroke()
  }

  // 计算Y轴范围
  const allVals = [...txData.value, ...rxData.value]
  const maxVal = Math.max(...allVals, 1000)

  // Y轴标签
  ctx.fillStyle = '#666'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'right'
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (plotH / 5) * i
    const val = Math.round(maxVal - (maxVal / 5) * i)
    ctx.fillText(val > 1000 ? (val / 1000).toFixed(1) + 'k' : String(val), w - pad.right + 30, y + 4)
  }

  // 绘制数据
  function drawLine(data: number[], color: string) {
    if (data.length < 2) return
    ctx.strokeStyle = color
    ctx.lineWidth = 1.5
    ctx.beginPath()
    data.forEach((val, i) => {
      const x = pad.left + (i / (data.length - 1)) * plotW
      const y = pad.top + plotH - (val / maxVal) * plotH
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    })
    ctx.stroke()

    // 渐变填充
    ctx.globalAlpha = 0.1
    ctx.lineTo(pad.left + plotW, pad.top + plotH)
    ctx.lineTo(pad.left, pad.top + plotH)
    ctx.closePath()
    ctx.fillStyle = color
    ctx.fill()
    ctx.globalAlpha = 1
  }

  if (dataSource.value === 'all' || dataSource.value === 'rx') {
    drawLine(rxData.value, '#4caf50')
  }
  if (dataSource.value === 'all' || dataSource.value === 'tx') {
    drawLine(txData.value, '#2196f3')
  }

  // 图例
  ctx.font = '11px JetBrains Mono, monospace'
  if (dataSource.value === 'all' || dataSource.value === 'tx') {
    ctx.fillStyle = '#2196f3'
    ctx.fillRect(pad.left + 10, pad.top + 5, 12, 3)
    ctx.fillText('上行 (TX)', pad.left + 28, pad.top + 12)
  }
  if (dataSource.value === 'all' || dataSource.value === 'rx') {
    ctx.fillStyle = '#4caf50'
    ctx.fillRect(pad.left + 110, pad.top + 5, 12, 3)
    ctx.fillText('下行 (RX)', pad.left + 128, pad.top + 12)
  }
}

function startChart() {
  running.value = true
  paused.value = false
  addDataPoint()
  intervalTimer = setInterval(addDataPoint, 200)
}

function stopChart() {
  running.value = false
  if (intervalTimer) {
    clearInterval(intervalTimer)
    intervalTimer = null
  }
}

function togglePause() {
  paused.value = !paused.value
}

function clearChart() {
  txData.value = []
  rxData.value = []
  peakTx.value = 0
  peakRx.value = 0
  avgTx.value = 0
  avgRx.value = 0
  totalTx.value = 0
  totalRx.value = 0
  currentTx.value = 0
  currentRx.value = 0
  drawChart()
}

function exportChart() {
  const canvas = canvasRef.value
  if (!canvas) return
  const link = document.createElement('a')
  link.download = `network_chart_${Date.now()}.png`
  link.href = canvas.toDataURL('image/png')
  link.click()
}

function formatBytes(bytes: number): string {
  if (bytes > 1000000) return (bytes / 1000000).toFixed(2) + ' MB'
  if (bytes > 1000) return (bytes / 1000).toFixed(1) + ' KB'
  return String(bytes) + ' B'
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
  <div class="network-chart">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">数据源:</label>
        <select v-model="dataSource" class="select-sm">
          <option value="all">上行 + 下行</option>
          <option value="tx">仅上行</option>
          <option value="rx">仅下行</option>
        </select>
      </div>
      <div class="toolbar-group">
        <span class="legend-dot tx-dot"></span>
        <span class="toolbar-label">上行</span>
        <span class="legend-dot rx-dot"></span>
        <span class="toolbar-label">下行</span>
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
      <div class="stat-card tx">
        <div class="stat-title">上行 (TX)</div>
        <div class="stat-row">
          <div class="stat-item">
            <span class="stat-label">当前</span>
            <span class="stat-val mono">{{ currentTx }} B/s</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">峰值</span>
            <span class="stat-val mono">{{ peakTx }} B/s</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">平均</span>
            <span class="stat-val mono">{{ avgTx }} B/s</span>
          </div>
        </div>
      </div>
      <div class="stat-card rx">
        <div class="stat-title">下行 (RX)</div>
        <div class="stat-row">
          <div class="stat-item">
            <span class="stat-label">当前</span>
            <span class="stat-val mono">{{ currentRx }} B/s</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">峰值</span>
            <span class="stat-val mono">{{ peakRx }} B/s</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">平均</span>
            <span class="stat-val mono">{{ avgRx }} B/s</span>
          </div>
        </div>
      </div>
      <div class="stat-card total">
        <div class="stat-title">总计</div>
        <div class="stat-row">
          <div class="stat-item">
            <span class="stat-label">发送</span>
            <span class="stat-val mono">{{ formatBytes(totalTx) }}</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">接收</span>
            <span class="stat-val mono">{{ formatBytes(totalRx) }}</span>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>时间窗口: 60秒</span>
      <span>{{ running ? (paused ? '已暂停' : '采集中') : '已停止' }}</span>
    </div>
  </div>
</template>

<style scoped>
.network-chart {
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
.legend-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
}
.tx-dot { background: #2196f3; }
.rx-dot { background: #4caf50; }
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
.select-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
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
}
.stat-card {
  flex: 1;
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  padding: 6px 10px;
}
.stat-card.tx { border-left: 3px solid #2196f3; }
.stat-card.rx { border-left: 3px solid #4caf50; }
.stat-card.total { border-left: 3px solid var(--primary); }
.stat-title {
  font-weight: 600;
  font-size: 11px;
  margin-bottom: 4px;
}
.stat-row {
  display: flex;
  gap: 12px;
}
.stat-item {
  display: flex;
  flex-direction: column;
  gap: 1px;
}
.stat-label {
  font-size: 10px;
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
