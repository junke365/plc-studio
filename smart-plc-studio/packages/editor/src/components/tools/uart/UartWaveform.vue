<template>
  <div class="uart-waveform">
    <div class="toolbar">
      <div class="toolbar-group">
        <button class="btn" :class="{ active: isCapturing }" @click="toggleCapture">
          <span class="material-symbols-outlined">{{ isCapturing ? 'stop' : 'play_arrow' }}</span>
          {{ isCapturing ? '停止' : '开始捕获' }}
        </button>
        <button class="btn" @click="clearWaveform">
          <span class="material-symbols-outlined">delete</span>
          清空
        </button>
      </div>
      <div class="toolbar-group">
        <label class="label-sm">时间窗口:</label>
        <select v-model="timeWindow" class="select-sm">
          <option :value="100">100ms</option>
          <option :value="500">500ms</option>
          <option :value="1000">1s</option>
          <option :value="5000">5s</option>
        </select>
        <label class="label-sm">缩放:</label>
        <input type="range" v-model.number="zoom" min="0.5" max="4" step="0.1" class="range-sm" />
      </div>
    </div>
    <div class="waveform-canvas">
      <canvas ref="canvasRef" :width="canvasWidth" :height="canvasHeight"></canvas>
    </div>
    <div class="legend">
      <div class="legend-item"><span class="legend-color" style="background:#4caf50"></span> TX (发送)</div>
      <div class="legend-item"><span class="legend-color" style="background:#2196f3"></span> RX (接收)</div>
    </div>
    <div class="info-bar">
      <span>采样点: {{ sampleCount }}</span>
      <span>时间范围: {{ timeWindow }}ms</span>
      <span>{{ isCapturing ? '采集中...' : '已停止' }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onUnmounted } from 'vue'
import { onSerialData } from '@/serial/serialClient'

const canvasRef = ref<HTMLCanvasElement | null>(null)
const canvasWidth = ref(800)
const canvasHeight = ref(200)
const isCapturing = ref(false)
const timeWindow = ref(500)
const zoom = ref(1)
const sampleCount = ref(0)

interface Sample { time: number; tx: number; rx: number }
const samples = ref<Sample[]>([])
let animationId: number | null = null
let cleanupData: (() => void) | null = null

let simTimer: ReturnType<typeof setInterval> | null = null

function toggleCapture() {
  isCapturing.value = !isCapturing.value
  if (isCapturing.value) startCapture()
  else stopCapture()
}

function startCapture() {
  cleanupData = onSerialData('*', (evt) => {
    if (!isCapturing.value) return
    const now = performance.now()
    for (const byte of evt.data) {
      for (let bit = 0; bit < 8; bit++) {
        samples.value.push({ time: now, tx: (byte >> bit) & 1, rx: (byte >> bit) & 1 })
        sampleCount.value++
      }
    }
    const cutoff = now - timeWindow.value
    samples.value = samples.value.filter(s => s.time >= cutoff)
  })
  const draw = () => {
    if (!isCapturing.value) return
    drawWaveform()
    animationId = requestAnimationFrame(draw)
  }
  draw()
}

function stopCapture() {
  if (animationId) { cancelAnimationFrame(animationId); animationId = null }
  if (simTimer) { clearInterval(simTimer); simTimer = null }
  if (cleanupData) { cleanupData(); cleanupData = null }
}

function clearWaveform() { samples.value = []; sampleCount.value = 0; drawWaveform() }

function drawWaveform() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return
  ctx.fillStyle = '#1e1e1e'
  ctx.fillRect(0, 0, canvasWidth.value, canvasHeight.value)
  const midY = canvasHeight.value / 2
  const txHeight = midY - 20
  const rxHeight = midY + 20
  const high = 30
  const low = 10
  ctx.strokeStyle = '#333'; ctx.lineWidth = 0.5
  for (let x = 0; x < canvasWidth.value; x += 50) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, canvasHeight.value); ctx.stroke() }
  if (samples.value.length < 2) return
  const timeRange = timeWindow.value
  const now = performance.now()
  ctx.strokeStyle = '#4caf50'; ctx.lineWidth = 1.5; ctx.beginPath()
  let started = false
  for (const sample of samples.value) {
    const x = ((sample.time - (now - timeRange)) / timeRange) * canvasWidth.value * zoom.value
    const y = sample.tx ? txHeight - high : txHeight - low
    if (!started) { ctx.moveTo(x, y); started = true } else ctx.lineTo(x, y)
  }
  ctx.stroke()
  ctx.strokeStyle = '#2196f3'; ctx.beginPath(); started = false
  for (const sample of samples.value) {
    const x = ((sample.time - (now - timeRange)) / timeRange) * canvasWidth.value * zoom.value
    const y = sample.rx ? rxHeight - high : rxHeight - low
    if (!started) { ctx.moveTo(x, y); started = true } else ctx.lineTo(x, y)
  }
  ctx.stroke()
}

onUnmounted(() => { stopCapture() })
</script>

<style scoped>
.uart-waveform {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--surface);
}
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
  flex-wrap: wrap;
  gap: 8px;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
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
.btn.active {
  background: var(--primary);
  color: var(--on-primary);
}
.btn .material-symbols-outlined { font-size: 14px; }
.select-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
}
.label-sm {
  font-size: 11px;
  color: var(--on-surface-variant);
}
.range-sm {
  width: 80px;
}
.waveform-canvas {
  flex: 1;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #1e1e1e;
}
.waveform-canvas canvas {
  width: 100%;
  height: 100%;
}
.legend {
  display: flex;
  gap: 16px;
  padding: 6px 12px;
  border-top: 1px solid var(--outline-variant);
}
.legend-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  color: var(--on-surface-variant);
}
.legend-color {
  width: 12px;
  height: 3px;
  border-radius: 2px;
}
.info-bar {
  display: flex;
  justify-content: space-between;
  padding: 4px 12px;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
}
</style>
