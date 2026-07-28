<script setup lang="ts">
import { ref, reactive, computed } from 'vue'

interface ParsedFrame {
  id: number
  raw: string
  timestamp: string
  slaveAddr: string
  funcCode: string
  funcDesc: string
  dataField: string
  crcValue: string
  crcValid: boolean
  frameHeader: string
}

interface Stats {
  total: number
  success: number
  error: number
  timeout: number
}

const rawInput = ref('01 03 00 00 00 0A C5 CD')
const parseResult = ref<ParsedFrame | null>(null)
const history = ref<ParsedFrame[]>([])
const stats = reactive<Stats>({ total: 0, success: 0, error: 0, timeout: 0 })
const nextId = ref(1)
const showOnlyErrors = ref(false)

const funcCodeMap: Record<number, string> = {
  0x01: '读线圈 Read Coils',
  0x02: '读离散输入 Read Discrete Inputs',
  0x03: '读保持寄存器 Read Holding Registers',
  0x04: '读输入寄存器 Read Input Registers',
  0x05: '写单个线圈 Write Single Coil',
  0x06: '写单个寄存器 Write Single Register',
  0x07: '读异常状态 Read Exception Status',
  0x08: '诊断 Diagnostics',
  0x0B: '获取通信事件计数器',
  0x0C: '获取通信事件日志',
  0x0F: '写多个线圈 Write Multiple Coils',
  0x10: '写多个寄存器 Write Multiple Registers',
  0x11: '报告服务器ID',
  0x14: '读文件记录',
  0x15: '写文件记录',
  0x16: '屏蔽写寄存器',
  0x17: '读/写多个寄存器',
  0x18: '读FIFO队列',
  0x2B: '封装接口传输',
}

const filteredHistory = computed(() => {
  if (showOnlyErrors.value) {
    return history.value.filter(h => !h.crcValid)
  }
  return history.value
})

function calcCrc16(data: number[]): number {
  let crc = 0xffff
  for (const byte of data) {
    crc ^= byte
    for (let i = 0; i < 8; i++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xa001
      } else {
        crc >>= 1
      }
    }
  }
  return crc
}

function parseHexInput(): ParsedFrame | null {
  const cleaned = rawInput.value.replace(/[^0-9a-fA-F\s]/g, '').trim()
  if (!cleaned) return null

  const bytes = cleaned.split(/\s+/).map(s => parseInt(s, 16))
  if (bytes.length < 4) return null

  const slaveAddr = bytes[0]
  const funcCode = bytes[1]
  const dataBytes = bytes.slice(2, bytes.length - 2)
  const crcReceived = (bytes[bytes.length - 2]) | (bytes[bytes.length - 1] << 8)
  const crcCalc = calcCrc16(bytes.slice(0, bytes.length - 2))
  const crcValid = crcReceived === crcCalc

  const frameHeader = bytes.length >= 6
    ? `MBAP: ${bytes.slice(0, 4).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')}`
    : `帧头: ${bytes.slice(0, Math.min(2, bytes.length)).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')}`

  const time = new Date().toLocaleTimeString('zh-CN', { hour12: false })

  return {
    id: nextId.value++,
    raw: cleaned,
    timestamp: time,
    slaveAddr: `0x${slaveAddr.toString(16).toUpperCase().padStart(2, '0')} (${slaveAddr})`,
    funcCode: `0x${funcCode.toString(16).toUpperCase().padStart(2, '0')} (${funcCode})`,
    funcDesc: funcCodeMap[funcCode] || `未知功能码`,
    dataField: dataBytes.length > 0
      ? dataBytes.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
      : '(空)',
    crcValue: `0x${crcCalc.toString(16).toUpperCase().padStart(4, '0')}${crcValid ? ' ✓' : ' ✗ (期望 0x' + crcReceived.toString(16).toUpperCase().padStart(4, '0') + ')'}`,
    crcValid,
    frameHeader,
  }
}

function doParse() {
  const result = parseHexInput()
  if (!result) {
    parseResult.value = null
    return
  }
  parseResult.value = result
  stats.total++
  if (result.crcValid) {
    stats.success++
  } else {
    stats.error++
  }
  history.value.unshift(result)
  if (history.value.length > 200) history.value.pop()
}

function parseAndDemo() {
  // 生成模拟报文并解析
  const funcCodes = [1, 2, 3, 4, 5, 6, 15, 16]
  const fc = funcCodes[Math.floor(Math.random() * funcCodes.length)]
  const slaveAddr = Math.floor(Math.random() * 247) + 1
  const startAddr = Math.floor(Math.random() * 1000)
  const quantity = Math.floor(Math.random() * 10) + 1

  const frame: number[] = [slaveAddr, fc]
  frame.push((startAddr >> 8) & 0xff, startAddr & 0xff)
  frame.push((quantity >> 8) & 0xff, quantity & 0xff)

  // 随机是否加入CRC错误
  const introduceError = Math.random() < 0.15
  if (introduceError) {
    // 故意让CRC错误
    const badCrc = 0x0000
    frame.push(badCrc & 0xff, (badCrc >> 8) & 0xff)
  } else {
    const crc = calcCrc16(frame)
    frame.push(crc & 0xff, (crc >> 8) & 0xff)
  }

  const hex = frame.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')
  rawInput.value = hex
  doParse()
}

function clearHistory() {
  history.value = []
  stats.total = 0
  stats.success = 0
  stats.error = 0
  stats.timeout = 0
}

function clearInput() {
  rawInput.value = ''
  parseResult.value = null
}

function loadHistoryToInput(frame: ParsedFrame) {
  rawInput.value = frame.raw
  parseResult.value = frame
}

const successRate = computed(() => {
  if (stats.total === 0) return '0%'
  return ((stats.success / stats.total) * 100).toFixed(1) + '%'
})
</script>

<template>
  <div class="modbus-analyzer">
    <div class="toolbar">
      <button class="btn btn-primary" @click="doParse">
        <span class="material-symbols-outlined" style="font-size:16px">biotech</span>
        解析
      </button>
      <button class="btn" @click="parseAndDemo">
        <span class="material-symbols-outlined" style="font-size:16px">casino</span>
        模拟报文
      </button>
      <button class="btn" @click="clearInput">
        <span class="material-symbols-outlined" style="font-size:16px">backspace</span>
        清空输入
      </button>
      <div class="toolbar-separator" />
      <button class="btn" @click="clearHistory">
        <span class="material-symbols-outlined" style="font-size:16px">delete_sweep</span>
        清空历史
      </button>
      <div class="toolbar-spacer" />
      <label class="checkbox-label">
        <input type="checkbox" v-model="showOnlyErrors" />
        <span>仅显示错误帧</span>
      </label>
    </div>

    <div class="main-content">
      <div class="content-left">
        <!-- 报文输入 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">edit</span>
            报文输入
            <span class="header-hint">粘贴 HEX 报文 (空格分隔)</span>
          </div>
          <div class="panel-body">
            <textarea
              v-model="rawInput"
              class="hex-input"
              placeholder="例如: 01 03 00 00 00 0A C5 CD"
              rows="3"
              spellcheck="false"
            />
          </div>
        </div>

        <!-- 解析结果 -->
        <div class="panel" v-if="parseResult">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">fact_check</span>
            解析结果
            <span v-if="parseResult.crcValid" class="parse-status valid">CRC 校验通过</span>
            <span v-else class="parse-status invalid">CRC 校验失败</span>
          </div>
          <div class="panel-body">
            <div class="result-grid">
              <div class="result-item">
                <span class="result-label">帧头</span>
                <span class="result-value mono">{{ parseResult.frameHeader }}</span>
              </div>
              <div class="result-item">
                <span class="result-label">从站地址</span>
                <span class="result-value mono">{{ parseResult.slaveAddr }}</span>
              </div>
              <div class="result-item">
                <span class="result-label">功能码</span>
                <span class="result-value mono">{{ parseResult.funcCode }}</span>
              </div>
              <div class="result-item full">
                <span class="result-label">功能描述</span>
                <span class="result-value">{{ parseResult.funcDesc }}</span>
              </div>
              <div class="result-item full">
                <span class="result-label">数据域</span>
                <span class="result-value mono">{{ parseResult.dataField }}</span>
              </div>
              <div class="result-item full">
                <span class="result-label">CRC 校验</span>
                <span class="result-value mono" :class="{ valid: parseResult.crcValid, invalid: !parseResult.crcValid }">
                  {{ parseResult.crcValue }}
                </span>
              </div>
            </div>
          </div>
        </div>

        <!-- 统计信息 -->
        <div class="panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">analytics</span>
            统计信息
          </div>
          <div class="panel-body">
            <div class="stats-grid">
              <div class="stat-card">
                <div class="stat-value mono">{{ stats.total }}</div>
                <div class="stat-label">总帧数</div>
              </div>
              <div class="stat-card success">
                <div class="stat-value mono">{{ stats.success }}</div>
                <div class="stat-label">成功帧</div>
              </div>
              <div class="stat-card error">
                <div class="stat-value mono">{{ stats.error }}</div>
                <div class="stat-label">错误帧</div>
              </div>
              <div class="stat-card timeout">
                <div class="stat-value mono">{{ stats.timeout }}</div>
                <div class="stat-label">超时帧</div>
              </div>
              <div class="stat-card rate">
                <div class="stat-value mono">{{ successRate }}</div>
                <div class="stat-label">成功率</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="content-right">
        <!-- 报文历史 -->
        <div class="panel history-panel">
          <div class="panel-header">
            <span class="material-symbols-outlined" style="font-size:16px">history</span>
            报文历史
            <div class="toolbar-spacer" />
            <span class="history-count">{{ filteredHistory.length }} 条</span>
          </div>
          <div class="panel-body table-wrap">
            <table class="data-table">
              <thead>
                <tr>
                  <th style="width:28px"></th>
                  <th>时间</th>
                  <th>ID</th>
                  <th>从站</th>
                  <th>功能码</th>
                  <th>描述</th>
                  <th>CRC</th>
                </tr>
              </thead>
              <tbody>
                <tr
                  v-for="frame in filteredHistory"
                  :key="frame.id"
                  :class="{ 'error-row': !frame.crcValid }"
                  class="history-row"
                  @click="loadHistoryToInput(frame)"
                >
                  <td>
                    <span v-if="!frame.crcValid" class="material-symbols-outlined" style="font-size:14px;color:#f44336">error</span>
                    <span v-else class="material-symbols-outlined" style="font-size:14px;color:#4caf50">check_circle</span>
                  </td>
                  <td class="mono">{{ frame.timestamp }}</td>
                  <td class="mono">{{ frame.id }}</td>
                  <td class="mono">{{ frame.slaveAddr.split(' ')[0] }}</td>
                  <td class="mono">{{ frame.funcCode.split(' ')[0] }}</td>
                  <td class="func-desc-cell">{{ frame.funcDesc.split(' ')[0] }}</td>
                  <td class="mono" :class="{ valid: frame.crcValid, invalid: !frame.crcValid }">
                    {{ frame.crcValid ? '✓ 有效' : '✗ 错误' }}
                  </td>
                </tr>
                <tr v-if="filteredHistory.length === 0">
                  <td colspan="7" class="empty-cell">暂无历史记录</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>

    <div class="status-bar">
      <span>总帧: {{ stats.total }}</span>
      <span>成功: {{ stats.success }} | 错误: {{ stats.error }} | 超时: {{ stats.timeout }}</span>
      <span>成功率: {{ successRate }}</span>
    </div>
  </div>
</template>

<style scoped>
.modbus-analyzer {
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

.toolbar-spacer { flex: 1; }

.toolbar-separator {
  width: 1px;
  height: 20px;
  background: var(--outline-variant);
  margin: 0 2px;
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
  font-size: 11px;
  color: var(--on-surface-variant);
}

.main-content {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.content-left {
  flex: 0 0 420px;
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

.header-hint {
  font-weight: 400;
  font-size: 11px;
  color: var(--on-surface-variant);
  margin-left: auto;
}

.panel-body { padding: 8px 10px; }

.hex-input {
  width: 100%;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 8px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 13px;
  line-height: 1.6;
  resize: vertical;
  outline: none;
}

.hex-input:focus {
  border-color: var(--primary);
}

.parse-status {
  margin-left: auto;
  font-size: 11px;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 3px;
}

.parse-status.valid {
  background: rgba(76, 175, 80, 0.15);
  color: #4caf50;
}

.parse-status.invalid {
  background: rgba(244, 67, 54, 0.15);
  color: #f44336;
}

.result-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.result-item {
  display: flex;
  flex-direction: column;
  gap: 2px;
  padding: 6px 8px;
  background: var(--surface-container);
  border-radius: 4px;
}

.result-item.full {
  grid-column: 1 / -1;
}

.result-label {
  font-size: 10px;
  color: var(--on-surface-variant);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.result-value {
  font-size: 12px;
  font-weight: 600;
}

.result-value.valid {
  color: #4caf50;
}

.result-value.invalid {
  color: #f44336;
}

.mono {
  font-family: 'JetBrains Mono', monospace;
}

.stats-grid {
  display: flex;
  gap: 8px;
}

.stat-card {
  flex: 1;
  text-align: center;
  padding: 10px 8px;
  background: var(--surface-container);
  border-radius: 6px;
  border: 1px solid var(--outline-variant);
}

.stat-value {
  font-size: 20px;
  font-weight: 700;
}

.stat-card.success .stat-value { color: #4caf50; }
.stat-card.error .stat-value { color: #f44336; }
.stat-card.timeout .stat-value { color: #ff9800; }
.stat-card.rate .stat-value { color: var(--primary); }

.stat-label {
  font-size: 10px;
  color: var(--on-surface-variant);
  margin-top: 2px;
}

.history-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.history-count {
  font-size: 11px;
  color: var(--on-surface-variant);
  font-weight: 400;
}

.table-wrap {
  flex: 1;
  overflow-y: auto;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 11px;
}

.data-table th {
  text-align: left;
  padding: 5px 8px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  position: sticky;
  top: 0;
  z-index: 1;
}

.data-table td {
  padding: 4px 8px;
  border-bottom: 1px solid var(--outline-variant);
}

.history-row {
  cursor: pointer;
  transition: background 0.1s;
}

.history-row:hover {
  background: var(--surface-variant);
}

.history-row.error-row {
  background: rgba(244, 67, 54, 0.05);
}

.history-row.error-row:hover {
  background: rgba(244, 67, 54, 0.1);
}

.func-desc-cell {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.valid {
  color: #4caf50;
}

.invalid {
  color: #f44336;
}

.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 20px !important;
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
