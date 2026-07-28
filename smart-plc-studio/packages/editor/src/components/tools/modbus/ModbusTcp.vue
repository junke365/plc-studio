<script setup lang="ts">
import { ref, reactive, onUnmounted } from 'vue'
import {
  tcpConnect,
  tcpDisconnect,
  tcpSend,
  onTcpData,
  onTcpClosed,
} from '@/serial/serialClient'

interface Transaction {
  time: string
  transId: number
  funcCode: string
  status: 'success' | 'error' | 'timeout'
}

interface ModbusRegister {
  address: string
  value: string
  hex: string
}

const TCP_CLIENT_ID = 'modbus-tcp'

const connected = ref(false)
const transIdCounter = ref(0)

const connConfig = reactive({
  ip: '192.168.1.100',
  port: 502,
})

const request = reactive({
  funcCode: 3,
  startAddr: 0,
  quantity: 10,
})

const registers = ref<ModbusRegister[]>([])
const transactions = ref<Transaction[]>([])
const logMessages = ref<string[]>([])

const funcCodes = [
  { code: 1, name: '01 - 读线圈', desc: 'Read Coils' },
  { code: 2, name: '02 - 读离散输入', desc: 'Read Discrete Inputs' },
  { code: 3, name: '03 - 读保持寄存器', desc: 'Read Holding Registers' },
  { code: 4, name: '04 - 读输入寄存器', desc: 'Read Input Registers' },
  { code: 5, name: '05 - 写单个线圈', desc: 'Write Single Coil' },
  { code: 6, name: '06 - 写单个寄存器', desc: 'Write Single Register' },
  { code: 15, name: '15 - 写多个线圈', desc: 'Write Multiple Coils' },
  { code: 16, name: '16 - 写多个寄存器', desc: 'Write Multiple Registers' },
]

// TCP 接收缓冲区和清理函数
let tcpRxBuffer: number[] = []
let cleanupTcpData: (() => void) | null = null
let cleanupTcpClosed: (() => void) | null = null
let responseTimer: ReturnType<typeof setTimeout> | null = null

function isWriteCode(code: number): boolean {
  return [5, 6, 15, 16].includes(code)
}

function addLog(msg: string) {
  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  logMessages.value.push(`[${time}] ${msg}`)
  if (logMessages.value.length > 200) logMessages.value.shift()
}

function buildMbapFrame(): number[] {
  transIdCounter.value++
  const tid = transIdCounter.value
  const pdu: number[] = [request.funcCode]
  pdu.push((request.startAddr >> 8) & 0xff, request.startAddr & 0xff)
  pdu.push((request.quantity >> 8) & 0xff, request.quantity & 0xff)
  if (isWriteCode(request.funcCode)) {
    if (request.funcCode === 5 || request.funcCode === 6) {
      pdu.splice(3)
      pdu.push(0xff, 0x00)
    } else {
      pdu.push(Math.ceil(request.quantity / 8))
      for (let i = 0; i < Math.ceil(request.quantity / 8); i++) pdu.push(0x00)
    }
  }
  const mbapHeader = [
    (tid >> 8) & 0xff, tid & 0xff,
    0x00, 0x00,
    0x00, pdu.length + 1,
    0x01,
  ]
  return [...mbapHeader, ...pdu]
}

function parseResponse(frame: number[]) {
  registers.value = []
  if (isWriteCode(request.funcCode)) {
    for (let i = 0; i < request.quantity; i++) {
      registers.value.push({
        address: `0x${(request.startAddr + i).toString(16).toUpperCase().padStart(4, '0')}`,
        value: String(request.quantity),
        hex: ((request.quantity >> 8) & 0xff).toString(16).toUpperCase().padStart(2, '0') +
             (request.quantity & 0xff).toString(16).toUpperCase().padStart(2, '0'),
      })
    }
  } else {
    const dataStart = 7
    for (let i = 0; i < request.quantity; i++) {
      const hi = frame[dataStart + i * 2] || 0
      const lo = frame[dataStart + i * 2 + 1] || 0
      const val = (hi << 8) | lo
      registers.value.push({
        address: `0x${(request.startAddr + i).toString(16).toUpperCase().padStart(4, '0')}`,
        value: String(val),
        hex: hi.toString(16).toUpperCase().padStart(2, '0') + lo.toString(16).toUpperCase().padStart(2, '0'),
      })
    }
  }
}

function frameToHex(frame: number[]): string {
  return frame.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
}

// 处理完整的TCP响应帧（MBAP头+PDU）
function processTcpResponse(frame: number[]) {
  if (frame.length < 7) {
    addLog(`RX 帧过短: ${frame.length} 字节`)
    return
  }
  const tid = (frame[0] << 8) | frame[1]
  const pduLen = (frame[4] << 8) | frame[5]
  const rxHex = frameToHex(frame)
  addLog(`RX [TID=${tid}]: ${rxHex}`)

  // 检查是否为异常响应（功能码最高位为1）
  const funcCode = frame[7]
  const isError = (funcCode & 0x80) !== 0

  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  transactions.value.unshift({
    time,
    transId: tid,
    funcCode: funcCodes.find(f => f.code === request.funcCode)?.name || '',
    status: isError ? 'error' : 'success',
  })
  if (transactions.value.length > 100) transactions.value.pop()

  if (!isError) {
    parseResponse(frame)
  } else {
    const excCode = frame.length > 8 ? frame[8] : -1
    addLog(`异常响应 - 功能码 0x${funcCode.toString(16).toUpperCase()} 异常码 0x${excCode.toString(16).toUpperCase()}`)
  }
}

function sendRequest() {
  if (!connected.value) {
    addLog('错误: 未连接到服务器')
    return
  }
  const frame = buildMbapFrame()
  const hex = frameToHex(frame)
  const tid = transIdCounter.value
  addLog(`TX [TID=${tid}]: ${hex}`)

  if (responseTimer) { clearTimeout(responseTimer) }
  responseTimer = setTimeout(() => {
    const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    addLog(`RX [TID=${tid}]: 响应超时`)
    transactions.value.unshift({
      time,
      transId: tid,
      funcCode: funcCodes.find(f => f.code === request.funcCode)?.name || '',
      status: 'timeout',
    })
    if (transactions.value.length > 100) transactions.value.pop()
    responseTimer = null
  }, 3000)

  tcpSend(TCP_CLIENT_ID, frame).then(result => {
    if (!result.success) {
      addLog(`发送失败: ${result.error}`)
      if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
    }
  })
}

function clearLog() {
  logMessages.value = []
}

function clearAll() {
  logMessages.value = []
  transactions.value = []
  registers.value = []
  transIdCounter.value = 0
}

function toHex(val: string): string {
  const n = parseInt(val)
  if (isNaN(n)) return val
  return '0x' + (n & 0xffff).toString(16).toUpperCase().padStart(4, '0')
}

async function toggleConnection() {
  if (!connected.value) {
    const result = await tcpConnect(TCP_CLIENT_ID, connConfig.ip, connConfig.port)
    if (!result.success) {
      addLog(`TCP 连接失败: ${result.error}`)
      return
    }
    // 注册TCP数据监听，缓冲接收MBAP帧
    tcpRxBuffer = []
    cleanupTcpData = onTcpData((data) => {
      if (data.id !== TCP_CLIENT_ID) return
      if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
      tcpRxBuffer.push(...data.data)
      // 按MBAP头中的长度字段分帧
      while (tcpRxBuffer.length >= 7) {
        const pduLen = (tcpRxBuffer[4] << 8) | tcpRxBuffer[5]
        const totalLen = 6 + pduLen  // MBAP头前6字节 + PDU长度
        if (tcpRxBuffer.length >= totalLen) {
          const frame = tcpRxBuffer.splice(0, totalLen)
          processTcpResponse(frame)
        } else {
          break
        }
      }
    })
    // 监听连接关闭事件
    cleanupTcpClosed = onTcpClosed((data) => {
      if (data.id !== TCP_CLIENT_ID) return
      connected.value = false
      addLog('TCP 连接已被远程关闭')
    })
    connected.value = true
    addLog(`TCP 连接到 ${connConfig.ip}:${connConfig.port}`)
  } else {
    if (cleanupTcpData) { cleanupTcpData(); cleanupTcpData = null }
    if (cleanupTcpClosed) { cleanupTcpClosed(); cleanupTcpClosed = null }
    if (responseTimer) { clearTimeout(responseTimer); responseTimer = null }
    tcpRxBuffer = []
    await tcpDisconnect(TCP_CLIENT_ID)
    connected.value = false
    addLog('TCP 连接已断开')
  }
}

const successCount = ref(0)
const errorCount = ref(0)

function getStatusClass(s: string) {
  if (s === 'success') return 'status-success'
  if (s === 'error') return 'status-error'
  return 'status-timeout'
}

// 组件卸载时清理资源
onUnmounted(() => {
  if (cleanupTcpData) cleanupTcpData()
  if (cleanupTcpClosed) cleanupTcpClosed()
  if (responseTimer) clearTimeout(responseTimer)
  if (connected.value) {
    tcpDisconnect(TCP_CLIENT_ID)
  }
})
</script>

<template>
  <div class="modbus-tcp">
    <div class="toolbar">
      <label class="toolbar-label">IP:</label>
      <input v-model="connConfig.ip" class="input-sm" style="width:120px" placeholder="192.168.1.100" />
      <label class="toolbar-label">端口:</label>
      <input v-model.number="connConfig.port" type="number" class="input-sm" style="width:64px" min="1" max="65535" />
      <button class="btn" :class="{ 'btn-primary': !connected }" @click="toggleConnection">
        <span class="material-symbols-outlined" style="font-size:16px">{{ connected ? 'link_off' : 'link' }}</span>
        {{ connected ? '断开' : '连接' }}
      </button>
      <span class="conn-dot" :class="{ on: connected }" />
      <div class="toolbar-spacer" />
      <button class="btn" @click="clearAll">
        <span class="material-symbols-outlined" style="font-size:16px">delete_sweep</span>
        清空全部
      </button>
    </div>

    <div class="main-content">
      <div class="content-left">
        <!-- 功能码 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">code</span>
            功能码
          </div>
          <div class="panel-body">
            <div class="func-grid">
              <button
                v-for="fc in funcCodes"
                :key="fc.code"
                class="func-btn"
                :class="{ active: request.funcCode === fc.code, write: isWriteCode(fc.code) }"
                @click="request.funcCode = fc.code"
              >
                <span class="func-code">{{ fc.code }}</span>
                <span class="func-name">{{ fc.name.split(' - ')[1] }}</span>
                <span v-if="isWriteCode(fc.code)" class="write-badge">W</span>
              </button>
            </div>
          </div>
        </div>

        <!-- 请求参数 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">edit_note</span>
            请求参数
          </div>
          <div class="panel-body">
            <div class="form-row">
              <label>起始地址:</label>
              <input v-model.number="request.startAddr" type="number" class="input-sm" min="0" max="65535" />
              <span class="unit">({{ toHex(String(request.startAddr)) }})</span>
            </div>
            <div class="form-row">
              <label>数量:</label>
              <input v-model.number="request.quantity" type="number" class="input-sm" min="1" max="125" />
            </div>
            <div class="send-row">
              <button class="btn btn-primary" :disabled="!connected" @click="sendRequest">
                <span class="material-symbols-outlined" style="font-size:16px">send</span>
                发送请求
              </button>
            </div>
          </div>
        </div>

        <!-- 响应数据 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">table_chart</span>
            响应数据
          </div>
          <div class="panel-body table-wrap">
            <table class="data-table">
              <thead>
                <tr>
                  <th>地址</th>
                  <th>值</th>
                  <th>HEX</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(reg, i) in registers" :key="i">
                  <td class="mono">{{ reg.address }}</td>
                  <td class="mono">{{ reg.value }}</td>
                  <td class="mono">{{ reg.hex }}</td>
                </tr>
                <tr v-if="registers.length === 0">
                  <td colspan="3" class="empty-cell">无数据</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <div class="content-right">
        <!-- 事务记录 -->
        <div class="panel tx-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">history</span>
            事务记录
            <div class="toolbar-spacer" />
            <span class="tx-count">{{ transactions.length }} 条</span>
          </div>
          <div class="panel-body table-wrap">
            <table class="data-table">
              <thead>
                <tr>
                  <th>时间</th>
                  <th>TID</th>
                  <th>功能码</th>
                  <th>状态</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="(tx, i) in transactions" :key="i" :class="getStatusClass(tx.status)">
                  <td class="mono">{{ tx.time }}</td>
                  <td class="mono">{{ tx.transId }}</td>
                  <td>{{ tx.funcCode }}</td>
                  <td>
                    <span class="status-badge" :class="getStatusClass(tx.status)">
                      {{ tx.status === 'success' ? '成功' : tx.status === 'error' ? '异常' : '超时' }}
                    </span>
                  </td>
                </tr>
                <tr v-if="transactions.length === 0">
                  <td colspan="4" class="empty-cell">无事务记录</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <!-- 原始报文 HEX -->
        <div class="panel raw-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">data_object</span>
            原始报文 HEX
          </div>
          <div class="panel-body raw-hex">
            <div v-for="(msg, i) in logMessages.slice(-50)" :key="i" class="hex-line">
              {{ msg }}
            </div>
            <div v-if="logMessages.length === 0" class="log-empty">暂无报文</div>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ connConfig.ip }}:{{ connConfig.port }} | {{ connected ? '已连接' : '未连接' }}</span>
      <span>事务 ID: {{ transIdCounter }}</span>
      <span>功能码: {{ funcCodes.find(f => f.code === request.funcCode)?.name || '' }}</span>
    </div>
  </div>
</template>

<style scoped>
.modbus-tcp {
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

.conn-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: #757575;
  flex-shrink: 0;
}

.conn-dot.on {
  background: #4caf50;
  box-shadow: 0 0 6px #4caf50;
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
.btn:disabled { opacity: 0.4; cursor: not-allowed; }

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

.main-content {
  display: flex;
  flex: 1;
  overflow: hidden;
}

.content-left {
  flex: 0 0 360px;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
  border-right: 1px solid var(--outline-variant);
}

.content-right {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel { border-bottom: 1px solid var(--outline-variant); }

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

.panel-body { padding: 8px 10px; }

.func-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 4px;
}

.func-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 8px;
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  background: var(--surface-variant);
  color: var(--on-surface);
  font-size: 11px;
  cursor: pointer;
  position: relative;
}

.func-btn.active {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.func-btn.write .func-code { color: #ff9800; }
.func-btn.active.write .func-code { color: var(--on-primary); }

.func-code {
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
  min-width: 20px;
}

.func-name { flex: 1; }

.write-badge {
  background: #ff9800;
  color: #000;
  font-size: 9px;
  padding: 1px 4px;
  border-radius: 3px;
  font-weight: 700;
}

.form-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
}

.form-row label:first-child {
  flex: 0 0 70px;
  text-align: right;
  color: var(--on-surface-variant);
  font-size: 12px;
}

.form-row .unit {
  color: var(--on-surface-variant);
  font-size: 11px;
  font-family: 'JetBrains Mono', monospace;
}

.send-row {
  margin-top: 8px;
}

.table-wrap {
  max-height: 300px;
  overflow-y: auto;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.data-table th {
  text-align: left;
  padding: 4px 8px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  position: sticky;
  top: 0;
}

.data-table td {
  padding: 3px 8px;
  border-bottom: 1px solid var(--outline-variant);
}

.mono {
  font-family: 'JetBrains Mono', monospace;
}

.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 16px !important;
}

.tx-panel {
  flex: 0 0 240px;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.tx-count {
  font-size: 11px;
  color: var(--on-surface-variant);
  font-weight: 400;
}

.status-success td {
  color: var(--on-surface);
}

.status-error td {
  color: #f44336;
}

.status-badge {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 10px;
  font-weight: 600;
}

.status-badge.status-success {
  background: rgba(76, 175, 80, 0.15);
  color: #4caf50;
}

.status-badge.status-error {
  background: rgba(244, 67, 54, 0.15);
  color: #f44336;
}

.status-badge.status-timeout {
  background: rgba(255, 152, 0, 0.15);
  color: #ff9800;
}

.raw-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.raw-hex {
  flex: 1;
  overflow-y: auto;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  line-height: 1.6;
  padding: 6px 10px;
}

.hex-line {
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
