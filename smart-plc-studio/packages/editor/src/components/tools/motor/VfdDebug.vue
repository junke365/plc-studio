<template>
  <div class="vfd-debug">
    <div class="toolbar">
      <span class="toolbar-title">
        <span class="material-symbols-outlined">electric_meter</span>
        变频器调试
      </span>
      <div class="toolbar-actions">
        <button class="btn btn-success" @click="runVfd" :disabled="vfdRunning">
          <span class="material-symbols-outlined">play_arrow</span>
          运行
        </button>
        <button class="btn btn-danger" @click="stopVfd" :disabled="!vfdRunning">
          <span class="material-symbols-outlined">stop</span>
          停止
        </button>
        <button class="btn" @click="toggleDirection">
          <span class="material-symbols-outlined">swap_vert</span>
          {{ direction === 1 ? '正转' : '反转' }}
        </button>
        <button class="btn" @click="readFault">
          <span class="material-symbols-outlined">error_outline</span>
          读取故障
        </button>
        <button class="btn" @click="clearFault">
          <span class="material-symbols-outlined">delete_forever</span>
          清除故障
        </button>
      </div>
    </div>

    <div class="main-content">
      <div class="left-panel">
        <!-- 通信设置 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">settings_ethernet</span>
            通信设置 (Modbus RTU)
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>站地址</label>
              <input type="number" v-model.number="comm.address" class="input-sm" min="1" max="247" />
            </div>
            <div class="form-row">
              <label>波特率</label>
              <select v-model.number="comm.baudRate" class="select-sm">
                <option :value="2400">2400</option>
                <option :value="4800">4800</option>
                <option :value="9600">9600</option>
                <option :value="19200">19200</option>
                <option :value="38400">38400</option>
                <option :value="57600">57600</option>
                <option :value="115200">115200</option>
              </select>
            </div>
            <div class="form-row">
              <label>数据位/校验/停止位</label>
              <span class="text-val">8 / 无 / 1</span>
            </div>
            <div class="form-row">
              <span class="comm-status" :class="{ connected: comm.connected }">
                <span class="material-symbols-outlined">{{ comm.connected ? 'link' : 'link_off' }}</span>
                {{ comm.connected ? '已连接' : '未连接' }}
              </span>
              <button class="btn" @click="comm.connected = !comm.connected">
                {{ comm.connected ? '断开' : '连接' }}
              </button>
            </div>
          </div>
        </div>

        <!-- 基本控制 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">sports_motorsports</span>
            频率设定
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>目标频率 (Hz)</label>
              <input type="range" v-model.number="targetFrequency" min="0" max="500" step="0.1" class="param-slider" />
              <input type="number" v-model.number="targetFrequency" class="input-sm" min="0" max="500" step="0.1" style="width:80px" />
            </div>
            <div class="freq-preset-btns">
              <button class="btn freq-preset" v-for="f in [10, 25, 50, 100, 200, 500]" :key="f" @click="targetFrequency = f">{{ f }}Hz</button>
            </div>
          </div>
        </div>

        <!-- 加减速时间 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">timer</span>
            加减速时间
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>加速时间 (s)</label>
              <input type="number" v-model.number="accTime" class="input-sm" min="0.1" max="600" step="0.1" />
            </div>
            <div class="form-row">
              <label>减速时间 (s)</label>
              <input type="number" v-model.number="decTime" class="input-sm" min="0.1" max="600" step="0.1" />
            </div>
          </div>
        </div>

        <!-- 多段速 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">queue</span>
            多段速设置 (8段)
          </div>
          <div class="card-body">
            <div class="multi-speed-grid">
              <div v-for="(speed, idx) in multiSpeeds" :key="idx" class="form-row">
                <label>段{{ idx + 1 }}</label>
                <input type="number" v-model.number="multiSpeeds[idx]" class="input-sm" min="0" max="500" step="0.1" />
                <span style="font-size:10px;color:var(--on-surface-variant)">Hz</span>
              </div>
            </div>
            <button class="btn btn-primary" style="width:100%;margin-top:6px;justify-content:center;" @click="applyMultiSpeed">
              <span class="material-symbols-outlined">send</span>
              应用多段速
            </button>
          </div>
        </div>

        <!-- 参数读写 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">edit_note</span>
            参数读写
          </div>
          <div class="card-body">
            <div class="form-row">
              <label>参数号</label>
              <input type="number" v-model.number="paramRead.num" class="input-sm" min="0" max="9999" />
            </div>
            <div class="form-row">
              <label>参数值</label>
              <input type="number" v-model.number="paramRead.val" class="input-sm" />
            </div>
            <div class="form-row">
              <button class="btn" @click="readParam">
                <span class="material-symbols-outlined">download</span>
                读取
              </button>
              <button class="btn btn-primary" @click="writeParam">
                <span class="material-symbols-outlined">upload</span>
                写入
              </button>
            </div>
            <div v-if="paramRead.result" class="param-result">{{ paramRead.result }}</div>
          </div>
        </div>
      </div>

      <div class="right-panel">
        <!-- 实时显示 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">monitoring</span>
            实时参数
          </div>
          <div class="card-body">
            <div class="realtime-grid">
              <div class="rt-item">
                <div class="rt-icon" style="color:#42a5f5;">
                  <span class="material-symbols-outlined">speed</span>
                </div>
                <div class="rt-info">
                  <span class="rt-label">实际频率</span>
                  <span class="rt-value">{{ actualFrequency.toFixed(1) }} Hz</span>
                </div>
              </div>
              <div class="rt-item">
                <div class="rt-icon" style="color:#ffa726;">
                  <span class="material-symbols-outlined">bolt</span>
                </div>
                <div class="rt-info">
                  <span class="rt-label">输出电流</span>
                  <span class="rt-value">{{ actualCurrent.toFixed(2) }} A</span>
                </div>
              </div>
              <div class="rt-item">
                <div class="rt-icon" style="color:#66bb6a;">
                  <span class="material-symbols-outlined">electric_bolt</span>
                </div>
                <div class="rt-info">
                  <span class="rt-label">输出电压</span>
                  <span class="rt-value">{{ actualVoltage.toFixed(1) }} V</span>
                </div>
              </div>
              <div class="rt-item">
                <div class="rt-icon" style="color:#ef5350;">
                  <span class="material-symbols-outlined">power</span>
                </div>
                <div class="rt-info">
                  <span class="rt-label">输出功率</span>
                  <span class="rt-value">{{ actualPower.toFixed(1) }} kW</span>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- V/F 曲线 -->
        <div class="card vf-card">
          <div class="card-header">
            <span class="material-symbols-outlined">show_chart</span>
            V/F 曲线
          </div>
          <div class="card-body chart-body">
            <canvas ref="vfCanvas"></canvas>
          </div>
        </div>

        <!-- 故障码 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">error</span>
            故障记录
          </div>
          <div class="card-body" style="max-height:120px;overflow-y:auto;">
            <div v-if="faultCodes.length === 0" class="empty-hint">无故障</div>
            <div v-for="(fault, idx) in faultCodes" :key="idx" class="fault-item">
              <span class="fault-code">{{ fault.code }}</span>
              <span class="fault-desc">{{ fault.desc }}</span>
              <span class="fault-time">{{ fault.time }}</span>
            </div>
          </div>
        </div>

        <!-- 运行统计 -->
        <div class="card">
          <div class="card-header">
            <span class="material-symbols-outlined">query_stats</span>
            运行统计
          </div>
          <div class="card-body">
            <div class="stats-grid">
              <div class="stat-item">
                <span class="stat-label">运行时间</span>
                <span class="stat-value">{{ formatRuntime(runHours) }}</span>
              </div>
              <div class="stat-item">
                <span class="stat-label">启停次数</span>
                <span class="stat-value">{{ startStopCount }}</span>
              </div>
              <div class="stat-item">
                <span class="stat-label">运行方向</span>
                <span class="stat-value">{{ direction === 1 ? '正转' : '反转' }}</span>
              </div>
              <div class="stat-item">
                <span class="stat-label">运行状态</span>
                <span class="stat-value" :class="{ 'status-on': vfdRunning }">{{ vfdRunning ? '运行' : '停止' }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>站地址: {{ comm.address }}</span>
      <span>波特率: {{ comm.baudRate }}</span>
      <span>频率: {{ actualFrequency.toFixed(1) }}Hz</span>
      <span>电流: {{ actualCurrent.toFixed(2) }}A</span>
      <span>{{ comm.connected ? '已连接' : '未连接' }} | {{ vfdRunning ? '运行中' : '已停止' }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, nextTick } from 'vue'

const comm = reactive({
  address: 1,
  baudRate: 9600,
  connected: false,
})

const vfdRunning = ref(false)
const direction = ref(1)
const targetFrequency = ref(50)
const actualFrequency = ref(0)
const actualCurrent = ref(0)
const actualVoltage = ref(0)
const actualPower = ref(0)

const accTime = ref(10)
const decTime = ref(10)

const multiSpeeds = ref([10, 25, 50, 100, 150, 200, 300, 500])

const paramRead = reactive({
  num: 0,
  val: 0,
  result: '',
})

const faultCodes = ref<Array<{ code: string; desc: string; time: string }>>([])

const runHours = ref(0)
const startStopCount = ref(0)

const vfCanvas = ref<HTMLCanvasElement | null>(null)

function runVfd() {
  vfdRunning.value = true
  startStopCount.value++
}

function stopVfd() {
  vfdRunning.value = false
}

function toggleDirection() {
  direction.value *= -1
}

function readFault() {
  if (faultCodes.value.length === 0) {
    const now = new Date()
    const time = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}`
    const faults = [
      { code: 'OC1', desc: '过电流' },
      { code: 'OH1', desc: '散热器过热' },
      { code: 'OV1', desc: '过电压' },
      { code: 'LU', desc: '欠电压' },
    ]
    const f = faults[Math.floor(Math.random() * faults.length)]
    faultCodes.value.unshift({ code: f.code, desc: f.desc, time })
  }
}

function clearFault() {
  faultCodes.value = []
}

function applyMultiSpeed() {
  // 模拟应用
}

function readParam() {
  paramRead.result = `参数${paramRead.num} = ${Math.floor(Math.random() * 1000)}`
}

function writeParam() {
  paramRead.result = `参数${paramRead.num} 已写入值: ${paramRead.val}`
}

function formatRuntime(h: number) {
  const hrs = Math.floor(h)
  const mins = Math.floor((h - hrs) * 60)
  return `${hrs}h ${mins}m`
}

function drawVFcurve() {
  const canvas = vfCanvas.value
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
  const pad = { top: 15, right: 15, bottom: 30, left: 45 }
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
  for (let i = 0; i <= 5; i++) {
    const x = pad.left + (cw / 5) * i
    ctx.beginPath()
    ctx.moveTo(x, pad.top)
    ctx.lineTo(x, pad.top + ch)
    ctx.stroke()
  }

  // V/F 曲线 (线性到基频50Hz，然后恒压)
  const baseFreq = 50
  const baseVoltage = 380

  ctx.beginPath()
  ctx.strokeStyle = '#42a5f5'
  ctx.lineWidth = 2
  for (let i = 0; i <= 500; i++) {
    const x = pad.left + (i / 500) * cw
    let v: number
    if (i <= baseFreq) {
      v = (i / baseFreq) * baseVoltage
    } else {
      v = baseVoltage
    }
    const y = pad.top + ch - (v / 400) * ch
    if (i === 0) ctx.moveTo(x, y)
    else ctx.lineTo(x, y)
  }
  ctx.stroke()

  // 当前工作点
  if (vfdRunning.value) {
    const x = pad.left + (actualFrequency.value / 500) * cw
    let v = actualFrequency.value <= baseFreq ? (actualFrequency.value / baseFreq) * baseVoltage : baseVoltage
    const y = pad.top + ch - (v / 400) * ch
    ctx.fillStyle = '#ef5350'
    ctx.beginPath()
    ctx.arc(x, y, 5, 0, Math.PI * 2)
    ctx.fill()
  }

  // 标注
  ctx.fillStyle = 'rgba(255,255,255,0.5)'
  ctx.font = '10px JetBrains Mono, monospace'
  ctx.textAlign = 'center'
  for (let i = 0; i <= 5; i++) {
    ctx.fillText(`${(500 / 5) * i}Hz`, pad.left + (cw / 5) * i, h - 8)
  }
  ctx.textAlign = 'right'
  for (let i = 0; i <= 5; i++) {
    const v = 400 - (400 / 5) * i
    ctx.fillText(`${v.toFixed(0)}V`, pad.left - 4, pad.top + (ch / 5) * i + 3)
  }
}

let simInterval: number | null = null
let simTime = 0

function simulationTick() {
  simTime += 0.01

  if (vfdRunning.value) {
    const accel = (targetFrequency.value - actualFrequency.value) / accTime.value * 0.01
    actualFrequency.value += accel
    actualFrequency.value = Math.max(0, Math.min(500, actualFrequency.value))

    actualVoltage.value = actualFrequency.value <= 50 ? (actualFrequency.value / 50) * 380 : 380
    actualCurrent.value = 2 + actualFrequency.value * 0.05 + Math.sin(simTime * 3) * 0.3
    actualPower.value = actualVoltage.value * actualCurrent.value * 1.732 * 0.85 / 1000
    runHours.value += 0.01 / 3600
  } else {
    actualFrequency.value *= 0.95
    if (actualFrequency.value < 0.1) actualFrequency.value = 0
    actualCurrent.value *= 0.95
    actualVoltage.value *= 0.95
    actualPower.value *= 0.95
  }

  drawVFcurve()
}

onMounted(() => {
  nextTick(() => {
    simInterval = window.setInterval(simulationTick, 10)
  })
})

onUnmounted(() => {
  if (simInterval !== null) clearInterval(simInterval)
})
</script>

<style scoped>
.vfd-debug {
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
  width: 120px;
  flex-shrink: 0;
  font-size: 11px;
  color: var(--on-surface-variant);
}

.text-val {
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
}

.comm-status {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  color: #ef5350;
}

.comm-status.connected {
  color: #66bb6a;
}

.param-slider {
  flex: 1;
  height: 4px;
  accent-color: var(--primary);
}

.freq-preset-btns {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.freq-preset {
  flex: 1;
  min-width: 50px;
  justify-content: center;
  font-size: 11px !important;
  padding: 3px 6px !important;
}

.multi-speed-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 4px;
}

.param-result {
  margin-top: 4px;
  padding: 4px 8px;
  background: rgba(0,0,0,0.2);
  border-radius: 3px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  color: var(--primary);
}

.realtime-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.rt-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  background: rgba(0,0,0,0.2);
  border-radius: 6px;
}

.rt-icon {
  font-size: 24px;
}

.rt-info {
  display: flex;
  flex-direction: column;
}

.rt-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.rt-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 16px;
  font-weight: 600;
}

.vf-card {
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

.fault-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 8px;
  margin-bottom: 4px;
  background: rgba(198, 40, 40, 0.15);
  border-radius: 3px;
  font-size: 11px;
}

.fault-code {
  font-family: 'JetBrains Mono', monospace;
  font-weight: 700;
  color: #ef5350;
}

.fault-desc {
  flex: 1;
}

.fault-time {
  color: var(--on-surface-variant);
  font-size: 10px;
}

.empty-hint {
  text-align: center;
  padding: 8px;
  color: var(--on-surface-variant);
  font-size: 11px;
}

.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.stat-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 4px 8px;
  background: rgba(0,0,0,0.1);
  border-radius: 3px;
}

.stat-label {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.stat-value {
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  font-weight: 600;
}

.stat-value.status-on {
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