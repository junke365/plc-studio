<script setup lang="ts">
import { ref, computed } from 'vue'

interface SdoLogEntry {
  id: number
  timestamp: string
  direction: 'tx' | 'rx'
  index: string
  subIndex: string
  data: string
  status: 'ok' | 'error'
  errorMsg?: string
}

const targetNodeId = ref(1)
const transferType = ref<'expedited' | 'normal'>('expedited')
const sdoIndex = ref('1000')
const sdoSubIndex = ref('0')
const writeValue = ref('00000000')
const logs = ref<SdoLogEntry[]>([])
let nextId = 1

const errorCodeMap: Record<number, string> = {
  0x05030000: 'Toggle bit not changed',
  0x05040001: 'Client/server command specifier not valid',
  0x05040002: 'Invalid block size',
  0x05040003: 'Invalid sequence number',
  0x05040004: 'CRC error',
  0x05040005: 'Out of memory',
  0x06010000: 'Access to a non-existent object',
  0x06010001: 'Attempt to read a write-only object',
  0x06010002: 'Attempt to write a read-only object',
  0x06020000: 'Object does not exist in the object dictionary',
  0x06040041: 'Object cannot be mapped to the PDO',
  0x06040042: 'Number and length of objects exceed PDO length',
  0x06060000: 'Object conflict',
  0x06070010: 'Data type mismatch',
  0x06070012: 'Data type mismatch, length too high',
  0x06070013: 'Data type mismatch, length too low',
  0x06090011: 'Sub-index does not exist',
  0x06090030: 'Value range exceeded',
  0x08000000: 'General error',
  0x08000020: 'Transfer or store error',
}

function getTimestamp(): string {
  const now = new Date()
  return now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

function performUpload() {
  const isError = Math.random() > 0.85
  const dataLen = transferType.value === 'expedited' ? 4 : 8
  const dataBytes: string[] = []
  for (let i = 0; i < dataLen; i++) {
    dataBytes.push(Math.floor(Math.random() * 256).toString(16).toUpperCase().padStart(2, '0'))
  }
  const errCode = isError
    ? Object.keys(errorCodeMap)[Math.floor(Math.random() * Object.keys(errorCodeMap).length)] as unknown as number
    : 0

  logs.value.unshift({
    id: nextId++,
    timestamp: getTimestamp(),
    direction: 'tx',
    index: sdoIndex.value.toUpperCase(),
    subIndex: sdoSubIndex.value,
    data: '--',
    status: 'ok',
  })

  setTimeout(() => {
    logs.value.unshift({
      id: nextId++,
      timestamp: getTimestamp(),
      direction: 'rx',
      index: sdoIndex.value.toUpperCase(),
      subIndex: sdoSubIndex.value,
      data: isError ? `0x${errCode.toString(16).toUpperCase().padStart(8, '0')}` : dataBytes.join(' '),
      status: isError ? 'error' : 'ok',
      errorMsg: isError ? errorCodeMap[errCode] : undefined,
    })
  }, 50)
}

function performDownload() {
  const isError = Math.random() > 0.9
  const errCode = isError
    ? Object.keys(errorCodeMap)[Math.floor(Math.random() * Object.keys(errorCodeMap).length)] as unknown as number
    : 0

  logs.value.unshift({
    id: nextId++,
    timestamp: getTimestamp(),
    direction: 'tx',
    index: sdoIndex.value.toUpperCase(),
    subIndex: sdoSubIndex.value,
    data: writeValue.value,
    status: 'ok',
  })

  setTimeout(() => {
    logs.value.unshift({
      id: nextId++,
      timestamp: getTimestamp(),
      direction: 'rx',
      index: sdoIndex.value.toUpperCase(),
      subIndex: sdoSubIndex.value,
      data: isError ? `0x${errCode.toString(16).toUpperCase().padStart(8, '0')}` : 'OK',
      status: isError ? 'error' : 'ok',
      errorMsg: isError ? errorCodeMap[errCode] : undefined,
    })
  }, 50)
}

function clearLogs() {
  logs.value = []
}

function formatHex(val: string) {
  return val.replace(/[^0-9a-fA-F]/g, '').toUpperCase()
}
</script>

<template>
  <div class="canopen-sdo">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">目标节点ID:</label>
        <select v-model.number="targetNodeId" class="select-sm">
          <option v-for="n in 127" :key="n" :value="n">{{ n }}</option>
        </select>
      </div>
      <div class="toolbar-group">
        <label class="toolbar-label">传输类型:</label>
        <select v-model="transferType" class="select-sm">
          <option value="expedited">加速传输</option>
          <option value="normal">普通传输</option>
        </select>
      </div>
      <div class="toolbar-group">
        <label class="toolbar-label">索引:</label>
        <input v-model="sdoIndex" class="input-sm" style="width:72px" placeholder="0x1000" @input="sdoIndex = formatHex(sdoIndex)" />
        <label class="toolbar-label">子索引:</label>
        <input v-model="sdoSubIndex" class="input-sm" style="width:40px" @input="sdoSubIndex = formatHex(sdoSubIndex)" />
      </div>
    </div>

    <div class="sdo-actions">
      <div class="action-group">
        <button class="btn btn-primary" @click="performUpload">
          <span class="material-symbols-outlined" style="font-size:14px">upload</span>
          SDO 读取 (Upload)
        </button>
      </div>
      <div class="action-group">
        <label class="toolbar-label">写入值:</label>
        <input v-model="writeValue" class="input-sm" style="width:120px" placeholder="HEX数据" @input="writeValue = formatHex(writeValue)" />
        <button class="btn btn-success" @click="performDownload">
          <span class="material-symbols-outlined" style="font-size:14px">download</span>
          SDO 写入 (Download)
        </button>
      </div>
      <div class="toolbar-spacer" />
      <button class="btn" @click="clearLogs">
        <span class="material-symbols-outlined" style="font-size:12px">delete</span>
        清空日志
      </button>
    </div>

    <div class="log-section">
      <div class="log-header">
        <span class="material-symbols-outlined" style="font-size:16px">list_alt</span>
        通信日志
        <span class="log-count">{{ logs.length }}</span>
      </div>
      <div class="log-table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th style="width:120px">时间</th>
              <th style="width:50px">方向</th>
              <th style="width:72px">索引</th>
              <th style="width:50px">子索引</th>
              <th>数据</th>
              <th style="width:60px">状态</th>
              <th style="width:160px">错误信息</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="log in logs" :key="log.id" :class="{ 'row-error': log.status === 'error' }">
              <td class="mono">{{ log.timestamp }}</td>
              <td>
                <span :class="['dir-badge', log.direction]">
                  {{ log.direction === 'tx' ? 'TX' : 'RX' }}
                </span>
              </td>
              <td class="mono">0x{{ log.index }}</td>
              <td class="mono">{{ log.subIndex }}</td>
              <td class="mono" style="font-size:11px">{{ log.data }}</td>
              <td>
                <span :class="['status-tag', log.status]">
                  {{ log.status === 'ok' ? '成功' : '错误' }}
                </span>
              </td>
              <td class="mono error-msg">{{ log.errorMsg || '--' }}</td>
            </tr>
            <tr v-if="logs.length === 0">
              <td colspan="7" class="empty-cell">无通信记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div class="status-bar">
      <span>节点ID: {{ targetNodeId }}</span>
      <span>索引: 0x{{ sdoIndex.toUpperCase() }}:{{ sdoSubIndex }}</span>
      <span>传输: {{ transferType === 'expedited' ? '加速' : '普通' }}</span>
      <span>日志: {{ logs.length }}</span>
    </div>
  </div>
</template>

<style scoped>
.canopen-sdo {
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
  gap: 8px;
  padding: 6px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
  flex-wrap: wrap;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.toolbar-label {
  font-size: 11px;
  color: var(--on-surface-variant);
}
.toolbar-spacer { flex: 1; }
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
.btn:hover { opacity: 0.85; }
.btn-primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-danger { background: #c62828; color: white; border-color: #c62828; }
.btn-success { background: #2e7d32; color: white; border-color: #2e7d32; }
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
.input-sm:focus { border-color: var(--primary); outline: none; }
.mono { font-family: 'JetBrains Mono', monospace; }
.sdo-actions {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
  flex-wrap: wrap;
}
.action-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.log-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-height: 0;
}
.log-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  font-size: 12px;
  flex-shrink: 0;
}
.log-count {
  font-size: 11px;
  font-weight: 400;
  color: var(--on-surface-variant);
}
.log-table-wrap {
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
  padding: 6px 8px;
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
.row-error { background: rgba(198, 40, 40, 0.05); }
.dir-badge {
  display: inline-block;
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 10px;
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
}
.dir-badge.tx { background: rgba(33, 150, 243, 0.15); color: #2196f3; }
.dir-badge.rx { background: rgba(76, 175, 80, 0.15); color: #4caf50; }
.status-tag {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 10px;
  font-weight: 600;
}
.status-tag.ok { background: rgba(46, 125, 50, 0.15); color: #2e7d32; }
.status-tag.error { background: rgba(198, 40, 40, 0.15); color: #c62828; }
.error-msg {
  font-size: 10px;
  color: #c62828;
}
.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 40px !important;
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
