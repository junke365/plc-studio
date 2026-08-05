<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { serialListPorts, serialOpen, serialClose, onSerialStatus, onSerialError, type PortInfo } from '@/serial/serialClient'

interface SerialPortInfo { path: string; manufacturer?: string; serialNumber?: string }

const portList = ref<SerialPortInfo[]>([])
const config = reactive({ port: '', baudRate: 115200, dataBits: 8, stopBits: 1, parity: 'none', flowControl: 'none' })
const advanced = reactive({ bufferSize: 4096, timeout: 1000, rts: false, dtr: false })
const isOpen = ref(false)
const selectedPort = ref('')
const logMessages = ref<string[]>([])

const baudRates = [1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
const dataBitsOptions = [5, 6, 7, 8]
const stopBitsOptions = [1, 1.5, 2]
const parityOptions = ['none', 'odd', 'even', 'mark', 'space']
const flowControlOptions = ['none', 'hardware', 'software']

const onlineCount = computed(() => portList.value.length)
const parityLabels: Record<string, string> = { none: '无', odd: '奇校验', even: '偶校验', mark: '标记校验', space: '空格校验' }
const flowControlLabels: Record<string, string> = { none: '无', hardware: '硬件流控', software: '软件流控' }

let cleanupStatus: (() => void) | null = null
let cleanupError: (() => void) | null = null

function addLog(msg: string) {
  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  logMessages.value.push(`[${time}] ${msg}`)
  if (logMessages.value.length > 200) logMessages.value.shift()
}

async function togglePort() {
  if (!isOpen.value) {
    const result = await serialOpen({ path: config.port, baudRate: config.baudRate, dataBits: config.dataBits as any, stopBits: config.stopBits as any, parity: config.parity as any })
    if (result.success) {
      isOpen.value = true
      addLog(`串口 ${config.port} 已打开 - ${config.baudRate},${config.dataBits},${parityLabels[config.parity]},${config.stopBits}`)
    } else {
      addLog(`打开串口失败: ${result.error}`)
    }
  } else {
    await serialClose(config.port)
    isOpen.value = false
    addLog(`串口 ${config.port} 已关闭`)
  }
}

async function refreshPorts() {
  addLog('刷新串口列表...')
  const ports = await serialListPorts()
  portList.value = ports.map(p => ({ path: p.path, manufacturer: p.manufacturer, serialNumber: p.serialNumber }))
  if (portList.value.length > 0 && !portList.value.some(p => p.path === config.port)) {
    config.port = portList.value[0].path
    selectedPort.value = config.port
  }
  addLog(`发现 ${ports.length} 个串口`)
}

function applyConfig() {
  addLog('配置已应用')
  addLog(`  波特率: ${config.baudRate} 数据位: ${config.dataBits} 停止位: ${config.stopBits}`)
  addLog(`  校验: ${parityLabels[config.parity]} 流控: ${flowControlLabels[config.flowControl]}`)
}

function selectPort(name: string) { selectedPort.value = name; config.port = name; addLog(`选中串口: ${name}`) }
function clearLog() { logMessages.value = [] }

onMounted(() => {
  refreshPorts()
  cleanupStatus = onSerialStatus((d) => {
    if (d.port === config.port) {
      isOpen.value = d.connected
      addLog(`串口 ${d.port} ${d.connected ? '已连接' : '已断开'}`)
    }
  })
  cleanupError = onSerialError((d) => {
    if (d.port === config.port) addLog(`串口错误: ${d.error}`)
  })
})

onUnmounted(() => { if (cleanupStatus) cleanupStatus(); if (cleanupError) cleanupError() })
</script>

<template>
  <div class="uart-config">
    <div class="toolbar">
      <button class="btn" @click="refreshPorts">
        <span class="material-symbols-outlined" style="font-size:16px">refresh</span>
        刷新
      </button>
      <button class="btn" :class="{ 'btn-primary': !isOpen }" @click="togglePort">
        <span class="material-symbols-outlined" style="font-size:16px">{{ isOpen ? 'stop' : 'play_arrow' }}</span>
        {{ isOpen ? '关闭串口' : '打开串口' }}
      </button>
      <button class="btn btn-primary" @click="applyConfig">
        <span class="material-symbols-outlined" style="font-size:16px">save</span>
        应用配置
      </button>
      <div class="toolbar-spacer" />
      <span class="port-count">在线: {{ onlineCount }}/{{ portList.length }}</span>
    </div>

    <div class="main-content">
      <div class="content-left">
        <!-- 串口参数配置 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">settings</span>
            串口参数
          </div>
          <div class="panel-body">
            <div class="form-row">
              <label>串口:</label>
              <select v-model="config.port" class="select-sm">
                <option v-if="portList.length === 0" value="" disabled>未检测到串口</option>
                <option v-for="p in portList" :key="p.path" :value="p.path">
                  {{ p.path }}
                </option>
              </select>
            </div>
            <div class="form-row">
              <label>波特率:</label>
              <select v-model.number="config.baudRate" class="select-sm">
                <option v-for="br in baudRates" :key="br" :value="br">{{ br }}</option>
              </select>
            </div>
            <div class="form-row">
              <label>数据位:</label>
              <select v-model.number="config.dataBits" class="select-sm">
                <option v-for="db in dataBitsOptions" :key="db" :value="db">{{ db }}</option>
              </select>
            </div>
            <div class="form-row">
              <label>停止位:</label>
              <select v-model.number="config.stopBits" class="select-sm">
                <option v-for="sb in stopBitsOptions" :key="sb" :value="sb">{{ sb }}</option>
              </select>
            </div>
            <div class="form-row">
              <label>校验位:</label>
              <select v-model="config.parity" class="select-sm">
                <option v-for="p in parityOptions" :key="p" :value="p">{{ parityLabels[p] }}</option>
              </select>
            </div>
            <div class="form-row">
              <label>流控:</label>
              <select v-model="config.flowControl" class="select-sm">
                <option v-for="fc in flowControlOptions" :key="fc" :value="fc">{{ flowControlLabels[fc] }}</option>
              </select>
            </div>
          </div>
        </div>

        <!-- 高级设置 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">tune</span>
            高级设置
          </div>
          <div class="panel-body">
            <div class="form-row">
              <label>缓冲区:</label>
              <input v-model.number="advanced.bufferSize" type="number" class="input-sm" min="256" max="65536" />
              <span class="unit">字节</span>
            </div>
            <div class="form-row">
              <label>超时:</label>
              <input v-model.number="advanced.timeout" type="number" class="input-sm" min="100" max="10000" step="100" />
              <span class="unit">毫秒</span>
            </div>
            <div class="form-row">
              <label>RTS:</label>
              <label class="checkbox-label">
                <input type="checkbox" v-model="advanced.rts" />
                <span>{{ advanced.rts ? '启用' : '禁用' }}</span>
              </label>
            </div>
            <div class="form-row">
              <label>DTR:</label>
              <label class="checkbox-label">
                <input type="checkbox" v-model="advanced.dtr" />
                <span>{{ advanced.dtr ? '启用' : '禁用' }}</span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div class="content-right">
        <!-- 串口列表 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">cable</span>
            串口列表
          </div>
          <div class="panel-body port-list">
            <div
              v-for="port in portList"
              :key="port.path"
              class="port-item"
              :class="{ selected: selectedPort === port.path }"
              @click="selectPort(port.path)"
            >
              <div class="port-status-dot online" />
              <div class="port-info">
                <div class="port-name">{{ port.path }}</div>
                <div class="port-desc">{{ port.manufacturer || '未知设备' }}</div>
              </div>
              <span class="port-status-text online">
                在线
              </span>
            </div>
          </div>
        </div>

        <!-- 日志输出 -->
        <div class="panel log-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">terminal</span>
            操作日志
            <div class="toolbar-spacer" />
            <button class="btn" @click="clearLog" style="padding:2px 6px">
              <span class="material-symbols-outlined" style="font-size:14px">delete_sweep</span>
              清空
            </button>
          </div>
          <div class="panel-body log-area">
            <div v-for="(msg, i) in logMessages" :key="i" class="log-line">{{ msg }}</div>
            <div v-if="logMessages.length === 0" class="log-empty">暂无日志</div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>串口: {{ config.port }} | {{ isOpen ? '已打开' : '已关闭' }}</span>
      <span>{{ config.baudRate }} | {{ config.dataBits }}{{ parityLabels[config.parity][0] }}{{ config.stopBits }}</span>
      <span>缓冲区: {{ advanced.bufferSize }}B | 超时: {{ advanced.timeout }}ms</span>
    </div>
  </div>
</template>

<style scoped>
.uart-config {
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
}

.toolbar-spacer {
  flex: 1;
}

.port-count {
  font-size: 11px;
  color: var(--on-surface-variant);
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

.btn:hover {
  opacity: 0.85;
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

.checkbox-label {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  cursor: pointer;
  font-size: 12px;
}

.main-content {
  display: flex;
  flex: 1;
  overflow: hidden;
  gap: 0;
}

.content-left {
  flex: 0 0 320px;
  display: flex;
  flex-direction: column;
  gap: 0;
  overflow-y: auto;
  border-right: 1px solid var(--outline-variant);
}

.content-right {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel {
  border-bottom: 1px solid var(--outline-variant);
}

.panel-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: var(--surface-container);
  font-weight: 600;
  font-size: 12px;
  border-bottom: 1px solid var(--outline-variant);
}

.panel-body {
  padding: 8px 10px;
}

.form-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
}

.form-row label:first-child {
  flex: 0 0 60px;
  text-align: right;
  color: var(--on-surface-variant);
  font-size: 12px;
}

.form-row .unit {
  color: var(--on-surface-variant);
  font-size: 11px;
}

.port-list {
  padding: 0;
}

.port-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 10px;
  cursor: pointer;
  border-bottom: 1px solid var(--outline-variant);
  transition: background 0.15s;
}

.port-item:hover {
  background: var(--surface-variant);
}

.port-item.selected {
  background: var(--primary);
  color: var(--on-primary);
}

.port-item.selected .port-desc,
.port-item.selected .port-status-text {
  color: var(--on-primary);
  opacity: 0.8;
}

.port-status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;
}

.port-status-dot.online {
  background: #4caf50;
  box-shadow: 0 0 4px #4caf50;
}

.port-status-dot.offline {
  background: #757575;
}

.port-info {
  flex: 1;
  min-width: 0;
}

.port-name {
  font-weight: 600;
  font-size: 12px;
}

.port-desc {
  font-size: 11px;
  color: var(--on-surface-variant);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.port-status-text {
  font-size: 11px;
  flex-shrink: 0;
}

.port-status-text.online {
  color: #4caf50;
}

.port-status-text.offline {
  color: var(--on-surface-variant);
}

.log-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.log-area {
  flex: 1;
  overflow-y: auto;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  line-height: 1.6;
  padding: 6px 10px;
}

.log-line {
  white-space: pre-wrap;
  word-break: break-all;
}

.log-empty {
  color: var(--on-surface-variant);
  text-align: center;
  padding: 20px;
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
