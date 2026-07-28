<template>
  <div class="torque-monitor">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">speed</span>
        扭矩/速度监控
      </span>
      <div class="toolbar-actions">
        <button class="btn" :class="{ 'btn-primary': !recording }" @click="togglePause">
          <span class="material-symbols-outlined">{{ paused ? 'play_arrow' : 'pause' }}</span>
          {{ paused ? '继续' : '暂停' }}
        </button>
        <button class="btn" :class="{ 'btn-danger': recording }" @click="toggleRecording">
          <span class="material-symbols-outlined">{{ recording ? 'stop' : 'fiber_manual_record' }}</span>
          {{ recording ? '停止录制' : '开始录制' }}
        </button>
        <button class="btn" @click="exportCSV">
          <span class="material-symbols-outlined">download</span>
          导出 CSV
        </button>
        <button class="btn" @click="clearData">
          <span class="material-symbols-outlined">delete_sweep</span>
          清除
        </button>
        <select v-model.number="sampleRate" class="select-sm">
          <option :value="1">1ms</option>
          <option :value="5">5ms</option>
          <option :value="10">10ms</option>
          <option :value="50">50ms</option>
          <option :value="100">100ms</option>
        </select>
      </div>
    </div>

    <!-- 通道配置 -->
    <div class="channel-bar">
      <div v-for="(ch, idx) in channels" :key="idx" class="channel-item">
        <label class="checkbox-label">
          <input type="checkbox" v-model="ch.enabled" />
          {{ ch.name }}
        </label>
        <input type="color" v-model="ch.color" class="color-pick" />
        <div class="alarm-cfg">
          <span style="font-size:10px;color:var(--on-surface-variant)">上限:</span>
          <input type="number" v-model.number="ch.alarmHigh" class="input-sm" style="width:60px" :disabled="!ch.enabled" />
        </div>
      </div>
    </div>

    <div class="main-content">
      <div class="top-panel">
        <!-- 仪表盘 -->
        <div v-for="(ch, idx) in channels" :key="'gauge-' + idx" class="gauge-card" v-show="ch.enabled">
          <div class="gauge-header">
            <span class="gauge-name" :style="{ color: ch.color }">{{ ch.name }}</span>
            <span class="material-symbols-outlined" v-if="ch.alarmActive" style="color:#ef5350;font-size:16px;">warning</span>
          </div>
          <div class="gauge-body">
            <div class="gauge-ring" :style="{ borderColor: ch.alarmActive ? '#ef5350' : ch.color }">
              <span class="gauge-value">{{ ch.current.toFixed(ch.decimals) }}</span>
              <span class="gauge-unit">{{ ch.unit }}</span>
            </div>
          </div>
          <div class="gauge-stats">
            <span>峰: {{ ch.peak.toFixed(ch.decimals) }}</span>
            <span>谷: {{ ch.valley.toFixed(ch.decimals) }}</span>
            <span>均: {{ ch.avg.toFixed(ch.decimals) }}</span>
          </div>
        </div>
      </div>

      <!-- 趋势图 -->
      <div class="chart-area">
        <div class="card chart-card">
          <div class="card-header">
            <span class="material-symbols-outlined">timeline</span>
            实时趋势图 (最近 60s)
            <div class="chart-legend">
              <span v-for="(ch, idx) in channels.filter(c => c.enabled)" :key="idx" class="legend-item">
                <span class="legend-dot" :style="{ background: ch.color }"></span>
                {{ ch.name }}
              </span>
            </div>
          </div>
          <div class="card-body chart-body">
            <canvas ref="trendCanvas"></canvas>
          </div>
        </div>
      </div>

      <!-- 录制回放 -->
      <div class="card" v-if="recordedData.length > 0" style="flex-shrink:0;">
        <div class="card-header">
          <span class="material-symbols-outlined">movie</span>
          录制回放 ({{ recordedData.length }} 点)
          <button class="btn" style="margin-left:auto;" @click="togglePlayback">
            <span class="material-symbols-outlined">{{ playingBack ? 'pause' : 'play_arrow' }}</span>
            {{ playingBack ? '暂停回放' : '回放' }}
          </button>
          <button class="btn btn-danger" @click="clearRecording" style="margin-left:4px;">
            <span class="material-symbols-outlined">delete</span>
            删除录制
          </button>
        </div>
        <div class="card-body" style="padding:4px 12px;">
          <input type="range" v-model.number="playbackIndex" :max="recordedData.length - 1" class="param-slider" style="width:100%;" />
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>采样率: {{ sampleRate }}ms</span>
      <span>数据点: {{ totalPoints }}</span>
      <span>录制: {{ recording ? '录制中' : '未录制' }}</span>
      <span>{{ paused ? '已暂停' : '采集中' }}</span>
      <span>运行时间: {{ runSeconds.toFixed(0) }}s</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, nextTick } from 'vue'

interface MonitorChannel {
  name: string
  unit: string
  decimals: number
  enabled: boolean
  color: string
  alarmHigh: number
  alarmActive: boolean
  current: number
  peak: number
  valley: number
  avg: number
  data: number[]
}

const channels = reactive<MonitorChannel[]>([
  {
    name: '扭矩', unit: 'N·m', decimals: 2, enabled: true, color: '#ef5350',
    alarmHigh: 80, alarmActive: false, current: 0, peak: 0, valley: 999, avg: 0, data: [],
  },
  {
    name: '速度', unit: 'rpm', decimals: 1, enabled: true, color: '#42a5f5',
    alarmHigh: 3000, alarmActive: false, current: 0, peak: 0, valley: 99999, avg: 0, data: [],
  },
  {
    name: '电流', unit: 'A', decimals: 2, enabled: true, color: '#ffa726',
    alarmHigh: 10, alarmActive: false, current: 0, peak: 0, valley: 999, avg: 0, data: [],
  },
])

const sampleRate = ref(10)
const paused = ref(false)
const recording = ref(false)
const recordedData = ref<Array<number[]>>([])
const playingBack = ref(false)
const playbackIndex = ref(0)
const totalPoints = ref(0)
const runSeconds = ref(0)

const trendCanvas = ref<HTMLCanvasElement | null>(null)

const maxDataPoints = 6000 // 60s at 10ms

function updateStats(ch: MonitorChannel) {
  if (ch.data.length === 0) return
  ch.current = ch.data[ch.data.length - 1]
  ch.peak = Math.max(...ch.data)
  ch.valley = Math.min(...ch.data)
  ch.avg = ch.data.reduce((a, b) => a + b, 0) / ch.data.length
  ch.alarmActive = ch.current > ch.alarmHigh
}

function generateData() {
  const t = runSeconds.value
  for (const ch of channels) {
    let val = 0
    if (ch.name === '扭矩') {
      val = 30 + 20 * Math.sin(t * 0.5) + 5 * Math.sin(t * 2.3) + (Math.random() - 0.5) * 4
    } else if (ch.name === '速度') {
      val = 1500 + 800 * Math.sin(t * 0.3) + 200 * Math.sin(t * 1.7) + (Math.random() - 0.5) * 50
    } else {
      val = 3 + 2 * Math.sin(t * 0.8) + Math.random() * 0.5
    }
    ch.data.push(Math.max(0, val))
    if (ch.data.length > maxDataPoints) ch.data.shift()
    updateStats(ch)
  }
  totalPoints.value = channels[0].data.length
}

function drawTrend() {
  const canvas = trendCanvas.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const dpr = window.devicePixelRatio || 1
  const rect = canvas.parentElement!.getBoundingClientRect()
  canvas.width = rect.width * dpr
  canvas.height = rect.height * dpr
  canvas.style.width = rect.width + 'px'
  canvas.style.height = rect.height + 'px'
  ctx.scale(dpr, dpr)

  const w = rect.width
  const h = rect.height
  const pad = { top: 10, right: 50, bottom: 20, left: 50 }
  const cw = w - pad.left - pad.right
  const ch = h - pad.top - pad.bottom

  ctx.fillStyle = 'rgba(26,26,46,0.95)'
  ctx.fillRect(0, 0, w, h)

  // 网格
  ctx.strokeStyle = 'rgba(255,255,255,0.06)'
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (ch / 5) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(pad.left + cw, y)
    ctx.stroke()
  }
  for (let i = 0; i <= 12; i++) {
    const x = pad.left + (cw / 12) * i
    ctx.beginPath()
    ctx.moveTo(x, pad.top)
    ctx.lineTo(x, pad.top + ch)
    ctx.stroke()
  }

  // 绘制各通道
  for (const channel of channels) {
    if (!channel.enabled || channel.data.length < 2) continue

    const maxVal = channel.alarmHigh * 1.3
    ctx.beginPath()
    ctx.strokeStyle = channel.color
    ctx.lineWidth = 1.5

    const len = channel.data.length
    for (let i = 0; i < len; i++) {
      const x = pad.left + ((i / maxDataPoints) * cw)
      const y = pad.top + ch - (channel.data[i] / maxVal) * ch
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
    ctx.stroke()

    // 报警线
    const alarmY = pad.top + ch - (channel.alarmHigh / maxVal) * ch
    ctx.strokeStyle = channel.color
    ctx.lineWidth = 1
    ctx.globalAlpha = 0.3
    ctx.setLineDash([4, 4])
    ctx.beginPath()
    ctx.moveTo(pad.left, alarmY)
    ctx.lineTo(pad.left + cw, alarmY)
    ctx.stroke()
    ctx.setLineDash([])
    ctx.globalAlpha = 1

    // 右侧Y轴标注
    ctx.fillStyle = channel.color
    ctx.font = '9px JetBrains Mono, monospace'
    ctx.textAlign = 'left'
    ctx.fillText(`${channel.name} ${maxVal.toFixed(0)}${channel.unit}`, pad.left + cw + 4, pad.top + 10)
    ctx.fillText(`报警 ${channel.alarmHigh}`, pad.left + cw + 4, alarmY + 3)
  }

  // 时间轴
  ctx.fillStyle = 'rgba(255,255,255,0.4)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  for (let i = 0; i <= 6; i++) {
    const t = i * 10
    const x = pad.left + (cw / 6) * i
    ctx.fillText(`-${60 - t}s`, x, h - 4)
  }
}

let simInterval: number | null = null

function tick() {
  if (!paused.value) {
    runSeconds.value += sampleRate.value / 1000
    generateData()
    if (recording.value) {
      recordedData.value.push(channels.map(c => c.current))
    }
  }
  if (playingBack.value) {
    playbackIndex.value = Math.min(playbackIndex.value + 1, recordedData.value.length - 1)
    if (playbackIndex.value >= recordedData.value.length - 1) {
      playingBack.value = false
    }
  }
  drawTrend()
}

function togglePause() {
  paused.value = !paused.value
}

function toggleRecording() {
  if (recording.value) {
    recording.value = false
  } else {
    recording.value = true
    recordedData.value = []
  }
}

function togglePlayback() {
  if (playingBack.value) {
    playingBack.value = false
  } else {
    playbackIndex.value = 0
    playingBack.value = true
  }
}

function clearRecording() {
  recordedData.value = []
  playingBack.value = false
  playbackIndex.value = 0
}

function clearData() {
  for (const ch of channels) {
    ch.data = []
    ch.peak = 0
    ch.valley = ch.name === '速度' ? 99999 : 999
    ch.avg = 0
    ch.current = 0
    ch.alarmActive = false
  }
  totalPoints.value = 0
  runSeconds.value = 0
}

function exportCSV() {
  let csv = 'Time'
  channels.forEach(ch => { if (ch.enabled) csv += `,${ch.name}(${ch.unit})` })
  csv += '\n'
  const len = channels[0].data.length
  for (let i = 0; i < len; i++) {
    csv += (i * sampleRate.value / 1000).toFixed(3)
    channels.forEach(ch => { if (ch.enabled) csv += `,${ch.data[i]?.toFixed(ch.decimals) ?? ''}` })
    csv += '\n'
  }
  const blob = new Blob([csv], { type: 'text/csv' })
  const a = document.createElement('a')
  a.href = URL.createObjectURL(blob)
  a.download = `monitor_${Date.now()}.csv`
  a.click()
}

onMounted(() => {
  nextTick(() => {
    simInterval = window.setInterval(tick, sampleRate.value)
  })
})

onUnmounted(() => {
  if (simInterval !== null) clearInterval(simInterval)
})
</script>

<style scoped>
.torque-monitor {
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
  justify-content: space-between;
  padding: 6px 12px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.toolbar-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-weight: 600;
  font-size: 13px;
}

.toolbar-actions {
  display: flex;
  gap: 6px;
  align-items: center;
}

.channel-bar {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 6px 12px;
  background: var(--surface-variant);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.channel-item {
  display: flex;
  align-items: center;
  gap: 6px;
}

.checkbox-label {
  display: inline-flex;
  align-items: center;
  gap: 3px;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
}

.color-pick {
  width: 20px;
  height: 20px;
  border: 1px solid var(--outline-variant);
  border-radius: 3px;
  padding: 0;
  cursor: pointer;
  background: transparent;
}

.alarm-cfg {
  display: flex;
  align-items: center;
  gap: 3px;
}

.main-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.top-panel {
  display: flex;
  gap: 8px;
  padding: 8px;
  flex-shrink: 0;
}

.gauge-card {
  flex: 1;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  padding: 8px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
}

.gauge-header {
  display: flex;
  align-items: center;
  gap: 4px;
}

.gauge-name {
  font-weight: 600;
  font-size: 12px;
}

.gauge-body {
  display: flex;
  justify-content: center;
  align-items: center;
}

.gauge-ring {
  width: 100px;
  height: 100px;
  border-radius: 50%;
  border: 3px solid;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(0,0,0,0.3);
}

.gauge-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 22px;
  font-weight: 700;
}

.gauge-unit {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.gauge-stats {
  display: flex;
  gap: 10px;
  font-size: 10px;
  color: var(--on-surface-variant);
  font-family: 'JetBrains Mono', monospace;
}

.chart-area {
  flex: 1;
  padding: 0 8px 8px 8px;
  min-height: 0;
  display: flex;
  flex-direction: column;
}

.chart-card {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.chart-body {
  flex: 1;
  padding: 0 !important;
}

.chart-body canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.chart-legend {
  margin-left: auto;
  display: flex;
  gap: 10px;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  font-weight: 400;
}

.legend-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  display: inline-block;
}

.param-slider {
  width: 100%;
  height: 4px;
  accent-color: var(--primary);
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

.card {
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  overflow: hidden;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: var(--surface-variant);
  font-weight: 600;
  font-size: 12px;
  border-bottom: 1px solid var(--outline-variant);
}

.card-body {
  padding: 10px;
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

.btn:disabled { opacity: 0.5; cursor: not-allowed; }

.btn-primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.btn-danger {
  background: #c62828;
  color: white;
  border-color: #c62828;
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
</style>