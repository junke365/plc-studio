<script setup lang="ts">
import { ref, reactive, computed } from 'vue'

type NmtState = 'init' | 'preop' | 'operational' | 'stopped'

interface NmtNode {
  nodeId: number
  state: NmtState
  heartbeatActive: boolean
  deviceName: string
  lastHeartbeat: string
}

const nodes = ref<NmtNode[]>([])
const selectedNodes = ref<Set<number>>(new Set())
const scanning = ref(false)

const onlineCount = computed(() => nodes.value.filter(n => n.state !== 'init').length)
const offlineCount = computed(() => nodes.value.filter(n => n.state === 'init').length)

const stateLabels: Record<NmtState, string> = {
  init: '初始化',
  preop: '预运行',
  operational: '运行',
  stopped: '停止',
}

function generateNodes() {
  const deviceNames = ['伺服驱动器', 'I/O模块', '变频器', '传感器', 'HMI面板', '编码器', '温度控制器', '安全模块']
  nodes.value = []
  for (let i = 1; i <= 16; i++) {
    const online = Math.random() > 0.3
    const state: NmtState = online
      ? (['preop', 'operational', 'stopped'] as NmtState[])[Math.floor(Math.random() * 3)]
      : 'init'
    nodes.value.push({
      nodeId: i,
      state,
      heartbeatActive: online && Math.random() > 0.2,
      deviceName: deviceNames[Math.floor(Math.random() * deviceNames.length)],
      lastHeartbeat: online ? new Date().toLocaleTimeString('zh-CN', { hour12: false }) : '--',
    })
  }
}

function toggleNodeSelect(nodeId: number) {
  if (selectedNodes.value.has(nodeId)) {
    selectedNodes.value.delete(nodeId)
  } else {
    selectedNodes.value.add(nodeId)
  }
}

function selectAll() {
  nodes.value.forEach(n => selectedNodes.value.add(n.nodeId))
}

function deselectAll() {
  selectedNodes.value.clear()
}

function scanNodes() {
  scanning.value = true
  setTimeout(() => {
    generateNodes()
    scanning.value = false
  }, 1500)
}

function sendNmtCommand(command: 'start' | 'stop' | 'preop' | 'reset') {
  const stateMap: Record<string, NmtState> = {
    start: 'operational',
    stop: 'stopped',
    preop: 'preop',
    reset: 'init',
  }
  const labelMap: Record<string, string> = {
    start: '启动',
    stop: '停止',
    preop: '预运行',
    reset: '复位',
  }
  nodes.value.forEach(n => {
    if (selectedNodes.value.has(n.nodeId)) {
      n.state = stateMap[command]
      n.lastHeartbeat = new Date().toLocaleTimeString('zh-CN', { hour12: false })
      n.heartbeatActive = n.state !== 'init'
    }
  })
}

generateNodes()
</script>

<template>
  <div class="canopen-nmt">
    <div class="toolbar">
      <div class="toolbar-group">
        <button class="btn" @click="scanNodes" :disabled="scanning">
          <span class="material-symbols-outlined" style="font-size:14px">{{ scanning ? 'hourglass_empty' : 'radar' }}</span>
          {{ scanning ? '扫描中...' : '节点扫描' }}
        </button>
        <button class="btn" @click="selectAll">全选</button>
        <button class="btn" @click="deselectAll">取消全选</button>
      </div>
      <div class="toolbar-spacer" />
      <span class="toolbar-label">已选: {{ selectedNodes.size }} 个节点</span>
    </div>

    <div class="node-table-wrap">
      <table class="data-table">
        <thead>
          <tr>
            <th style="width:36px">
              <input type="checkbox" @change="$event.target.checked ? selectAll() : deselectAll()" />
            </th>
            <th style="width:70px">节点ID</th>
            <th style="width:80px">状态</th>
            <th style="width:80px">心跳</th>
            <th>设备名称</th>
            <th style="width:110px">最后心跳</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="node in nodes" :key="node.nodeId" :class="{ selected: selectedNodes.has(node.nodeId) }">
            <td>
              <input type="checkbox" :checked="selectedNodes.has(node.nodeId)" @change="toggleNodeSelect(node.nodeId)" />
            </td>
            <td class="mono" style="font-weight:700">{{ node.nodeId }}</td>
            <td>
              <span :class="['state-badge', node.state]">{{ stateLabels[node.state] }}</span>
            </td>
            <td>
              <span :class="['heartbeat-dot', { active: node.heartbeatActive }]"></span>
              {{ node.heartbeatActive ? '活跃' : '无' }}
            </td>
            <td>{{ node.deviceName }}</td>
            <td class="mono" style="font-size:11px">{{ node.lastHeartbeat }}</td>
          </tr>
          <tr v-if="nodes.length === 0">
            <td colspan="6" class="empty-cell">无节点数据，请先扫描</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="nmt-commands">
      <div class="section-title">NMT 命令面板</div>
      <div class="cmd-buttons">
        <button class="btn btn-success" :disabled="selectedNodes.size === 0" @click="sendNmtCommand('start')">
          <span class="material-symbols-outlined" style="font-size:14px">play_arrow</span>
          启动远程节点
        </button>
        <button class="btn btn-danger" :disabled="selectedNodes.size === 0" @click="sendNmtCommand('stop')">
          <span class="material-symbols-outlined" style="font-size:14px">stop</span>
          停止远程节点
        </button>
        <button class="btn" :disabled="selectedNodes.size === 0" @click="sendNmtCommand('preop')">
          <span class="material-symbols-outlined" style="font-size:14px">pause</span>
          进入预运行
        </button>
        <button class="btn" :disabled="selectedNodes.size === 0" @click="sendNmtCommand('reset')">
          <span class="material-symbols-outlined" style="font-size:14px">refresh</span>
          复位节点
        </button>
      </div>
    </div>

    <div class="status-bar">
      <span>在线: <strong class="count-online">{{ onlineCount }}</strong></span>
      <span>离线: <strong class="count-offline">{{ offlineCount }}</strong></span>
      <span>总计: {{ nodes.length }} 个节点</span>
    </div>
  </div>
</template>

<style scoped>
.canopen-nmt {
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
.btn:disabled { opacity: 0.4; cursor: not-allowed; }
.btn-primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-danger { background: #c62828; color: white; border-color: #c62828; }
.btn-success { background: #2e7d32; color: white; border-color: #2e7d32; }
.mono { font-family: 'JetBrains Mono', monospace; }
.node-table-wrap {
  flex: 1;
  overflow-y: auto;
  min-height: 0;
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
  padding: 5px 8px;
  border-bottom: 1px solid var(--outline-variant);
}
tr.selected { background: rgba(var(--primary-rgb, 33, 150, 243), 0.1); }
.state-badge {
  display: inline-block;
  padding: 2px 8px;
  border-radius: 10px;
  font-size: 10px;
  font-weight: 600;
}
.state-badge.init { background: rgba(158, 158, 158, 0.2); color: #9e9e9e; }
.state-badge.preop { background: rgba(255, 152, 0, 0.2); color: #ff9800; }
.state-badge.operational { background: rgba(46, 125, 50, 0.2); color: #2e7d32; }
.state-badge.stopped { background: rgba(198, 40, 40, 0.2); color: #c62828; }
.heartbeat-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #9e9e9e;
  vertical-align: middle;
  margin-right: 4px;
}
.heartbeat-dot.active {
  background: #4caf50;
  animation: pulse 1.5s infinite;
}
@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.4; }
}
.empty-cell {
  text-align: center;
  color: var(--on-surface-variant);
  padding: 40px !important;
}
.nmt-commands {
  padding: 10px 12px;
  border-top: 1px solid var(--outline-variant);
  background: var(--surface-container);
  flex-shrink: 0;
}
.section-title {
  font-weight: 600;
  font-size: 12px;
  margin-bottom: 8px;
}
.cmd-buttons {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
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
.count-online { color: #2e7d32; }
.count-offline { color: #c62828; }
</style>
