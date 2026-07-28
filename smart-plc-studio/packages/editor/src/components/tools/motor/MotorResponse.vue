<template>
  <div class="motor-response">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">monitor_heart</span>
        电机响应曲线
      </span>
      <div class="toolbar-actions">
        <button class="btn" :class="{ 'btn-primary': !paused }" @click="togglePause">
          <span class="material-symbols-outlined">{{ paused ? 'play_arrow' : 'pause' }}</span>
          {{ paused ? '继续' : '暂停' }}
        </button>
        <button class="btn" @click="clearWaveform">
          <span class="material-symbols-outlined">delete_sweep</span>
          清除
        </button>
        <button class="btn" @click="exportCSV">
          <span class="material-symbols-outlined">download</span>
          导出 CSV
        </button>
        <button class="btn" @click="saveScreenshot">
          <span class="material-symbols-outlined">screenshot_monitor</span>
          截图
        </button>
      </div>
    </div>

    <!-- 通道配置 -->
    <div class="channel-config-bar">
      <div v-for="(ch, idx) in channels" :key="idx" class="channel-cfg">
        <label class="checkbox-label">
          <input type="checkbox" v-model="ch.enabled" />
          CH{{ idx + 1 }}
        </label>
        <select v-model="ch.source" class="select-sm" :disabled="!ch.enabled">
          <option value="position">位置</option>
          <option value="velocity">速度</option>
          <option value="current">电流</option>
          <option value="torque">扭矩</option>
        </select>
        <input type="color" v-model="ch.color" class="color-pick" :disabled="!ch.enabled" />
      </div>
      <div class="time-cfg">
        <label>时间窗口</label>
        <select v-model.number="timeWindow" class="select-sm">
          <option :value="0.1">100ms</option>
          <option :value="0.5">500ms</option>
          <option :value="1">1s</option>
          <option :value="2">2s</option>
          <option :value="5">5s</option>
          <option :value="10">10s</option>
        </select>
      </div>
      <div class="trigger-cfg">
        <label>触发</label>
        <select v-model="trigger.type" class="select-sm">
          <option value="none">无</option>
          <option value="rising">上升沿</option>
          <option value="falling">下降沿</option>
          <option value="level">电平</option>
        </select>
        <input type="number" v-model.number="trigger.level" class="input-sm" style="width:60px" :disabled="trigger.type === 'none'" />
        <span style="font-size:10px;">V</span>
      </div>
    </div>

    <div class="main-content">
      <!-- 波形区域 -->
      <div class="waveform-area">
        <canvas ref="waveformCanvas" @mousedown="onCanvasMouseDown" @mousemove="onCanvasMouseMove" @mouseup="onCanvasMouseUp" @wheel="onCanvasWheel"></canvas>
        <!-- 游标 -->
        <div v-if="cursor1.visible" class="cursor-line cursor1" :style="{ left: cursor1.x + 'px' }">
          <div class="cursor-label">T1: {{ cursor1.time.toFixed(3) }}s</div>
        </div>
        <div v-if="cursor2.visible" class="cursor-line cursor2" :style="{ left: cursor2.x + 'px' }">
          <div class="cursor-label">T2: {{ cursor2.time.toFixed(3) }}s</div>
        </div>
        <!-- 测量结果 -->
        <div v-if="cursor1.visible && cursor2.visible" class="cursor-delta">
          ΔT = {{ Math.abs(cursor2.time - cursor1.time).toFixed(3) }}s
          ΔV = {{ Math.abs(cursor2.value - cursor1.value).toFixed(2) }}
        </div>
      </div>
    </div>

    <!-- 游标信息 -->
    <div class="cursor-info-bar">
      <span>游标1: {{ cursor1.visible ? `${cursor1.time.toFixed(3)}s / ${cursor1.value.toFixed(2)}` : '未放置' }}</span>
      <span>游标2: {{ cursor2.visible ? `${cursor2.time.toFixed(3)}s / ${cursor2.value.toFixed(2)}` : '未放置' }}</span>
      <span>缩放: {{ (zoomLevel * 100).toFixed(0) }}%</span>
      <span>偏移: {{ panOffset.toFixed(0) }}px</span>
      <span>{{ paused ? '已暂停' : '采集中' }} | {{ totalSamples }} 采样点</span>
    </div>

    <div class="status-bar">
      <span>时间窗口: {{ timeWindow }}s</span>
      <span>触发: {{ trigger.type === 'none' ? '无' : trigger.type === 'rising' ? '上升沿' : trigger.type === 'falling' ? '下降沿' : '电平' }}</span>
      <span>采样率: 1kHz</span>
      <span>{{ activeChannelCount }} / 4 通道激活</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted, nextTick } from 'vue'

interface Channel {
  enabled: boolean
  source: string
  color: string
  data: number[]
}

interface Trigger {
  type: 'none' | 'rising' | 'falling' | 'level'
  level: number
}

const channels = reactive<Channel[]>([
  { enabled: true, source: 'position', color: '#42a5f5', data: [] },
  { enabled: true, source: 'velocity', color: '#66bb6a', data: [] },
  { enabled: false, source: 'current', color: '#ffa726', data: [] },
  { enabled: false, source: 'torque', color: '#ef5350', data: [] },
])

const trigger = reactive<Trigger>({ type: 'none', level: 50 })
const timeWindow = ref(2)
const paused = ref(false)
const zoomLevel = ref(1)
const panOffset = ref(0)
const totalSamples = ref(0)
const activeChannelCount = computed(() => channels.filter(c => c.enabled).length)

const waveformCanvas = ref<HTMLCanvasElement | null>(null)

const cursor1 = reactive({ visible: false, x: 0, time: 0, value: 0 })
const cursor2 = reactive({ visible: false, x: 0, time: 0, value: 0 })
let cursorClickCount = 0

const maxDataPoints = computed(() => Math.floor(timeWindow.value * 1000))

// 模拟信号生成
let simTime = 0
function generateSimData() {
  channels[0].data.push(30 + 20 * Math.sin(simTime * 2) + 5 * Math.sin(simTime * 7) + Math.random() * 2)
  channels[1].data.push(60 * Math.cos(simTime * 2) + 35 * Math.cos(simTime * 7) + Math.random() * 3)
  channels[2].data.push(10 + 8 * Math.sin(simTime * 3) + Math.random() * 1.5)
  channels[3].data.push(25 + 15 * Math.sin(simTime * 1.5) + Math.random() * 2)

  for (const ch of channels) {
    if (ch.data.length > maxDataPoints.value + 200) {
      ch.data.splice(0, ch.data.length - maxDataPoints.value)
    }
  }
  totalSamples.value = channels[0].data.length
}

function drawWaveform() {
  const canvas = waveformCanvas.value
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
  const pad = { top: 15, right: 60, bottom: 25, left: 50 }
  const cw = w - pad.left - pad.right
  const ch = h - pad.top - pad.bottom

  // 背景
  ctx.fillStyle = '#1a1a2e'
  ctx.fillRect(0, 0, w, h)

  // 网格
  ctx.strokeStyle = 'rgba(255,255,255,0.06)'
  ctx.lineWidth = 1
  for (let i = 0; i <= 8; i++) {
    const y = pad.top + (ch / 8) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(pad.left + cw, y)
    ctx.stroke()
  }
  for (let i = 0; i <= 10; i++) {
    const x = pad.left + (cw / 10) * i
    ctx.beginPath()
    ctx.moveTo(x, pad.top)
    ctx.lineTo(x, pad.top + ch)
    ctx.stroke()
  }

  // Y轴刻度
  ctx.fillStyle = 'rgba(255,255,255,0.5)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'right'
  for (let i = 0; i <= 8; i++) {
    const y = pad.top + (ch / 8) * i
    const val = 100 - (100 / 8) * i
    ctx.fillText(val.toFixed(0), pad.left - 4, y + 3)
  }

  // X轴时间
  ctx.textAlign = 'center'
  for (let i = 0; i <= 10; i++) {
    const x = pad.left + (cw / 10) * i
    const t = (timeWindow.value / 10) * i
    ctx.fillText(t.toFixed(1) + 's', x, h - 5)
  }

  // 绘制各通道波形
  for (const ch of channels) {
    if (!ch.enabled || ch.data.length < 2) continue
    ctx.beginPath()
    ctx.strokeStyle = ch.color
    ctx.lineWidth = 1.5
    const step = cw / (maxDataPoints.value - 1)
    const offset = panOffset.value
    for (let i = 0; i < ch.data.length; i++) {
      const x = pad.left + (i / maxDataPoints.value) * cw * zoomLevel.value + offset
      const y = pad.top + ch - (ch.data[i] / 100) * ch
      if (x < pad.left || x > pad.left + cw) continue
      if (i === 0 || x === pad.left) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
    ctx.stroke()
  }

  // 触发电平线
  if (trigger.type !== 'none') {
    const ty = pad.top + ch - (trigger.level / 100) * ch
    ctx.strokeStyle = 'rgba(255,193,7,0.5)'
    ctx.lineWidth = 1
    ctx.setLineDash([4, 4])
    ctx.beginPath()
    ctx.moveTo(pad.left, ty)
    ctx.lineTo(pad.left + cw, ty)
    ctx.stroke()
    ctx.setLineDash([])
    ctx.fillStyle = 'rgba(255,193,7,0.7)'
    ctx.font = '10px JetBrains Mono, monospace'
    ctx.textAlign = 'left'
    ctx.fillText(`触发 ${trigger.level}V`, pad.left + cw + 4, ty + 3)
  }
}

let animId: number | null = null

function tick() {
  if (!paused.value) {
    simTime += 0.001
    generateSimData()
  }
  drawWaveform()
}

// 游标交互
function onCanvasMouseDown(e: MouseEvent) {
  const rect = waveformCanvas.value!.getBoundingClientRect()
  const x = e.clientX - rect.left
  const pad = { left: 50, top: 15 }
  const cw = rect.width - 110
  const relX = (x - pad.left) / cw
  const time = relX * timeWindow.value

  // 模拟值读取
  let value = 0
  for (const ch of channels) {
    if (ch.enabled && ch.data.length > 0) {
      const idx = Math.floor(relX * ch.data.length)
      if (idx >= 0 && idx < ch.data.length) value = ch.data[idx]
      break
    }
  }

  cursorClickCount++
  if (cursorClickCount % 2 === 1) {
    cursor1.visible = true
    cursor1.x = x
    cursor1.time = time
    cursor1.value = value
  } else {
    cursor2.visible = true
    cursor2.x = x
    cursor2.time = time
    cursor2.value = value
  }
}

function onCanvasMouseMove(_e: MouseEvent) {}
function onCanvasMouseUp() {}

function onCanvasWheel(e: WheelEvent) {
  e.preventDefault()
  if (e.deltaY < 0) zoomLevel.value = Math.min(5, zoomLevel.value * 1.1)
  else zoomLevel.value = Math.max(0.2, zoomLevel.value / 1.1)
}

function togglePause() { paused.value = !paused.value }

function clearWaveform() {
  for (const ch of channels) ch.data = []
  totalSamples.value = 0
  simTime = 0
  cursorClickCount = 0
  cursor1.visible = false
  cursor2.visible = false
  zoomLevel.value = 1
  panOffset.value = 0
}

function exportCSV() {
  let csv = 'Time'
  channels.forEach((ch, i) => { if (ch.enabled) csv += `,CH${i + 1}_${ch.source}` })
  csv += '\n'
  const len = channels[0].data.length
  for (let i = 0; i < len; i++) {
    csv += (i * 0.001).toFixed(4)
    channels.forEach(ch => { if (ch.enabled) csv += `,${ch.data[i]?.toFixed(3) ?? ''}` })
    csv += '\n'
  }
  const blob = new Blob([csv], { type: 'text/csv' })
  const a = document.createElement('a')
  a.href = URL.createObjectURL(blob)
  a.download = `waveform_${Date.now()}.csv`
  a.click()
}

function saveScreenshot() {
  const canvas = waveformCanvas.value
  if (!canvas) return
  const a = document.createElement('a')
  a.href = canvas.toDataURL('image/png')
  a.download = `waveform_${Date.now()}.png`
  a.click()
}

onMounted(() => {
  nextTick(() => {
    animId = window.setInterval(tick, 1)
  })
})

onUnmounted(() => {
  if (animId !== null) clearInterval(animId)
})
</script>

<style scoped>
.motor-response {
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
}

.channel-config-bar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 6px 12px;
  background: var(--surface-variant);
  border-bottom: 1px solid var(--outline-variant);
  flex-wrap: wrap;
  flex-shrink: 0;
}

.channel-cfg {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 2px 6px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
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
  width: 22px;
  height: 22px;
  border: 1px solid var(--outline-variant);
  border-radius: 3px;
  padding: 0;
  cursor: pointer;
  background: transparent;
}

.time-cfg, .trigger-cfg {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
}

.main-content {
  flex: 1;
  padding: 8px;
  overflow: hidden;
}

.waveform-area {
  position: relative;
  width: 100%;
  height: 100%;
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  overflow: hidden;
}

.waveform-area canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.cursor-line {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 1px;
  pointer-events: none;
}

.cursor1 {
  background: rgba(255, 193, 7, 0.7);
}

.cursor2 {
  background: rgba(156, 39, 176, 0.7);
}

.cursor-label {
  position: absolute;
  top: 2px;
  transform: translateX(-50%);
  font-size: 10px;
  padding: 1px 4px;
  border-radius: 2px;
  white-space: nowrap;
}

.cursor1 .cursor-label {
  background: rgba(255, 193, 7, 0.8);
  color: #000;
}

.cursor2 .cursor-label {
  background: rgba(156, 39, 176, 0.8);
  color: #fff;
}

.cursor-delta {
  position: absolute;
  bottom: 30px;
  right: 10px;
  background: rgba(0,0,0,0.8);
  color: #fff;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 11px;
  font-family: 'JetBrains Mono', monospace;
}

.cursor-info-bar {
  display: flex;
  justify-content: space-between;
  padding: 4px 12px;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-variant);
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
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

.btn-success {
  background: #2e7d32;
  color: white;
  border-color: #2e7d32;
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