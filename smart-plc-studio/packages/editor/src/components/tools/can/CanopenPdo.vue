<script setup lang="ts">
import { ref, reactive, computed, onUnmounted } from 'vue'

interface PdoMapping {
  id: number
  pdoNumber: number
  cobId: string
  transType: number
  enabled: boolean
}

interface PdoDataItem {
  byteIndex: number
  value: string
}

const pdoType = ref<'rpdo' | 'tpdo'>('rpdo')
let nextPdoId = 1

const mappings = ref<PdoMapping[]>([
  { id: nextPdoId++, pdoNumber: 1, cobId: '0x200', transType: 255, enabled: true },
  { id: nextPdoId++, pdoNumber: 2, cobId: '0x300', transType: 255, enabled: true },
  { id: nextPdoId++, pdoNumber: 3, cobId: '0x400', transType: 255, enabled: false },
  { id: nextPdoId++, pdoNumber: 4, cobId: '0x500', transType: 255, enabled: false },
])

const liveData = ref<Record<number, PdoDataItem[]>>({})
const cycleCount = ref(0)
const cycleRate = ref(0)
let timer: ReturnType<typeof setInterval> | null = null
const running = ref(false)

function initLiveData() {
  mappings.value.forEach(m => {
    if (!liveData.value[m.pdoNumber]) {
      liveData.value[m.pdoNumber] = Array.from({ length: 8 }, (_, i) => ({
        byteIndex: i,
        value: '00',
      }))
    }
  })
}

function randomizeData() {
  mappings.value.forEach(m => {
    if (m.enabled && liveData.value[m.pdoNumber]) {
      liveData.value[m.pdoNumber].forEach(b => {
        b.value = Math.floor(Math.random() * 256).toString(16).toUpperCase().padStart(2, '0')
      })
    }
  })
  cycleCount.value++
}

function startMonitor() {
  running.value = true
  cycleCount.value = 0
  randomizeData()
  timer = setInterval(() => {
    randomizeData()
  }, 100)
  setTimeout(() => {
    cycleRate.value = Math.round(cycleCount.value / ((Date.now() - (Date.now() - cycleCount.value * 100)) / 1000)) || 10
  }, 1000)
}

function stopMonitor() {
  running.value = false
  if (timer) {
    clearInterval(timer)
    timer = null
  }
}

function togglePdoEnabled(pdo: PdoMapping) {
  pdo.enabled = !pdo.enabled
  if (!pdo.enabled) {
    delete liveData.value[pdo.pdoNumber]
  } else {
    initLiveData()
  }
}

function addPdo() {
  const num = mappings.value.length + 1
  const prefix = pdoType.value === 'rpdo' ? '0x200' : '0x180'
  mappings.value.push({
    id: nextPdoId++,
    pdoNumber: num,
    cobId: `0x${(parseInt(prefix) + num * 0x100).toString(16).toUpperCase()}`,
    transType: 255,
    enabled: false,
  })
}

function removePdo(id: number) {
  const pdo = mappings.value.find(m => m.id === id)
  if (pdo) {
    delete liveData.value[pdo.pdoNumber]
    mappings.value = mappings.value.filter(m => m.id !== id)
  }
}

initLiveData()

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<template>
  <div class="canopen-pdo">
    <div class="toolbar">
      <div class="toolbar-group">
        <label class="toolbar-label">PDO类型:</label>
        <select v-model="pdoType" class="select-sm">
          <option value="rpdo">RPDO (接收)</option>
          <option value="tpdo">TPDO (发送)</option>
        </select>
        <button class="btn btn-primary" @click="running ? stopMonitor() : startMonitor()">
          <span class="material-symbols-outlined" style="font-size:14px">{{ running ? 'stop' : 'play_arrow' }}</span>
          {{ running ? '停止' : '启动监控' }}
        </button>
      </div>
      <div class="toolbar-spacer" />
      <button class="btn" @click="addPdo">
        <span class="material-symbols-outlined" style="font-size:14px">add</span>
        添加PDO
      </button>
    </div>

    <div class="mapping-section">
      <div class="section-header">PDO映射配置</div>
      <table class="data-table">
        <thead>
          <tr>
            <th style="width:80px">PDO编号</th>
            <th style="width:90px">COB-ID</th>
            <th style="width:80px">传输类型</th>
            <th style="width:70px">启用</th>
            <th style="width:40px"></th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="m in mappings" :key="m.id">
            <td class="mono">PDO{{ m.pdoNumber }}</td>
            <td class="mono">{{ m.cobId }}</td>
            <td class="mono">{{ m.transType }}</td>
            <td>
              <label class="toggle-switch">
                <input type="checkbox" :checked="m.enabled" @change="togglePdoEnabled(m)" />
                <span class="toggle-slider"></span>
              </label>
            </td>
            <td>
              <button class="btn-icon" @click="removePdo(m.id)">
                <span class="material-symbols-outlined" style="font-size:14px">close</span>
              </button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="live-section">
      <div class="section-header">
        PDO数据实时显示
        <div class="toolbar-spacer" />
        <span v-if="running" class="running-indicator">
          <span class="material-symbols-outlined blink" style="font-size:14px">fiber_manual_record</span>
          实时
        </span>
      </div>
      <div class="live-content">
        <div v-for="m in mappings.filter(p => p.enabled)" :key="m.id" class="pdo-data-card">
          <div class="card-title">
            <span class="mono">PDO{{ m.pdoNumber }}</span>
            <span class="card-cobid">{{ m.cobId }}</span>
          </div>
          <div class="byte-values">
            <div v-for="(b, idx) in liveData[m.pdoNumber] || []" :key="idx" class="byte-cell">
              <span class="byte-idx">B{{ idx }}</span>
              <span class="byte-val mono">{{ b.value }}</span>
            </div>
          </div>
        </div>
        <div v-if="mappings.filter(p => p.enabled).length === 0" class="empty-hint">无启用的PDO</div>
      </div>
    </div>

    <div class="status-bar">
      <span>{{ pdoType === 'rpdo' ? 'RPDO' : 'TPDO' }}</span>
      <span>映射数: {{ mappings.length }}</span>
      <span>周期数: {{ cycleCount }}</span>
    </div>
  </div>
</template>

<style scoped>
.canopen-pdo {
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
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 6px;
}
.toolbar-label { font-size: 11px; color: var(--on-surface-variant); }
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
.select-sm {
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
}
.mono { font-family: 'JetBrains Mono', monospace; }
.btn-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.btn-icon:hover { background: rgba(244, 67, 54, 0.15); color: #f44336; }
.section-header {
  display: flex;
  align-items: center;
  padding: 6px 12px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  font-size: 12px;
  flex-shrink: 0;
}
.mapping-section {
  flex-shrink: 0;
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
}
.data-table td {
  padding: 4px 8px;
  border-bottom: 1px solid var(--outline-variant);
}
.toggle-switch {
  position: relative;
  display: inline-block;
  width: 34px;
  height: 18px;
  cursor: pointer;
}
.toggle-switch input { opacity: 0; width: 0; height: 0; }
.toggle-slider {
  position: absolute;
  inset: 0;
  background: #555;
  border-radius: 9px;
  transition: 0.2s;
}
.toggle-slider::before {
  content: '';
  position: absolute;
  width: 14px;
  height: 14px;
  left: 2px;
  bottom: 2px;
  background: white;
  border-radius: 50%;
  transition: 0.2s;
}
.toggle-switch input:checked + .toggle-slider { background: var(--primary, #2196f3); }
.toggle-switch input:checked + .toggle-slider::before { transform: translateX(16px); }
.live-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-height: 0;
}
.running-indicator {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: #4caf50;
  font-size: 11px;
  font-weight: 400;
}
.blink { animation: blink-anim 1s infinite; }
@keyframes blink-anim {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.2; }
}
.live-content {
  flex: 1;
  overflow-y: auto;
  padding: 10px 12px;
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}
.pdo-data-card {
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  overflow: hidden;
  min-width: 260px;
}
.card-title {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 10px;
  background: var(--surface-container);
  font-weight: 600;
  font-size: 12px;
}
.card-cobid {
  font-size: 10px;
  color: var(--on-surface-variant);
  font-weight: 400;
}
.byte-values {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  padding: 8px 10px;
}
.byte-cell {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  min-width: 32px;
}
.byte-idx {
  font-size: 9px;
  color: var(--on-surface-variant);
}
.byte-val {
  font-size: 13px;
  font-weight: 700;
  padding: 2px 6px;
  background: rgba(33, 150, 243, 0.08);
  border-radius: 3px;
}
.empty-hint {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 30px;
  width: 100%;
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
