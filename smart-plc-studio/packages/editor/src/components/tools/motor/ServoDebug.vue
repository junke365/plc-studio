<template>
  <div class="servo-debug">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">precision_manufacturing</span>
        伺服电机调试
      </span>
      <div class="toolbar-actions">
        <button class="btn" :class="{ 'btn-success': servoEnabled }" @click="toggleServo">
          <span class="material-symbols-outlined">{{ servoEnabled ? 'power_settings_new' : 'power_off' }}</span>
          {{ servoEnabled ? '使能' : '去使能' }}
        </button>
        <button class="btn btn-danger" @click="clearAlarm">
          <span class="material-symbols-outlined">alarm_off</span>
          清除报警
        </button>
        <button class="btn" @click="readAllParams">
          <span class="material-symbols-outlined">refresh</span>
          读取参数
        </button>
      </div>
    </div>

    <div class="main-content">
      <div class="left-panel">
        <!-- 控制模式 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">tune</span>
            控制模式
          </div>
          <div class="card-body">
            <div class="mode-btns">
              <button class="btn" :class="{ 'btn-primary': controlMode === 'position' }" @click="controlMode = 'position'">
                <span class="material-symbols-outlined">my_location</span>
                位置模式
              </button>
              <button class="btn" :class="{ 'btn-primary': controlMode === 'velocity' }" @click="controlMode = 'velocity'">
                <span class="material-symbols-outlined">speed</span>
                速度模式
              </button>
              <button class="btn" :class="{ 'btn-primary': controlMode === 'torque' }" @click="controlMode = 'torque'">
                <span class="material-symbols-outlined">fitness_center</span>
                扭矩模式
              </button>
            </div>
          </div>
        </div>

        <!-- 位置模式 -->
        <div class="card" v-if="controlMode === 'position'">
          <div class="card-header">
            <span class="material-symbols-outlined">my_location</span>
            位置控制
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>目标位置 (deg)</label>
              <input type="number" v-model.number="posTarget" class="input-sm" />
            </div>
            <div class="param-display">
              <div class="pd-item">
                <span class="pd-label">实际位置</span>
                <span class="pd-value">{{ posActual.toFixed(2) }}°</span>
              </div>
              <div class="pd-item">
                <span class="pd-label">位置误差</span>
                <span class="pd-value" :class="{ error: Math.abs(posError) > 1 }">{{ posError.toFixed(3) }}°</span>
              </div>
            </div>
            <button class="btn btn-primary" style="margin-top:8px;width:100%;" @click="goToPosition" :disabled="!servoEnabled">
              <span class="material-symbols-outlined">send</span>
              执行定位
            </button>
          </div>
        </div>

        <!-- 速度模式 -->
        <div class="card" v-if="controlMode === 'velocity'">
          <div class="card-header">
            <span class="material-symbols-outlined">speed</span>
            速度控制
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>目标速度 (rpm)</label>
              <input type="number" v-model.number="velTarget" class="input-sm" />
            </div>
            <div class="param-display">
              <div class="pd-item">
                <span class="pd-label">实际速度</span>
                <span class="pd-value">{{ velActual.toFixed(1) }} rpm</span>
              </div>
              <div class="pd-item">
                <span class="pd-label">速度误差</span>
                <span class="pd-value" :class="{ error: Math.abs(velError) > 5 }">{{ velError.toFixed(1) }} rpm</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 扭矩模式 -->
        <div class="card" v-if="controlMode === 'torque'">
          <div class="card-header">
            <span class="material-symbols-outlined">fitness_center</span>
            扭矩控制
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>目标扭矩 (%)</label>
              <input type="range" v-model.number="torTarget" min="-100" max="100" class="param-slider" />
              <span class="slider-val">{{ torTarget }}%</span>
            </div>
            <div class="form-row">
              <label>扭矩限制 (%)</label>
              <input type="number" v-model.number="torLimit" class="input-sm" min="0" max="100" />
            </div>
            <div class="param-display">
              <div class="pd-item">
                <span class="pd-label">实际扭矩</span>
                <span class="pd-value">{{ torActual.toFixed(1) }}%</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 增益调整 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">equalizer</span>
            增益调整
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>位置环增益</label>
              <input type="range" v-model.number="gains.positionKp" min="0" max="500" class="param-slider" />
              <span class="slider-val">{{ gains.positionKp }}</span>
            </div>
            <div class="form-row">
              <label>速度环增益</label>
              <input type="range" v-model.number="gains.velocityKp" min="0" max="200" class="param-slider" />
              <span class="slider-val">{{ gains.velocityKp }}</span>
            </div>
            <div class="form-row">
              <label>积分时间常数 (ms)</label>
              <input type="range" v-model.number="gains.integralTime" min="0" max="500" class="param-slider" />
              <span class="slider-val">{{ gains.integralTime }}</span>
            </div>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <!-- 报警状态 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">warning</span>
            报警状态
          </div>
          <div class="card-body">
            <div class="alarm-grid">
              <div class="alarm-item" :class="{ active: alarms.overload }">
                <span class="material-symbols-outlined">fitness_center</span>
                过载
              </div>
              <div class="alarm-item" :class="{ active: alarms.overheat }">
                <span class="material-symbols-outlined">thermostat</span>
                过热
              </div>
              <div class="alarm-item" :class="{ active: alarms.overvoltage }">
                <span class="material-symbols-outlined">bolt</span>
                过压
              </div>
              <div class="alarm-item" :class="{ active: alarms.undervoltage }">
                <span class="material-symbols-outlined">battery_alert</span>
                欠压
              </div>
              <div class="alarm-item" :class="{ active: alarms.encoder }">
                <span class="material-symbols-outlined">ensors</span>
                编码器
              </div>
              <div class="alarm-item" :class="{ active: alarms.position }">
                <span class="material-symbols-outlined">my_location</span>
                位置偏差
              </div>
            </div>
          </div>
        </div>

        <!-- JOG 模式 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">gamepad</span>
            JOG 点动
          </div>
          <div class="card-body">
            <div class="jog-btns">
              <button class="btn jog-btn rev" @mousedown="jogBackward" @mouseup="jogStop" @mouseleave="jogStop">
                <span class="material-symbols-outlined">replay_30</span>
                点动后退
              </button>
              <button class="btn jog-btn fwd" @mousedown="jogForward" @mouseup="jogStop" @mouseleave="jogStop">
                <span class="material-symbols-outlined">forward_30</span>
                点动前进
              </button>
            </div>
            <div class="form-row" style="margin-top:8px;">
              <label>JOG 速度 (rpm)</label>
              <input type="number" v-model.number="jogSpeed" class="input-sm" min="1" max="3000" />
            </div>
          </div>
        </div>

        <!-- 编码器信息 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">route</span>
            编码器信息
          </div>
          <div class="card-body">
            <div class="encoder-grid">
              <div class="enc-item">
                <span class="enc-label">当前位置</span>
                <span class="enc-value">{{ encoderPos }}</span>
                <span class="enc-unit">pulse</span>
              </div>
              <div class="enc-item">
                <span class="enc-label">当前速度</span>
                <span class="enc-value">{{ encoderVel.toFixed(0) }}</span>
                <span class="enc-unit">rpm</span>
              </div>
              <div class="enc-item">
                <span class="enc-label">编码器类型</span>
                <span class="enc-value">增量式</span>
                <span class="enc-unit">17bit</span>
              </div>
              <div class="enc-item">
                <span class="enc-label">Z 脉冲</span>
                <span class="enc-value">{{ zPulseCount }}</span>
                <span class="enc-unit">次</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 运动状态 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">monitoring</span>
            运行状态
          </div>
          <div class="card-body">
            <div class="state-grid">
              <div class="state-row">
                <span class="state-label">伺服状态</span>
                <span class="state-val" :class="{ on: servoEnabled }">{{ servoEnabled ? '使能' : '去使能' }}</span>
              </div>
              <div class="state-row">
                <span class="state-label">控制模式</span>
                <span class="state-val">{{ controlModeText }}</span>
              </div>
              <div class="state-row">
                <span class="state-label">运行状态</span>
                <span class="state-val">{{ runStatus }}</span>
              </div>
              <div class="state-row">
                <span class="state-label">母线电压</span>
                <span class="state-val">{{ busVoltage.toFixed(1) }}V</span>
              </div>
              <div class="state-row">
                <span class="state-label">驱动温度</span>
                <span class="state-val">{{ driverTemp.toFixed(0) }}°C</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>模式: {{ controlModeText }}</span>
      <span>使能: {{ servoEnabled ? 'ON' : 'OFF' }}</span>
      <span>位置: {{ posActual.toFixed(2) }}°</span>
      <span>速度: {{ velActual.toFixed(1) }} rpm</span>
      <span>扭矩: {{ torActual.toFixed(1) }}%</span>
      <span>{{ hasAlarm ? '⚠ 报警' : '正常' }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'

const servoEnabled = ref(false)
const controlMode = ref<'position' | 'velocity' | 'torque'>('position')

const posTarget = ref(0)
const posActual = ref(0)
const posError = computed(() => posTarget.value - posActual.value)

const velTarget = ref(0)
const velActual = ref(0)
const velError = computed(() => velTarget.value - velActual.value)

const torTarget = ref(0)
const torActual = ref(0)
const torLimit = ref(80)

const jogSpeed = ref(100)

const gains = reactive({
  positionKp: 120,
  velocityKp: 50,
  integralTime: 30,
})

const alarms = reactive({
  overload: false,
  overheat: false,
  overvoltage: false,
  undervoltage: false,
  encoder: false,
  position: false,
})

const encoderPos = ref(0)
const encoderVel = ref(0)
const zPulseCount = ref(0)
const busVoltage = ref(48.2)
const driverTemp = ref(35)

const hasAlarm = computed(() => Object.values(alarms).some(v => v))
const controlModeText = computed(() => {
  const m: Record<string, string> = { position: '位置模式', velocity: '速度模式', torque: '扭矩模式' }
  return m[controlMode.value]
})
const runStatus = computed(() => {
  if (hasAlarm.value) return '报警'
  if (!servoEnabled.value) return '未使能'
  if (Math.abs(posError.value) < 0.05 && controlMode.value === 'position') return '到位'
  return '运行'
})

function toggleServo() {
  servoEnabled.value = !servoEnabled.value
  if (!servoEnabled.value) {
    velActual.value = 0
    torActual.value = 0
  }
}

function clearAlarm() {
  Object.keys(alarms).forEach(k => (alarms as any)[k] = false)
}

function readAllParams() {
  // 模拟读取参数
}

function goToPosition() {
  if (!servoEnabled.value) return
}

function jogForward() {
  if (!servoEnabled.value) return
  velActual.value = jogSpeed.value
}

function jogBackward() {
  if (!servoEnabled.value) return
  velActual.value = -jogSpeed.value
}

function jogStop() {
  velActual.value = 0
}

let simInterval: number | null = null

function simulationTick() {
  if (!servoEnabled.value) {
    posActual.value *= 0.95
    velActual.value *= 0.9
    torActual.value *= 0.9
    encoderPos.value = Math.round(posActual.value * 1000)
    encoderVel.value = velActual.value
    return
  }

  if (controlMode.value === 'position') {
    const err = posTarget.value - posActual.value
    const kp = gains.positionKp / 100
    const ki = 1 / (gains.integralTime + 1)
    velActual.value += err * kp * 2
    velActual.value = Math.max(-3000, Math.min(3000, velActual.value))
    velActual.value *= 0.98
    posActual.value += velActual.value * 0.001
    torActual.value = Math.min(torLimit.value, Math.abs(err) * 5)
  } else if (controlMode.value === 'velocity') {
    const err = velTarget.value - velActual.value
    velActual.value += err * 0.1
    posActual.value += velActual.value * 0.001
    torActual.value = Math.min(torLimit.value, Math.abs(err) * 0.5 + 5)
  } else {
    velActual.value += (torTarget.value * 30 - velActual.value) * 0.05
    velActual.value = Math.max(-3000, Math.min(3000, velActual.value))
    posActual.value += velActual.value * 0.001
    torActual.value = torTarget.value * 0.8 + (Math.random() - 0.5) * 2
  }

  encoderPos.value = Math.round(posActual.value * 1000)
  encoderVel.value = velActual.value
  busVoltage.value = 48 + (Math.random() - 0.5) * 0.5
  driverTemp.value = 35 + Math.abs(torActual.value) * 0.15

  // 模拟偶发报警
  if (driverTemp.value > 70) alarms.overheat = true
}

onMounted(() => {
  simInterval = window.setInterval(simulationTick, 10)
})

onUnmounted(() => {
  if (simInterval !== null) clearInterval(simInterval)
})
</script>

<style scoped>
.servo-debug {
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

.mode-btns {
  display: flex;
  gap: 8px;
}

.mode-btns .btn {
  flex: 1;
  justify-content: center;
  padding: 8px !important;
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

.param-display {
  display: flex;
  gap: 10px;
}

.pd-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  padding: 8px;
  background: rgba(0,0,0,0.2);
  border-radius: 6px;
}

.pd-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.pd-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 16px;
  font-weight: 600;
}

.pd-value.error {
  color: #ef5350;
}

.alarm-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 6px;
}

.alarm-item {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  padding: 8px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
  font-size: 11px;
  opacity: 0.4;
  transition: all 0.2s;
}

.alarm-item.active {
  opacity: 1;
  background: rgba(198, 40, 40, 0.3);
  color: #ef5350;
  animation: blink 1s infinite;
}

@keyframes blink {
  50% { opacity: 0.7; }
}

.jog-btns {
  display: flex;
  gap: 12px;
  justify-content: center;
}

.jog-btn {
  flex: 1;
  padding: 12px !important;
  font-size: 13px !important;
  font-weight: 600;
  justify-content: center;
}

.jog-btn.fwd { border-color: #2e7d32; }
.jog-btn.rev { border-color: #1565c0; }

.encoder-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.enc-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  padding: 8px;
  background: rgba(0,0,0,0.15);
  border-radius: 4px;
}

.enc-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.enc-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 16px;
  font-weight: 600;
}

.enc-unit {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.state-grid {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.state-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 4px 8px;
  background: rgba(0,0,0,0.1);
  border-radius: 3px;
}

.state-label {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.state-val {
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  font-weight: 600;
}

.state-val.on {
  color: #66bb6a;
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