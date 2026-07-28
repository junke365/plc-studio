<template>
  <div class="motion-chart">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">route</span>
        运动轨迹图表
      </span>
      <div class="toolbar-actions">
        <select v-model="trajectoryType" class="select-sm">
          <option value="trapezoid">梯形速度曲线</option>
          <option value="scurve">S形速度曲线</option>
          <option value="jog">Jog 曲线</option>
        </select>
        <button class="btn" :class="{ 'btn-primary': animating }" @click="toggleAnimation">
          <span class="material-symbols-outlined">{{ animating ? 'pause' : 'play_arrow' }}</span>
          {{ animating ? '暂停' : '播放' }}
        </button>
        <button class="btn" @click="resetAll">
          <span class="material-symbols-outlined">refresh</span>
          重置
        </button>
        <button class="btn btn-primary" @click="exportGcode">
          <span class="material-symbols-outlined">download</span>
          导出 G-code
        </button>
      </div>
    </div>

    <div class="main-content">
      <div class="left-panel">
        <!-- 轨迹参数 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">tune</span>
            轨迹参数
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>目标速度 (mm/s)</label>
              <input type="number" v-model.number="params.targetVelocity" class="input-sm" min="1" max="10000" />
            </div>
            <div class="form-row">
              <label>加速度 (mm/s²)</label>
              <input type="number" v-model.number="params.acceleration" class="input-sm" min="1" max="50000" />
            </div>
            <div class="form-row">
              <label>减速度 (mm/s²)</label>
              <input type="number" v-model.number="params.deceleration" class="input-sm" min="1" max="50000" />
            </div>
            <div class="form-row">
              <label>运行时间 (s)</label>
              <input type="number" v-model.number="params.runTime" class="input-sm" min="0.1" max="100" step="0.1" />
            </div>
            <div class="form-row" v-if="trajectoryType === 'scurve'">
              <label>S曲线平滑度</label>
              <input type="range" v-model.number="params.smoothness" min="0.1" max="1" step="0.05" class="param-slider" />
              <span class="slider-val">{{ params.smoothness.toFixed(2) }}</span>
            </div>
            <div class="computed-vals">
              <div class="cv-item">
                <span class="cv-label">加速时间</span>
                <span class="cv-value">{{ accelTime.toFixed(2) }}s</span>
              </div>
              <div class="cv-item">
                <span class="cv-label">减速时间</span>
                <span class="cv-value">{{ decelTime.toFixed(2) }}s</span>
              </div>
              <div class="cv-item">
                <span class="cv-label">总行程</span>
                <span class="cv-value">{{ totalDistance.toFixed(1) }}mm</span>
              </div>
              <div class="cv-item">
                <span class="cv-label">最大加加速度</span>
                <span class="cv-value">{{ maxJerk.toFixed(0) }}mm/s³</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 多轴配置 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">axis</span>
            多轴配置 (最多4轴)
          </div>
          <div class="card-body">
            <div v-for="(axis, idx) in axes" :key="idx" class="axis-row">
              <label class="checkbox-label">
                <input type="checkbox" v-model="axis.enabled" />
                轴{{ idx + 1 }}
              </label>
              <input type="color" v-model="axis.color" class="color-pick" :disabled="!axis.enabled" />
              <input type="number" v-model.number="axis.scale" class="input-sm" style="width:60px" :disabled="!axis.enabled" placeholder="缩放" title="速度缩放因子" />
              <span style="font-size:10px;color:var(--on-surface-variant)">×{{ axis.scale.toFixed(1) }}</span>
            </div>
          </div>
        </div>

        <!-- 轨迹数据表格 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">table_chart</span>
            轨迹数据 (关键点)
          </div>
          <div class="card-body" style="max-height:200px;overflow-y:auto;">
            <table class="data-table">
              <thead>
                <tr>
                  <th>时间(s)</th>
                  <th>位置(mm)</th>
                  <th>速度(mm/s)</th>
                  <th>加速度(mm/s²)</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(pt, idx) in trajectoryData" :key="idx">
                  <td>{{ pt.time.toFixed(3) }}</td>
                  <td>{{ pt.position.toFixed(1) }}</td>
                  <td>{{ pt.velocity.toFixed(1) }}</td>
                  <td>{{ pt.accel.toFixed(1) }}</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <!-- 速度/加速度图表 -->
        <div class="card chart-card">
          <div class="card-header">
            <span class="material-symbols-outlined">show_chart</span>
            速度-加速度图表
            <div class="chart-legend">
              <span class="legend-item"><span class="legend-dot" style="background:#42a5f5;"></span>速度</span>
              <span class="legend-item"><span class="legend-dot" style="background:#ffa726;"></span>加速度</span>
              <span class="legend-item"><span class="legend-dot" style="background:#66bb6a;"></span>位置</span>
            </div>
          </div>
          <div class="card-body chart-body">
            <canvas ref="mainChart"></canvas>
          </div>
        </div>

        <!-- 多轴联动显示 -->
        <div class="card chart-card" style="height:200px;">
          <div class="card-header">
            <span class="material-symbols-outlined">compare_arrows</span>
            多轴联动
            <div class="chart-legend">
              <span v-for="(axis, idx) in axes.filter(a => a.enabled)" :key="idx" class="legend-item">
                <span class="legend-dot" :style="{ background: axis.color }"></span>
                轴{{ idx + 1 }}
              </span>
            </div>
          </div>
          <div class="card-body chart-body">
            <canvas ref="multiAxisChart"></canvas>
          </div>
        </div>

        <!-- 播放进度 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">animation</span>
            动画模拟
          </div>
          <div class="card-body">
            <div class="playback-bar">
              <span class="time-display">{{ currentTime.toFixed(2) }}s / {{ params.runTime.toFixed(1) }}s</span>
              <input type="range" v-model.number="currentTime" :max="params.runTime" step="0.01" class="param-slider" />
              <span class="time-display">{{ (currentTime / params.runTime * 100).toFixed(0) }}%</span>
            </div>
            <div class="playback-info">
              <span>当前位置: {{ currentPos.toFixed(1) }}mm</span>
              <span>当前速度: {{ currentVel.toFixed(1) }}mm/s</span>
              <span>当前加速度: {{ currentAccel.toFixed(1) }}mm/s²</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>轨迹类型: {{ trajectoryType === 'trapezoid' ? '梯形' : trajectoryType === 'scurve' ? 'S形' : 'Jog' }}</span>
      <span>目标速度: {{ params.targetVelocity }}mm/s</span>
      <span>总行程: {{ totalDistance.toFixed(1) }}mm</span>
      <span>播放: {{ animating ? '运行中' : '已暂停' }}</span>
      <span>时间: {{ currentTime.toFixed(2) }}s</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted, nextTick, watch } from 'vue'

const trajectoryType = ref<'trapezoid' | 'scurve' | 'jog'>('trapezoid')
const animating = ref(false)
const currentTime = ref(0)

const params = reactive({
  targetVelocity: 200,
  acceleration: 500,
  deceleration: 400,
  runTime: 3,
  smoothness: 0.5,
})

interface AxisConfig {
  enabled: boolean
  color: string
  scale: number
}

const axes = reactive<AxisConfig[]>([
  { enabled: true, color: '#42a5f5', scale: 1.0 },
  { enabled: true, color: '#66bb6a', scale: 0.8 },
  { enabled: false, color: '#ffa726', scale: 1.2 },
  { enabled: false, color: '#ef5350', scale: 0.5 },
])

interface TrajectoryPoint {
  time: number
  position: number
  velocity: number
  accel: number
}

const trajectoryData = ref<TrajectoryPoint[]>([])

const mainChart = ref<HTMLCanvasElement | null>(null)
const multiAxisChart = ref<HTMLCanvasElement | null>(null)

const accelTime = computed(() => params.targetVelocity / params.acceleration)
const decelTime = computed(() => params.targetVelocity / params.deceleration)
const cruiseTime = computed(() => Math.max(0, params.runTime - accelTime.value - decelTime.value))

const totalDistance = computed(() => {
  if (trajectoryType.value === 'trapezoid') {
    const accDist = 0.5 * params.acceleration * accelTime.value ** 2
    const decDist = 0.5 * params.deceleration * decelTime.value ** 2
    const cruiseDist = params.targetVelocity * cruiseTime.value
    return accDist + decDist + cruiseDist
  } else if (trajectoryType.value === 'scurve') {
    return params.targetVelocity * params.runTime * 0.85
  } else {
    return params.targetVelocity * params.runTime * 0.5
  }
})

const maxJerk = computed(() => params.acceleration / (accelTime.value * params.smoothness))

const currentPos = computed(() => {
  const pt = getTrajectoryAt(currentTime.value)
  return pt.position
})

const currentVel = computed(() => {
  const pt = getTrajectoryAt(currentTime.value)
  return pt.velocity
})

const currentAccel = computed(() => {
  const pt = getTrajectoryAt(currentTime.value)
  return pt.accel
})

function getTrajectoryAt(t: number): TrajectoryPoint {
  const ta = accelTime.value
  const td = decelTime.value
  const tc = cruiseTime.value
  let position = 0
  let velocity = 0
  let accel = 0

  if (trajectoryType.value === 'trapezoid') {
    if (t <= ta) {
      velocity = params.acceleration * t
      accel = params.acceleration
      position = 0.5 * params.acceleration * t * t
    } else if (t <= ta + tc) {
      velocity = params.targetVelocity
      accel = 0
      position = 0.5 * params.acceleration * ta * ta + params.targetVelocity * (t - ta)
    } else if (t <= ta + tc + td) {
      const dt = t - ta - tc
      velocity = params.targetVelocity - params.deceleration * dt
      accel = -params.deceleration
      position = 0.5 * params.acceleration * ta * ta + params.targetVelocity * tc + params.targetVelocity * dt - 0.5 * params.deceleration * dt * dt
    } else {
      velocity = 0
      accel = 0
      position = totalDistance.value
    }
  } else if (trajectoryType.value === 'scurve') {
    const totalT = params.runTime
    const s = Math.min(1, t / totalT)
    const ss = s * s
    const sss = ss * s
    velocity = params.targetVelocity * (3 * ss - 2 * sss)
    accel = (params.targetVelocity / totalT) * (6 * s - 6 * ss)
    position = totalDistance.value * (3 * ss - 2 * sss)
  } else {
    velocity = params.targetVelocity * Math.sin(Math.PI * t / params.runTime)
    accel = (params.targetVelocity * Math.PI / params.runTime) * Math.cos(Math.PI * t / params.runTime)
    position = totalDistance.value * (1 - Math.cos(Math.PI * t / params.runTime)) / 2
  }

  velocity = Math.max(0, velocity)
  return { time: t, position, velocity, accel }
}

function generateTrajectoryData() {
  const data: TrajectoryPoint[] = []
  const steps = 20
  for (let i = 0; i <= steps; i++) {
    data.push(getTrajectoryAt((params.runTime / steps) * i))
  }
  trajectoryData.value = data
}

function drawMainChart() {
  const canvas = mainChart.value
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
  const pad = { top: 15, right: 15, bottom: 25, left: 50 }
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

  const maxVel = params.targetVelocity * 1.2
  const maxAccel = params.acceleration * 1.5
  const maxPos = totalDistance.value * 1.1

  function drawCurve(fn: (t: number) => number, maxVal: number, color: string) {
    ctx.beginPath()
    ctx.strokeStyle = color
    ctx.lineWidth = 1.5
    const steps = 200
    for (let i = 0; i <= steps; i++) {
      const t = (params.runTime / steps) * i
      const x = pad.left + (i / steps) * cw
      const y = pad.top + ch - (fn(t) / maxVal) * ch
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
    ctx.stroke()
  }

  drawCurve(t => getTrajectoryAt(t).velocity, maxVel, '#42a5f5')
  drawCurve(t => Math.abs(getTrajectoryAt(t).accel), maxAccel, '#ffa726')
  drawCurve(t => getTrajectoryAt(t).position, maxPos, '#66bb6a')

  // 当前时间线
  if (true) {
    const x = pad.left + (currentTime.value / params.runTime) * cw
    ctx.strokeStyle = 'rgba(255,255,255,0.5)'
    ctx.lineWidth = 1
    ctx.setLineDash([3, 3])
    ctx.beginPath()
    ctx.moveTo(x, pad.top)
    ctx.lineTo(x, pad.top + ch)
    ctx.stroke()
    ctx.setLineDash([])
  }

  // X轴标签
  ctx.fillStyle = 'rgba(255,255,255,0.4)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  for (let i = 0; i <= 5; i++) {
    const t = (params.runTime / 5) * i
    ctx.fillText(t.toFixed(1) + 's', pad.left + (cw / 5) * i, h - 5)
  }
}

function drawMultiAxisChart() {
  const canvas = multiAxisChart.value
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
  const pad = { top: 10, right: 15, bottom: 20, left: 50 }
  const cw = w - pad.left - pad.right
  const ch = h - pad.top - pad.bottom

  ctx.fillStyle = 'rgba(26,26,46,0.95)'
  ctx.fillRect(0, 0, w, h)

  ctx.strokeStyle = 'rgba(255,255,255,0.06)'
  for (let i = 0; i <= 4; i++) {
    const y = pad.top + (ch / 4) * i
    ctx.beginPath()
    ctx.moveTo(pad.left, y)
    ctx.lineTo(pad.left + cw, y)
    ctx.stroke()
  }

  const activeAxes = axes.filter(a => a.enabled)
  const maxPos = totalDistance.value * 1.1

  for (let ai = 0; ai < activeAxes.length; ai++) {
    const axis = activeAxes[ai]
    ctx.beginPath()
    ctx.strokeStyle = axis.color
    ctx.lineWidth = 1.5
    const steps = 150
    for (let i = 0; i <= steps; i++) {
      const t = (params.runTime / steps) * i
      const pt = getTrajectoryAt(t)
      const val = pt.position * axis.scale
      const x = pad.left + (i / steps) * cw
      const y = pad.top + ch - (val / (maxPos * axis.scale)) * ch
      if (i === 0) ctx.moveTo(x, y)
      else ctx.lineTo(x, y)
    }
    ctx.stroke()
  }

  // 当前时间线
  const cx = pad.left + (currentTime.value / params.runTime) * cw
  ctx.strokeStyle = 'rgba(255,255,255,0.4)'
  ctx.lineWidth = 1
  ctx.setLineDash([3, 3])
  ctx.beginPath()
  ctx.moveTo(cx, pad.top)
  ctx.lineTo(cx, pad.top + ch)
  ctx.stroke()
  ctx.setLineDash([])

  // X轴标签
  ctx.fillStyle = 'rgba(255,255,255,0.4)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  for (let i = 0; i <= 5; i++) {
    const t = (params.runTime / 5) * i
    ctx.fillText(t.toFixed(1) + 's', pad.left + (cw / 5) * i, h - 4)
  }
}

function toggleAnimation() {
  animating.value = !animating.value
}

function resetAll() {
  animating.value = false
  currentTime.value = 0
  generateTrajectoryData()
  drawMainChart()
  drawMultiAxisChart()
}

function exportGcode() {
  let gcode = '; 运动轨迹 G-code\n'
  gcode += `; 轨迹类型: ${trajectoryType.value}\n`
  gcode += `; 目标速度: ${params.targetVelocity} mm/s\n`
  gcode += `; 加速度: ${params.acceleration} mm/s²\n`
  gcode += `; 减速度: ${params.deceleration} mm/s²\n`
  gcode += '\nG21 ; 毫米\nG90 ; 绝对坐标\n'

  const steps = 100
  for (let i = 0; i <= steps; i++) {
    const t = (params.runTime / steps) * i
    const pt = getTrajectoryAt(t)
    gcode += `G1 X${pt.position.toFixed(3)} F${(pt.velocity * 60).toFixed(0)}\n`
  }

  gcode += 'M30 ; 程序结束\n'
  const blob = new Blob([gcode], { type: 'text/plain' })
  const a = document.createElement('a')
  a.href = URL.createObjectURL(blob)
  a.download = `trajectory_${trajectoryType.value}_${Date.now()}.nc`
  a.click()
}

let animId: number | null = null

function tick() {
  if (animating.value) {
    currentTime.value += 0.016
    if (currentTime.value >= params.runTime) {
      currentTime.value = 0
    }
  }
  drawMainChart()
  drawMultiAxisChart()
}

watch([trajectoryType, () => params.targetVelocity, () => params.acceleration, () => params.deceleration, () => params.runTime, () => params.smoothness], () => {
  generateTrajectoryData()
})

onMounted(() => {
  nextTick(() => {
    generateTrajectoryData()
    animId = window.setInterval(tick, 16)
  })
})

onUnmounted(() => {
  if (animId !== null) clearInterval(animId)
})
</script>

<style scoped>
.motion-chart {
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

.main-content {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.left-panel {
  width: 360px;
  overflow-y: auto;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  border-right: 1px solid var(--outline-variant);
}

.right-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 8px;
  gap: 8px;
  overflow: hidden;
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

.form-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.form-row label {
  width: 120px;
  flex-shrink: 0;
  font-size: 11px;
  color: var(--on-surface-variant);
}

.param-slider {
  flex: 1;
  height: 4px;
  accent-color: var(--primary);
}

.slider-val {
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  min-width: 36px;
  text-align: right;
}

.computed-vals {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.cv-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 4px 8px;
  background: rgba(0,0,0,0.15);
  border-radius: 3px;
  font-size: 11px;
}

.cv-label {
  color: var(--on-surface-variant);
}

.cv-value {
  font-family: 'JetBrains Mono', monospace;
  font-weight: 600;
  color: var(--primary);
}

.axis-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 6px;
  padding: 4px 6px;
  background: rgba(0,0,0,0.1);
  border-radius: 3px;
}

.checkbox-label {
  display: inline-flex;
  align-items: center;
  gap: 3px;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  min-width: 40px;
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

.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.data-table th,
.data-table td {
  padding: 3px 6px;
  border-bottom: 1px solid var(--outline-variant);
  text-align: right;
  font-family: 'JetBrains Mono', monospace;
}

.data-table th {
  background: var(--surface-variant);
  font-weight: 600;
  text-align: center;
  position: sticky;
  top: 0;
}

.chart-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
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

.playback-bar {
  display: flex;
  align-items: center;
  gap: 8px;
}

.time-display {
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  min-width: 80px;
}

.playback-info {
  display: flex;
  gap: 16px;
  margin-top: 6px;
  font-size: 11px;
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