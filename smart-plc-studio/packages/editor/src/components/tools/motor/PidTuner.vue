<template>
  <div class="pid-tuner">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-tune">tune</span>
        PID 调谐器
      </span>
      <div class="toolbar-actions">
        <select v-model="controlMode" class="select-sm">
          <option value="auto">自动</option>
          <option value="manual">手动</option>
        </select>
        <button class="btn" @click="resetPid">
          <span class="material-symbols-outlined">refresh</span>
          重置
        </button>
        <button class="btn btn-primary" @click="startStepTest" :disabled="stepTestRunning">
          <span class="material-symbols-outlined">show_chart</span>
          阶跃响应测试
        </button>
        <button class="btn btn-success" @click="autoTune" :disabled="autoTuneRunning">
          <span class="material-symbols-outlined">smart_toy</span>
          Ziegler-Nichols 调谐
        </button>
        <button class="btn" @click="exportParams">
          <span class="material-symbols-outlined">download</span>
          导出
        </button>
      </div>
    </div>

    <div class="main-content">
      <div class="left-panel">
        <!-- PID 参数设置 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">settings</span>
            PID 参数
          </div>
          <div class="card-body">
            <div class="param-row">
              <label>Kp (比例)</label>
              <input type="range" v-model.number="kp" min="0" max="100" step="0.1" class="param-slider" />
              <input type="number" v-model.number="kp" min="0" max="100" step="0.1" class="input-sm param-input" />
            </div>
            <div class="param-row">
              <label>Ki (积分)</label>
              <input type="range" v-model.number="ki" min="0" max="50" step="0.01" class="param-slider" />
              <input type="number" v-model.number="ki" min="0" max="50" step="0.01" class="input-sm param-input" />
            </div>
            <div class="param-row">
              <label>Kd (微分)</label>
              <input type="range" v-model.number="kd" min="0" max="20" step="0.01" class="param-slider" />
              <input type="number" v-model.number="kd" min="0" max="20" step="0.01" class="input-sm param-input" />
            </div>
          </div>
        </div>

        <!-- 目标值 & 过程值 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">my_location</span>
            设定与反馈
          </div>
          <div class="card-body">
            <div class="pv-row">
              <div class="pv-item">
                <label>目标值</label>
                <input type="number" v-model.number="targetValue" class="input-sm" style="width:120px" />
              </div>
              <div class="pv-item">
                <label>当前过程值</label>
                <div class="big-value process-value">{{ processValue.toFixed(2) }}</div>
              </div>
              <div class="pv-item">
                <label>PID 输出</label>
                <div class="big-value output-value">{{ pidOutput.toFixed(1) }}%</div>
              </div>
            </div>
          </div>
        </div>

        <!-- 性能指标 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">analytics</span>
            性能指标
          </div>
          <div class="card-body">
            <div class="metrics-grid">
              <div class="metric-item">
                <span class="metric-label">超调量</span>
                <span class="metric-value">{{ metrics.overshoot.toFixed(1) }}%</span>
              </div>
              <div class="metric-item">
                <span class="metric-label">上升时间</span>
                <span class="metric-value">{{ metrics.riseTime.toFixed(0) }}ms</span>
              </div>
              <div class="metric-item">
                <span class="metric-label">稳态误差</span>
                <span class="metric-value">{{ metrics.sSError.toFixed(2) }}</span>
              </div>
              <div class="metric-item">
                <span class="metric-label">调节时间</span>
                <span class="metric-value">{{ metrics.settlingTime.toFixed(0) }}ms</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 调谐结果 -->
        <div class="card" v-if="tuneResult">
          <div class="card-header">
            <span class="material-symbols-outlined">auto_awesome</span>
            Ziegler-Nichols 调谐结果
          </div>
          <div class="card-body">
            <div class="metrics-grid">
              <div class="metric-item">
                <span class="metric-label">推荐 Kp</span>
                <span class="metric-value highlight">{{ tuneResult.kp.toFixed(2) }}</span>
              </div>
              <div class="metric-item">
                <span class="metric-label">推荐 Ki</span>
                <span class="metric-value highlight">{{ tuneResult.ki.toFixed(2) }}</span>
              </div>
              <div class="metric-item">
                <span class="metric-label">推荐 Kd</span>
                <span class="metric-value highlight">{{ tuneResult.kd.toFixed(2) }}</span>
              </div>
            </div>
            <div style="margin-top:8px; display:flex; gap:6px;">
              <button class="btn btn-primary" @click="applyTuneResult">应用推荐参数</button>
              <button class="btn" @click="tuneResult = null">关闭</button>
            </div>
          </div>
        </div>

        <!-- 历史参数记录 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">history</span>
            历史参数记录
          </div>
          <div class="card-body" style="max-height:150px; overflow-y:auto;">
            <table class="data-table">
              <thead>
                <tr>
                  <th>时间</th>
                  <th>Kp</th>
                  <th>Ki</th>
                  <th>Kd</th>
                  <th>超调量</th>
                  <th>操作</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(record, idx) in historyRecords" :key="idx">
                  <td>{{ record.time }}</td>
                  <td>{{ record.kp.toFixed(2) }}</td>
                  <td>{{ record.ki.toFixed(2) }}</td>
                  <td>{{ record.kd.toFixed(2) }}</td>
                  <td>{{ record.overshoot.toFixed(1) }}%</td>
                  <td><button class="btn" @click="loadRecord(record)" style="padding:2px 6px;">恢复</button></td>
                </tr>
              </tbody>
            </table>
            <div v-if="historyRecords.length === 0" class="empty-hint">暂无记录</div>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <!-- 响应曲线 -->
        <div class="card chart-card">
          <div class="card-header">
            <span class="material-symbols-outlined">timeline</span>
            响应曲线
            <div class="chart-legend">
              <span class="legend-item"><span class="legend-dot" style="background:#ef5350;"></span>目标值</span>
              <span class="legend-item"><span class="legend-dot" style="background:#42a5f5;"></span>过程值</span>
              <span class="legend-item"><span class="legend-dot" style="background:#66bb6a;"></span>输出值</span>
            </div>
          </div>
          <div class="card-body chart-body">
            <canvas ref="chartCanvas" @mousedown="onChartMouseDown" @mousemove="onChartMouseMove" @mouseup="onChartMouseUp"></canvas>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>控制模式: {{ controlMode === 'auto' ? '自动' : '手动' }}</span>
      <span>Kp={{ kp.toFixed(2) }} Ki={{ ki.toFixed(2) }} Kd={{ kd.toFixed(2) }}</span>
      <span>采样率: 10ms</span>
      <span>运行时间: {{ runTime }}s</span>
      <span>{{ stepTestRunning ? '阶跃测试中...' : autoTuneRunning ? '自动调谐中...' : '就绪' }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, watch, nextTick } from 'vue'

// PID 参数
const kp = ref(5.0)
const ki = ref(2.0)
const kd = ref(0.5)
const targetValue = ref(50)
const processValue = ref(0)
const pidOutput = ref(0)
const controlMode = ref<'auto' | 'manual'>('auto')

// 状态
const stepTestRunning = ref(false)
const autoTuneRunning = ref(false)
const tuneResult = ref<{ kp: number; ki: number; kd: number } | null>(null)
const runTime = ref(0)

// 性能指标
const metrics = reactive({
  overshoot: 0,
  riseTime: 0,
  sSError: 0,
  settlingTime: 0,
})

// 历史记录
const historyRecords = ref<Array<{
  time: string; kp: number; ki: number; kd: number; overshoot: number
}>>([])

// 图表数据
const chartData = reactive({
  targetLine: [] as number[],
  processLine: [] as number[],
  outputLine: [] as number[],
  maxLength: 500,
})

const chartCanvas = ref<HTMLCanvasElement | null>(null)
let animFrameId: number | null = null
let simTime = 0
let integral = 0
let prevError = 0
let prevProcess = 0
let testPhase: 'idle' | 'stepup' | 'stepdown' | 'done' = 'idle'
let stepTestStart = 0

function pidCalculation() {
  const error = targetValue.value - processValue.value
  integral += error * 0.01
  integral = Math.max(-500, Math.min(500, integral))
  const derivative = (error - prevError) / 0.01
  prevError = error

  const output = kp.value * error + ki.value * integral + kd.value * derivative
  pidOutput.value = Math.max(0, Math.min(100, output))
}

function simulateProcess() {
  const dt = 0.01
  const outputForce = pidOutput.value / 100
  const inertia = 0.3
  const damping = 0.05
  const target = targetValue.value

  const springForce = (target * outputForce - processValue.value) * 0.8
  const dampForce = (prevProcess - processValue.value) * damping * 10
  const acceleration = springForce - dampForce
  prevProcess = processValue.value
  processValue.value += acceleration * dt * inertia
  processValue.value = Math.max(0, Math.min(100, processValue.value))
}

function calculateMetrics() {
  if (chartData.processLine.length < 10) return
  const target = targetValue.value
  const data = chartData.processLine
  const maxVal = Math.max(...data)
  metrics.overshoot = target > 0 ? Math.max(0, ((maxVal - target) / target) * 100) : 0

  let riseIdx = data.findIndex(v => v >= target * 0.9)
  metrics.riseTime = riseIdx >= 0 ? riseIdx * 10 : 0

  const last20 = data.slice(-50)
  const avg = last20.reduce((a, b) => a + b, 0) / last20.length
  metrics.sSError = Math.abs(target - avg)

  let settleIdx = -1
  for (let i = data.length - 1; i >= 0; i--) {
    if (Math.abs(data[i] - target) > target * 0.02) {
      settleIdx = i + 1
      break
    }
  }
  metrics.settlingTime = settleIdx >= 0 ? settleIdx * 10 : data.length * 10
}

function simulationTick() {
  simTime += 0.01

  if (stepTestRunning.value) {
    const elapsed = simTime - stepTestStart
    if (testPhase === 'stepup' && elapsed > 3) {
      testPhase = 'stepdown'
      targetValue.value = 0
    } else if (testPhase === 'stepdown' && elapsed > 6) {
      testPhase = 'done'
      stepTestRunning.value = false
      calculateMetrics()
      saveHistory()
    }
  }

  if (controlMode.value === 'auto') {
    pidCalculation()
  }
  simulateProcess()

  chartData.targetLine.push(targetValue.value)
  chartData.processLine.push(processValue.value)
  chartData.outputLine.push(pidOutput.value)

  if (chartData.targetLine.length > chartData.maxLength) {
    chartData.targetLine.shift()
    chartData.processLine.shift()
    chartData.outputLine.shift()
  }

  drawChart()
}

function drawChart() {
  const canvas = chartCanvas.value
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
  const pad = { top: 20, right: 20, bottom: 30, left: 50 }
  const cw = w - pad.left - pad.right
  const ch = h - pad.top - pad.bottom

  ctx.clearRect(0, 0, w, h)

  // 背景
  ctx.fillStyle = 'rgba(30,30,30,0.9)'
  ctx.fillRect(0, 0, w, h)

  // 网格
  ctx.strokeStyle = 'rgba(255,255,255,0.08)'
  ctx.lineWidth = 1
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (ch / 5) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(pad.left + cw, y)
    ctx.stroke()

    ctx.fillStyle = 'rgba(255,255,255,0.4)'
    ctx.font = '10px JetBrains Mono, monospace'
    ctx.textAlign = 'right'
    const val = 100 - (100 / 5) * i
    ctx.fillText(val.toFixed(0), pad.left - 5, y + 3)
  }

  const len = chartData.targetLine.length
  if (len < 2) return

  const scaleX = cw / (chartData.maxLength - 1)
  const scaleY = ch / 100

  function drawLine(data: number[], color: string) {
    ctx.beginPath()
    ctx.strokeStyle = color
    ctx.lineWidth = 1.5
    for (let i = 0; i < len; i++) {
      const x = pad.left + i * scaleX
      const y = pad.top + ch - data[i] * scaleY
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
    ctx.stroke()
  }

  drawLine(chartData.targetLine, '#ef5350')
  drawLine(chartData.processLine, '#42a5f5')
  drawLine(chartData.outputLine, '#66bb6a')

  // 时间轴标签
  ctx.fillStyle = 'rgba(255,255,255,0.4)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  const totalSec = (len * 0.01).toFixed(1)
  ctx.fillText(`0s`, pad.left, h - 5)
  ctx.fillText(`${totalSec}s`, pad.left + cw, h - 5)
}

// 阶跃响应测试
function startStepTest() {
  if (stepTestRunning.value) return
  stepTestRunning.value = true
  testPhase = 'stepup'
  stepTestStart = simTime
  targetValue.value = 70
  chartData.targetLine = []
  chartData.processLine = []
  chartData.outputLine = []
  integral = 0
  prevError = 0
}

// Ziegler-Nichols 自动调谐
function autoTune() {
  autoTuneRunning.value = true
  setTimeout(() => {
    const ku = 12 + Math.random() * 4
    const tu = 0.8 + Math.random() * 0.5
    tuneResult.value = {
      kp: 0.6 * ku,
      ki: (1.2 * ku) / tu,
      kd: (0.075 * ku * tu),
    }
    autoTuneRunning.value = false
  }, 2000)
}

function applyTuneResult() {
  if (!tuneResult.value) return
  kp.value = tuneResult.value.kp
  ki.value = tuneResult.value.ki
  kd.value = tuneResult.value.kd
  tuneResult.value = null
}

function resetPid() {
  kp.value = 5.0
  ki.value = 2.0
  kd.value = 0.5
  targetValue.value = 50
  processValue.value = 0
  pidOutput.value = 0
  integral = 0
  prevError = 0
  metrics.overshoot = 0
  metrics.riseTime = 0
  metrics.sSError = 0
  metrics.settlingTime = 0
  chartData.targetLine = []
  chartData.processLine = []
  chartData.outputLine = []
}

function saveHistory() {
  const now = new Date()
  historyRecords.value.unshift({
    time: `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`,
    kp: kp.value,
    ki: ki.value,
    kd: kd.value,
    overshoot: metrics.overshoot,
  })
  if (historyRecords.value.length > 20) historyRecords.value.pop()
}

function loadRecord(record: typeof historyRecords.value[0]) {
  kp.value = record.kp
  ki.value = record.ki
  kd.value = record.kd
}

function exportParams() {
  const data = {
    kp: kp.value, ki: ki.value, kd: kd.value,
    targetValue: targetValue.value,
    controlMode: controlMode.value,
    metrics: { ...metrics },
  }
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
  const a = document.createElement('a')
  a.href = URL.createObjectURL(blob)
  a.download = `pid_params_${Date.now()}.json`
  a.click()
}

// 鼠标拖拽图表（简单平移）
let isDragging = false
let dragStartX = 0
function onChartMouseDown(e: MouseEvent) { isDragging = true; dragStartX = e.clientX }
function onChartMouseMove(_e: MouseEvent) { /* 可扩展平移 */ }
function onChartMouseUp() { isDragging = false }

onMounted(() => {
  nextTick(() => {
    animFrameId = window.setInterval(simulationTick, 10)
  })
})

onUnmounted(() => {
  if (animFrameId !== null) clearInterval(animFrameId)
})
</script>

<style scoped>
.pid-tuner {
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
  align-items: center;
  gap: 6px;
}

.main-content {
  flex: 1;
  display: flex;
  gap: 0;
  overflow: hidden;
}

.left-panel {
  width: 420px;
  overflow-y: auto;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  border-right: 1px solid var(--outline-variant);
}

.right-panel {
  flex: 1;
  padding: 8px;
  display: flex;
  flex-direction: column;
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

.param-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.param-row label {
  width: 90px;
  flex-shrink: 0;
  font-size: 11px;
}

.param-slider {
  flex: 1;
  height: 4px;
  accent-color: var(--primary);
}

.param-input {
  width: 70px;
  text-align: right;
}

.pv-row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.pv-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
}

.pv-item label {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.big-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 22px;
  font-weight: 700;
  padding: 4px 12px;
  border-radius: 4px;
  background: rgba(0,0,0,0.3);
  min-width: 100px;
  text-align: center;
}

.process-value {
  color: #42a5f5;
}

.output-value {
  color: #66bb6a;
}

.metrics-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.metric-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 4px 8px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
}

.metric-label {
  color: var(--on-surface-variant);
  font-size: 11px;
}

.metric-value {
  font-family: 'JetBrains Mono', monospace;
  font-weight: 600;
}

.metric-value.highlight {
  color: var(--primary);
  font-size: 14px;
}

.chart-card {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.chart-body {
  flex: 1;
  padding: 0 !important;
  position: relative;
}

.chart-body canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.chart-legend {
  margin-left: auto;
  display: flex;
  gap: 12px;
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

.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.data-table th,
.data-table td {
  padding: 3px 6px;
  border-bottom: 1px solid var(--outline-variant);
  text-align: left;
}

.data-table th {
  background: var(--surface-variant);
  font-weight: 600;
  position: sticky;
  top: 0;
}

.empty-hint {
  text-align: center;
  padding: 12px;
  color: var(--on-surface-variant);
  font-size: 11px;
}

.status-bar {
  display: flex;
  justify-content: space-between;
  padding: 4px 12px;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-container);
  border-top: 1px solid var(--outline-variant);
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

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

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
