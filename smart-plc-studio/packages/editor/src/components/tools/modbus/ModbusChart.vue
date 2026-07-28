<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, watch, nextTick } from 'vue'

interface ChartChannel {
  address: number
  label: string
  color: string
  data: { time: number; value: number }[]
  visible: boolean
}

const canvasRef = ref<HTMLCanvasElement | null>(null)
const paused = ref(false)
const maxPoints = ref(200)
const refreshInterval = ref(1000)

const channels = ref<ChartChannel[]>([
  { address: 100, label: 'Reg 100', color: '#42a5f5', data: [], visible: true },
  { address: 101, label: 'Reg 101', color: '#66bb6a', data: [], visible: true },
  { address: 102, label: 'Reg 102', color: '#ff9800', data: [], visible: true },
  { address: 103, label: 'Reg 103', color: '#e91e63', data: [], visible: false },
])

const newAddress = ref(200)
let timer: ReturnType<typeof setInterval> | null = null
let animId: number | null = null

const COLORS = ['#42a5f5', '#66bb6a', '#ff9800', '#e91e63']

function addChannel() {
  const color = COLORS[channels.value.length % COLORS.length]
  channels.value.push({
    address: newAddress.value,
    label: `Reg ${newAddress.value}`,
    color,
    data: [],
    visible: true,
  })
}

function removeChannel(idx: number) {
  channels.value.splice(idx, 1)
}

function toggleChannel(idx: number) {
  channels.value[idx].visible = !channels.value[idx].visible
}

function simulateData() {
  const now = Date.now()
  channels.value.forEach(ch => {
    const lastVal = ch.data.length > 0 ? ch.data[ch.data.length - 1].value : 5000
    const newVal = Math.max(0, Math.min(65535, lastVal + (Math.random() - 0.5) * 2000))
    ch.data.push({ time: now, value: Math.round(newVal) })
    if (ch.data.length > maxPoints.value) {
      ch.data.shift()
    }
  })
}

function startRefresh() {
  if (timer) clearInterval(timer)
  paused.value = false
  timer = setInterval(() => {
    if (!paused.value) {
      simulateData()
    }
  }, refreshInterval.value)
  simulateData()
}

function stopRefresh() {
  if (timer) {
    clearInterval(timer)
    timer = null
  }
}

function togglePause() {
  paused.value = !paused.value
}

function clearAll() {
  channels.value.forEach(ch => { ch.data = [] })
}

function onIntervalChange() {
  if (timer) {
    stopRefresh()
    startRefresh()
  }
}

function drawChart() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const rect = canvas.parentElement!.getBoundingClientRect()
  const dpr = window.devicePixelRatio || 1
  canvas.width = rect.width * dpr
  canvas.height = rect.height * dpr
  canvas.style.width = rect.width + 'px'
  canvas.style.height = rect.height + 'px'
  ctx.scale(dpr, dpr)

  const W = rect.width
  const H = rect.height
  const padLeft = 60
  const padRight = 20
  const padTop = 20
  const padBottom = 30
  const chartW = W - padLeft - padRight
  const chartH = H - padTop - padBottom

  ctx.clearRect(0, 0, W, H)

  // 找到全局时间范围和值范围
  let minTime = Infinity, maxTime = -Infinity
  let minVal = Infinity, maxVal = -Infinity
  let hasData = false

  channels.value.forEach(ch => {
    if (!ch.visible || ch.data.length === 0) return
    ch.data.forEach(d => {
      hasData = true
      if (d.time < minTime) minTime = d.time
      if (d.time > maxTime) maxTime = d.time
      if (d.value < minVal) minVal = d.value
      if (d.value > maxVal) maxVal = d.value
    })
  })

  if (!hasData) {
    ctx.fillStyle = '#888'
    ctx.font = '14px sans-serif'
    ctx.textAlign = 'center'
    ctx.fillText('等待数据...', W / 2, H / 2)
    if (animId) cancelAnimationFrame(animId)
    animId = requestAnimationFrame(drawChart)
    return
  }

  // Y轴范围留余量
  const valRange = maxVal - minVal || 1
  minVal = Math.max(0, minVal - valRange * 0.1)
  maxVal = maxVal + valRange * 0.1
  const finalRange = maxVal - minVal

  // 绘制网格
  ctx.strokeStyle = 'rgba(128,128,128,0.2)'
  ctx.lineWidth = 1

  // 水平网格线
  const yTicks = 5
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.fillStyle = '#888'
  ctx.textAlign = 'right'
  for (let i = 0; i <= yTicks; i++) {
    const y = padTop + chartH - (i / yTicks) * chartH
    ctx.beginPath()
    ctx.moveTo(padLeft, y)
    ctx.lineTo(padLeft + chartW, y)
    ctx.stroke()
    const val = minVal + (i / yTicks) * finalRange
    ctx.fillText(val.toFixed(0), padLeft - 6, y + 3)
  }

  // 垂直网格线(时间轴)
  const xTicks = 6
  ctx.textAlign = 'center'
  const timeRange = maxTime - minTime || 1
  for (let i = 0; i <= xTicks; i++) {
    const x = padLeft + (i / xTicks) * chartW
    ctx.beginPath()
    ctx.moveTo(x, padTop)
    ctx.lineTo(x, padTop + chartH)
    ctx.stroke()
    const t = minTime + (i / xTicks) * timeRange
    const d = new Date(t)
    const label = d.toLocaleTimeString('zh-CN', { hour12: false })
    ctx.fillText(label, x, padTop + chartH + 16)
  }

  // Y轴标签
  ctx.save()
  ctx.translate(12, padTop + chartH / 2)
  ctx.rotate(-Math.PI / 2)
  ctx.textAlign = 'center'
  ctx.fillStyle = '#888'
  ctx.font = '11px sans-serif'
  ctx.fillText('值', 0, 0)
  ctx.restore()

  // 绘制曲线
  channels.value.forEach(ch => {
    if (!ch.visible || ch.data.length < 2) return
    ctx.strokeStyle = ch.color
    ctx.lineWidth = 2
    ctx.lineJoin = 'round'
    ctx.beginPath()
    ch.data.forEach((d, i) => {
      const x = padLeft + ((d.time - minTime) / timeRange) * chartW
      const y = padTop + chartH - ((d.value - minVal) / finalRange) * chartH
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    })
    ctx.stroke()
  })

  animId = requestAnimationFrame(drawChart)
}

function exportChartData() {
  if (channels.value.every(ch => ch.data.length === 0)) return
  const maxLen = Math.max(...channels.value.map(ch => ch.data.length))
  let csv = '\uFEFF时间'
  channels.value.forEach(ch => { csv += `,${ch.label}` })
  csv += '\n'

  for (let i = 0; i < maxLen; i++) {
    const d = channels.value[0]?.data[i]
    if (!d) continue
    csv += new Date(d.time).toLocaleTimeString('zh-CN', { hour12: false })
    channels.value.forEach(ch => {
      csv += ',' + (ch.data[i]?.value ?? '')
    })
    csv += '\n'
  }

  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `modbus_chart_${Date.now()}.csv`
  a.click()
  URL.revokeObjectURL(url)
}

onMounted(() => {
  startRefresh()
  drawChart()
})

onUnmounted(() => {
  if (timer) clearInterval(timer)
  if (animId) cancelAnimationFrame(animId)
})
</script>

<template>
  <div class="modbus-chart">
    <div class="toolbar">
      <button class="btn" :class="{ 'btn-primary': !paused }" @click="togglePause">
        <span class="material-symbols-outlined" style="font-size:16px">{{ paused ? 'play_arrow' : 'pause' }}</span>
        {{ paused ? '继续' : '暂停' }}
      </button>
      <div class="toolbar-separator" />
      <label class="toolbar-label">数据点:</label>
      <select v-model.number="maxPoints" class="select-sm">
        <option :value="100">100</option>
        <option :value="200">200</option>
        <option :value="500">500</option>
        <option :value="1000">1000</option>
      </select>
      <label class="toolbar-label">刷新:</label>
      <select v-model.number="refreshInterval" class="select-sm" @change="onIntervalChange">
        <option :value="100">100ms</option>
        <option :value="500">500ms</option>
        <option :value="1000">1s</option>
        <option :value="2000">2s</option>
        <option :value="5000">5s</option>
      </select>
      <div class="toolbar-spacer" />
      <label class="toolbar-label">添加通道:</label>
      <input v-model.number="newAddress" type="number" class="input-sm" style="width:64px" min="0" max="65535" />
      <button class="btn" @click="addChannel">
        <span class="material-symbols-outlined" style="font-size:16px">add</span>
        添加
      </button>
      <button class="btn" @click="exportChartData">
        <span class="material-symbols-outlined" style="font-size:16px">download</span>
        导出
      </button>
      <button class="btn" @click="clearAll">
        <span class="material-symbols-outlined" style="font-size:16px">delete_sweep</span>
        清空
      </button>
    </div>

    <div class="main-content">
      <div class="chart-area">
        <canvas ref="canvasRef" class="chart-canvas" />
      </div>
      <div class="channel-sidebar">
        <div class="sidebar-header">
          <span class="material-symbols-outlined" style="font-size:14px">palette</span>
          通道
        </div>
        <div v-for="(ch, i) in channels" :key="i" class="channel-item">
          <div class="channel-color" :style="{ background: ch.color, opacity: ch.visible ? 1 : 0.3 }" />
          <div class="channel-info">
            <div class="channel-label">{{ ch.label }}</div>
            <div class="channel-addr mono">0x{{ ch.address.toString(16).toUpperCase().padStart(4, '0') }}</div>
            <div class="channel-last mono">
              {{ ch.data.length > 0 ? ch.data[ch.data.length - 1].value : '--' }}
            </div>
          </div>
          <button class="btn-icon" @click="toggleChannel(i)">
            <span class="material-symbols-outlined" style="font-size:14px">{{ ch.visible ? 'visibility' : 'visibility_off' }}</span>
          </button>
          <button class="btn-icon" @click="removeChannel(i)">
            <span class="material-symbols-outlined" style="font-size:14px">close</span>
          </button>
        </div>
        <div v-if="channels.length === 0" class="sidebar-empty">无通道</div>
      </div>
    </div>

    <div class="status-bar">
      <span>通道: {{ channels.filter(c => c.visible).length }}/{{ channels.length }}</span>
      <span>数据点上限: {{ maxPoints }}</span>
      <span>状态: {{ paused ? '已暂停' : '运行中' }}</span>
    </div>
  </div>
</template>

<style scoped>
.modbus-chart {
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
  width: 22px;
  height: 22px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: var(--on-surface-variant);
  cursor: pointer;
  flex-shrink: 0;
}

.btn-icon:hover {
  background: var(--surface-variant);
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

.mono {
  font-family: 'JetBrains Mono', monospace;
}

.main-content {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.chart-area {
  flex: 1;
  position: relative;
  overflow: hidden;
}

.chart-canvas {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
}

.channel-sidebar {
  flex: 0 0 160px;
  border-left: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.sidebar-header {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 8px;
  background: var(--surface-container);
  font-weight: 600;
  font-size: 11px;
  border-bottom: 1px solid var(--outline-variant);
}

.channel-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px;
  border-bottom: 1px solid var(--outline-variant);
}

.channel-color {
  width: 12px;
  height: 12px;
  border-radius: 3px;
  flex-shrink: 0;
}

.channel-info {
  flex: 1;
  min-width: 0;
}

.channel-label {
  font-weight: 600;
  font-size: 11px;
}

.channel-addr {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.channel-last {
  font-size: 11px;
  font-weight: 700;
  color: var(--on-surface);
}

.sidebar-empty {
  text-align: center;
  padding: 20px;
  color: var(--on-surface-variant);
  font-size: 12px;
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
