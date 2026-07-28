<template>
  <div class="stepper-debug">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">settings_suggest</span>
        步进电机调试
      </span>
      <div class="toolbar-actions">
        <button class="btn" :class="{ 'btn-primary': enabled }" @click="toggleEnable">
          <span class="material-symbols-outlined">{{ enabled ? 'power_settings_new' : 'power_off' }}</span>
          {{ enabled ? '已使能' : '未使能' }}
        </button>
        <button class="btn btn-danger" @click="emergencyStop">
          <span class="material-symbols-outlined">emergency</span>
          急停
        </button>
      </div>
    </div>

    <div class="main-content">
      <div class="left-panel">
        <!-- 电机参数 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">settings</span>
            电机参数
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>步距角 (°)</label>
              <input type="number" v-model.number="motorParams.stepAngle" class="input-sm" step="0.1" />
            </div>
            <div class="form-row">
              <label>细分数</label>
              <select v-model.number="motorParams.microstep" class="select-sm">
                <option :value="1">1 (整步)</option>
                <option :value="2">2 (半步)</option>
                <option :value="4">4</option>
                <option :value="8">8</option>
                <option :value="16">16</option>
                <option :value="32">32</option>
                <option :value="64">64</option>
                <option :value="128">128</option>
                <option :value="256">256</option>
              </select>
            </div>
            <div class="form-row">
              <label>最大速度 (rpm)</label>
              <input type="number" v-model.number="motorParams.maxSpeed" class="input-sm" min="1" max="6000" />
            </div>
            <div class="form-row">
              <label>加速度 (rpm/s²)</label>
              <input type="number" v-model.number="motorParams.acceleration" class="input-sm" min="100" max="100000" />
            </div>
            <div class="form-row">
              <label>脉冲当量</label>
              <div class="computed-value">{{ pulsePerRev.toFixed(1) }} 脉冲/转</div>
            </div>
          </div>
        </div>

        <!-- 运动控制 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">sports_motorsports</span>
            运动控制
          </div>
          <div class="card-body">
            <div class="motion-btns">
              <button class="btn motion-btn rev" @mousedown="startJog(-1)" @mouseup="stopJog" @mouseleave="stopJog">
                <span class="material-symbols-outlined">rotate_left</span>
                反转
              </button>
              <button class="btn btn-danger" @click="stopMotion">
                <span class="material-symbols-outlined">stop</span>
                停止
              </button>
              <button class="btn motion-btn fwd" @mousedown="startJog(1)" @mouseup="stopJog" @mouseleave="stopJog">
                <span class="material-symbols-outlined">rotate_right</span>
                正转
              </button>
            </div>
          </div>
        </div>

        <!-- 位置控制 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">my_location</span>
            位置控制
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>运动模式</label>
              <select v-model="motionMode" class="select-sm">
                <option value="point">点位模式</option>
                <option value="continuous">连续模式</option>
              </select>
            </div>
            <div class="form-row">
              <label>目标位置 (脉冲)</label>
              <input type="number" v-model.number="targetPosition" class="input-sm" />
            </div>
            <div class="form-row">
              <label>目标速度 (%)</label>
              <input type="range" v-model.number="speedPercent" min="0" max="100" class="param-slider" />
              <span class="slider-val">{{ speedPercent }}%</span>
            </div>
            <div class="form-row">
              <button class="btn btn-primary" @click="moveToTarget" :disabled="!enabled">
                <span class="material-symbols-outlined">send</span>
                执行运动
              </button>
              <button class="btn" @click="setHome" :disabled="!enabled">
                <span class="material-symbols-outlined">home</span>
                设为原点
              </button>
            </div>
          </div>
        </div>

        <!-- 速度控制 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">speed</span>
            速度控制
          </div>
          <div class="card-body">
            <div class="speed-bar-wrap">
              <input type="range" v-model.number="speedPercent" min="0" max="100" class="speed-slider" />
              <div class="speed-display">
                <span class="speed-val">{{ speedPercent }}</span>
                <span class="speed-unit">%</span>
              </div>
            </div>
            <div class="freq-display">
              脉冲频率: <span class="freq-val">{{ pulseFrequency.toFixed(0) }}</span> Hz
            </div>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <!-- 实时状态 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">monitoring</span>
            实时状态
          </div>
          <div class="card-body">
            <div class="status-grid">
              <div class="status-item">
                <span class="status-label">实际位置</span>
                <span class="status-value big">{{ actualPosition }}</span>
                <span class="status-unit">脉冲</span>
              </div>
              <div class="status-item">
                <span class="status-label">实际速度</span>
                <span class="status-value big">{{ actualSpeed.toFixed(0) }}</span>
                <span class="status-unit">rpm</span>
              </div>
            </div>
            <div class="state-indicators">
              <div class="state-ind" :class="{ active: motionState === 'running' }">
                <span class="material-symbols-outlined">play_circle</span>
                运行
              </div>
              <div class="state-ind" :class="{ active: motionState === 'stopped' }">
                <span class="material-symbols-outlined">stop_circle</span>
                停止
              </div>
              <div class="state-ind" :class="{ active: motionState === 'homedone' }">
                <span class="material-symbols-outlined">check_circle</span>
                到位
              </div>
              <div class="state-ind alarm" :class="{ active: motionState === 'alarm' }">
                <span class="material-symbols-outlined">error</span>
                报警
              </div>
            </div>
          </div>
        </div>

        <!-- 限位开关 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">toggle_on</span>
            限位开关状态
          </div>
          <div class="card-body">
            <div class="limit-grid">
              <div class="limit-item" :class="{ triggered: limitState.positive }">
                <span class="material-symbols-outlined">arrow_upward</span>
                正限位
                <span class="limit-led" :class="{ on: limitState.positive }"></span>
              </div>
              <div class="limit-item" :class="{ triggered: limitState.negative }">
                <span class="material-symbols-outlined">arrow_downward</span>
                负限位
                <span class="limit-led" :class="{ on: limitState.negative }"></span>
              </div>
              <div class="limit-item" :class="{ triggered: limitState.origin }">
                <span class="material-symbols-outlined">home</span>
                原点
                <span class="limit-led" :class="{ on: limitState.origin }"></span>
              </div>
            </div>
            <div style="margin-top:8px;">
              <button class="btn" @click="homeReturn" :disabled="!enabled">
                <span class="material-symbols-outlined">flight_takeoff</span>
                手动回原点
              </button>
            </div>
          </div>
        </div>

        <!-- IO 状态 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">cable</span>
            IO 状态监控
          </div>
          <div class="card-body">
            <div class="io-grid">
              <div v-for="(io, idx) in ioStates" :key="idx" class="io-item" :class="{ on: io.state }">
                <span class="io-name">{{ io.name }}</span>
                <span class="io-led" :class="{ on: io.state }"></span>
              </div>
            </div>
          </div>
        </div>

        <!-- 脉冲信息 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">graphic_eq</span>
            脉冲信息
          </div>
          <div class="card-body">
            <div class="status-grid">
              <div class="status-item">
                <span class="status-label">脉冲输出频率</span>
                <span class="status-value">{{ pulseFrequency.toFixed(0) }} Hz</span>
              </div>
              <div class="status-item">
                <span class="status-label">已发脉冲数</span>
                <span class="status-value">{{ emittedPulses }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>使能: {{ enabled ? 'ON' : 'OFF' }}</span>
      <span>模式: {{ motionMode === 'point' ? '点位' : '连续' }}</span>
      <span>位置: {{ actualPosition }} pulses</span>
      <span>速度: {{ actualSpeed.toFixed(0) }} rpm</span>
      <span>状态: {{ stateText }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'

const enabled = ref(false)

const motorParams = reactive({
  stepAngle: 1.8,
  microstep: 16,
  maxSpeed: 600,
  acceleration: 5000,
})

const motionMode = ref('point')
const targetPosition = ref(10000)
const speedPercent = ref(50)

const actualPosition = ref(0)
const actualSpeed = ref(0)
const motionState = ref<'idle' | 'running' | 'stopped' | 'homedone' | 'alarm'>('stopped')
const emittedPulses = ref(0)

const limitState = reactive({
  positive: false,
  negative: false,
  origin: true,
})

const ioStates = ref([
  { name: 'DI0-使能', state: true },
  { name: 'DI1-正限位', state: false },
  { name: 'DI2-负限位', state: false },
  { name: 'DI3-原点', state: true },
  { name: 'DO0-脉冲', state: false },
  { name: 'DO1-方向', state: false },
  { name: 'DO2-报警清除', state: false },
  { name: 'DO3-使能输出', state: false },
])

const pulsePerRev = computed(() => (360 / motorParams.stepAngle) / motorParams.microstep)
const pulseFrequency = computed(() => {
  const rpm = motorParams.maxSpeed * speedPercent.value / 100
  return (rpm / 60) * pulsePerRev.value
})

const stateText = computed(() => {
  const map: Record<string, string> = {
    idle: '空闲', running: '运行中', stopped: '已停止', homedone: '回原完成', alarm: '报警'
  }
  return map[motionState.value] || motionState.value
})

let jogDirection = 0
let jogInterval: number | null = null
let simInterval: number | null = null
let targetPos = 0

function toggleEnable() {
  enabled.value = !enabled.value
  if (enabled.value) motionState.value = 'stopped'
  else {
    motionState.value = 'idle'
    actualSpeed.value = 0
  }
}

function emergencyStop() {
  targetPos = actualPosition.value
  actualSpeed.value = 0
  motionState.value = 'stopped'
}

function stopMotion() {
  targetPos = actualPosition.value
  actualSpeed.value = 0
  motionState.value = 'stopped'
}

function startJog(dir: number) {
  if (!enabled.value) return
  jogDirection = dir
  motionState.value = 'running'
  const speed = motorParams.maxSpeed * speedPercent.value / 100
  actualSpeed.value = speed * dir
  targetPos = actualPosition.value + dir * 100000
  ioStates.value[4].state = true
  ioStates.value[5].state = dir > 0
}

function stopJog() {
  if (jogDirection === 0) return
  jogDirection = 0
  targetPos = actualPosition.value
  actualSpeed.value = 0
  motionState.value = 'stopped'
  ioStates.value[4].state = false
}

function moveToTarget() {
  if (!enabled.value) return
  targetPos = targetPosition.value
  motionState.value = 'running'
  ioStates.value[4].state = true
}

function setHome() {
  actualPosition.value = 0
  targetPos = 0
  emittedPulses.value = 0
  limitState.origin = true
  motionState.value = 'homedone'
}

function homeReturn() {
  if (!enabled.value) return
  targetPos = 0
  motionState.value = 'running'
  actualSpeed.value = -motorParams.maxSpeed * 0.3
  ioStates.value[4].state = true
  ioStates.value[5].state = false
}

function simulationTick() {
  if (!enabled.value) return
  const diff = targetPos - actualPosition.value
  if (Math.abs(diff) < 1) {
    actualSpeed.value = 0
    ioStates.value[4].state = false
    if (motionState.value === 'running') motionState.value = 'homedone'
    return
  }

  const maxVel = motorParams.maxSpeed * speedPercent.value / 100
  const dir = diff > 0 ? 1 : -1
  const accel = motorParams.acceleration

  if (Math.abs(actualSpeed.value) < maxVel) {
    actualSpeed.value += dir * accel * 0.01
    if (Math.abs(actualSpeed.value) > maxVel) actualSpeed.value = dir * maxVel
  } else {
    actualSpeed.value = dir * maxVel
  }

  const pulsesPerTick = (actualSpeed.value / 60) * pulsePerRev.value * 0.01
  actualPosition.value += Math.round(pulsesPerTick)
  emittedPulses.value += Math.abs(Math.round(pulsesPerTick))

  if (Math.abs(diff) < Math.abs(pulsesPerTick) * 2) {
    actualPosition.value = targetPos
    actualSpeed.value = 0
    motionState.value = 'homedone'
    ioStates.value[4].state = false
  }

  // 模拟限位触发
  limitState.positive = actualPosition.value > 50000
  limitState.negative = actualPosition.value < -50000
  ioStates.value[1].state = limitState.positive
  ioStates.value[2].state = limitState.negative
}

onMounted(() => {
  simInterval = window.setInterval(simulationTick, 10)
})

onUnmounted(() => {
  if (simInterval !== null) clearInterval(simInterval)
  if (jogInterval !== null) clearInterval(jogInterval)
})
</script>

<style scoped>
.stepper-debug {
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

.main-content {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.left-panel {
  width: 380px;
  overflow-y: auto;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  border-right: 1px solid var(--outline-variant);
}

.right-panel {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 8px;
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
  width: 110px;
  flex-shrink: 0;
  font-size: 11px;
  color: var(--on-surface-variant);
}

.computed-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  color: var(--primary);
  padding: 4px 8px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
}

.motion-btns {
  display: flex;
  justify-content: center;
  gap: 12px;
}

.motion-btn {
  padding: 10px 20px !important;
  font-size: 13px !important;
  font-weight: 600;
}

.motion-btn.fwd { border-color: #2e7d32; }
.motion-btn.rev { border-color: #1565c0; }

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

.speed-bar-wrap {
  display: flex;
  align-items: center;
  gap: 12px;
}

.speed-slider {
  flex: 1;
  height: 8px;
  accent-color: var(--primary);
}

.speed-display {
  display: flex;
  align-items: baseline;
  gap: 2px;
}

.speed-val {
  font-size: 24px;
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
  color: var(--primary);
}

.speed-unit {
  font-size: 12px;
  color: var(--on-surface-variant);
}

.freq-display {
  margin-top: 8px;
  text-align: center;
  font-size: 11px;
  color: var(--on-surface-variant);
}

.freq-val {
  font-family: 'JetBrains Mono', monospace;
  color: var(--on-surface);
  font-weight: 600;
}

.status-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.status-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  padding: 8px;
  background: rgba(0,0,0,0.2);
  border-radius: 6px;
}

.status-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.status-value {
  font-family: 'JetBrains Mono', monospace;
  font-weight: 600;
}

.status-value.big {
  font-size: 20px;
}

.status-unit {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.state-indicators {
  display: flex;
  gap: 8px;
  margin-top: 10px;
  justify-content: center;
}

.state-ind {
  display: flex;
  align-items: center;
  gap: 3px;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 11px;
  background: rgba(0,0,0,0.15);
  opacity: 0.5;
  transition: all 0.2s;
}

.state-ind.active {
  opacity: 1;
  background: rgba(46, 125, 50, 0.3);
  color: #66bb6a;
}

.state-ind.alarm.active {
  background: rgba(198, 40, 40, 0.3);
  color: #ef5350;
}

.limit-grid {
  display: flex;
  gap: 10px;
}

.limit-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 10px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
  font-size: 11px;
  flex: 1;
}

.limit-item.triggered {
  background: rgba(198, 40, 40, 0.3);
  color: #ef5350;
}

.limit-led, .io-led {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #555;
  margin-left: auto;
  transition: background 0.2s;
}

.limit-led.on, .io-led.on {
  background: #66bb6a;
  box-shadow: 0 0 4px #66bb6a;
}

.io-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.io-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 8px;
  background: rgba(0,0,0,0.1);
  border-radius: 3px;
  font-size: 11px;
}

.io-item.on {
  background: rgba(46, 125, 50, 0.15);
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