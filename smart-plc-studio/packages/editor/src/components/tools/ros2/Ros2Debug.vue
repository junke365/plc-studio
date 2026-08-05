<template>
  <div class="ros2-debug">
    <div class="toolbar">
      <div class="toolbar-group">
        <span class="status-dot" :class="status.strategy"></span>
        <span class="strategy-badge" :class="status.strategy">{{ strategyLabel }}</span>
        <span class="toolbar-label" v-if="status.rosDistro">{{ status.rosDistro }}</span>
        <span class="toolbar-label ws-state" :class="{ online: wsConnected }">
          {{ wsConnected ? 'WS 在线' : 'WS 断开' }}
        </span>
      </div>
      <div class="toolbar-group">
        <button class="btn" @click="refreshAll">
          <span class="material-symbols-outlined">refresh</span>
          刷新
        </button>
        <button class="btn" @click="clearLog">
          <span class="material-symbols-outlined">delete</span>
          清空日志
        </button>
      </div>
    </div>

    <div class="status-card">
      <div class="status-grid">
        <div class="status-item"><span class="k">策略</span><span class="v">{{ status.strategy }}</span></div>
        <div class="status-item"><span class="k">发行版</span><span class="v">{{ status.rosDistro || '-' }}</span></div>
        <div class="status-item"><span class="k">安装路径</span><span class="v" :title="status.rosPrefix">{{ status.rosPrefix || '-' }}</span></div>
        <div class="status-item"><span class="k">ros2 CLI</span><span class="v">{{ status.ros2Cli ? '可用' : '不可用' }}</span></div>
        <div class="status-item"><span class="k">rclnodejs</span><span class="v">{{ status.rclnodejs ? '可用' : '不可用' }}</span></div>
        <div class="status-item"><span class="k">桥接地址</span><span class="v" :title="status.bridgeUrl">{{ status.bridgeUrl || '-' }}</span></div>
      </div>
      <div class="status-desc">{{ status.description }}</div>
    </div>

    <div class="tabs">
      <div
        v-for="t in tabList"
        :key="t.id"
        :class="['tab', { active: activeTab === t.id }]"
        @click="switchTab(t.id)"
      >
        {{ t.label }}
      </div>
    </div>

    <div class="tab-content">
      <!-- 节点 -->
      <div v-if="activeTab === 'nodes'" class="list-pane">
        <div v-if="nodesError" class="empty-hint">{{ nodesError }}</div>
        <div v-else-if="nodes.length === 0" class="empty-hint">无节点</div>
        <div v-else class="item-list">
          <div v-for="n in nodes" :key="n" class="list-item">
            <span class="material-symbols-outlined">memory</span>
            <span class="item-name">{{ n }}</span>
          </div>
        </div>
      </div>

      <!-- 话题 -->
      <div v-if="activeTab === 'topics'" class="list-pane">
        <div v-if="topicsError" class="empty-hint">{{ topicsError }}</div>
        <div v-else-if="topics.length === 0" class="empty-hint">无话题</div>
        <div v-else class="item-list">
          <div v-for="t in topics" :key="t.name" class="list-item">
            <span class="material-symbols-outlined">rss_feed</span>
            <span class="item-name">{{ t.name }}</span>
            <span class="item-types">{{ t.types.join(', ') }}</span>
            <button
              class="btn btn-small"
              :class="{ subscribed: subscribedTopics.has(t.name) }"
              @click="toggleSubscribe(t.name)"
            >
              {{ subscribedTopics.has(t.name) ? '取消订阅' : '订阅' }}
            </button>
          </div>
        </div>
      </div>

      <!-- 服务 -->
      <div v-if="activeTab === 'services'" class="list-pane">
        <div v-if="servicesError" class="empty-hint">{{ servicesError }}</div>
        <div v-else-if="services.length === 0" class="empty-hint">无服务</div>
        <div v-else class="item-list">
          <div v-for="s in services" :key="s" class="list-item">
            <span class="material-symbols-outlined">call_split</span>
            <span class="item-name">{{ s }}</span>
            <button class="btn btn-small" @click="pickService(s)">调用</button>
          </div>
        </div>
        <div v-if="serviceForm.open" class="form-pane">
          <div class="form-title">调用服务 <span class="mono">{{ serviceForm.name }}</span></div>
          <div class="form-row">
            <label>类型</label>
            <input v-model="serviceForm.type" class="input-field mono" placeholder="std_srvs/srv/Trigger" />
          </div>
          <div class="form-row">
            <label>请求(JSON)</label>
            <textarea v-model="serviceForm.request" class="input-field textarea mono" rows="4" placeholder="{}"></textarea>
          </div>
          <div class="form-row">
            <button class="btn btn-primary" @click="callService">调用</button>
            <button class="btn" @click="serviceForm.open = false">关闭</button>
          </div>
          <div v-if="serviceResult" class="result-box mono">{{ serviceResult }}</div>
        </div>
      </div>

      <!-- 动作 -->
      <div v-if="activeTab === 'actions'" class="list-pane">
        <div v-if="actionsError" class="empty-hint">{{ actionsError }}</div>
        <div v-else-if="actions.length === 0" class="empty-hint">无动作</div>
        <div v-else class="item-list">
          <div v-for="a in actions" :key="a" class="list-item">
            <span class="material-symbols-outlined">directions_run</span>
            <span class="item-name">{{ a }}</span>
          </div>
        </div>
      </div>

      <!-- 订阅实时数据 -->
      <div v-if="activeTab === 'subscribe'" class="list-pane">
        <div v-if="subscribedTopics.size === 0" class="empty-hint">
          未订阅任何话题。请到「话题」页签点击订阅，实时数据将显示在这里。
        </div>
        <div v-for="topic in [...subscribedTopics]" :key="topic" class="sub-block">
          <div class="sub-head">
            <span class="mono sub-topic">{{ topic }}</span>
            <span class="sub-count">已收 {{ subCount(topic) }} 条</span>
            <button class="btn btn-small" @click="toggleSubscribe(topic)">取消订阅</button>
          </div>
          <div class="sub-msgs mono">
            <div v-for="(m, i) in messages[topic] || []" :key="i" class="sub-msg">
              <span class="sub-time">{{ m.time }}</span>
              <span class="sub-payload">{{ prettyPayload(m.payload) }}</span>
            </div>
            <div v-if="(messages[topic] || []).length === 0" class="empty-hint">等待消息...</div>
          </div>
        </div>
      </div>

      <!-- 发布 -->
      <div v-if="activeTab === 'publish'" class="list-pane">
        <div class="form-pane">
          <div class="form-title">发布话题消息</div>
          <div class="form-row">
            <label>话题</label>
            <select v-model="publishForm.topic" class="input-field">
              <option value="" disabled>选择话题</option>
              <option v-for="t in topics" :key="t.name" :value="t.name">{{ t.name }}</option>
            </select>
          </div>
          <div class="form-row">
            <label>类型</label>
            <input v-model="publishForm.type" class="input-field mono" placeholder="std_msgs/msg/String" />
          </div>
          <div class="form-row">
            <label>消息(JSON)</label>
            <textarea v-model="publishForm.message" class="input-field textarea mono" rows="4" placeholder='{"data": "hello"}'></textarea>
          </div>
          <div class="form-row">
            <button class="btn btn-primary" @click="doPublish">发布</button>
          </div>
          <div v-if="publishResult" class="result-box mono">{{ publishResult }}</div>
        </div>
      </div>

      <!-- 日志 -->
      <div v-if="activeTab === 'log'" class="list-pane log-pane">
        <div v-if="logLines.length === 0" class="empty-hint">暂无日志</div>
        <div v-for="(l, i) in logLines" :key="i" :class="['log-line', l.level]">
          <span class="log-time">{{ l.time }}</span>
          <span class="log-text">{{ l.text }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import {
  ROS2_WS_URL,
  getRos2Status,
  getRos2Nodes,
  getRos2Topics,
  getRos2Services,
  getRos2Actions,
  ros2Publish,
  ros2Call,
  type Ros2Status,
  type Ros2Topic,
} from '@/api/ros2'

interface LogLine { time: string; level: 'info' | 'warn' | 'error'; text: string }
interface SubMessage { time: string; payload: unknown }

const tabList = [
  { id: 'nodes', label: '节点' },
  { id: 'topics', label: '话题' },
  { id: 'services', label: '服务' },
  { id: 'actions', label: '动作' },
  { id: 'subscribe', label: '实时订阅' },
  { id: 'publish', label: '发布' },
  { id: 'log', label: '日志' },
]

const activeTab = ref('nodes')
const status = ref<Ros2Status>({ strategy: 'none', rosDistro: null, rosPrefix: null, ros2Cli: false, rclnodejs: false, bridgeUrl: null, description: '' })
const nodes = ref<string[]>([])
const topics = ref<Ros2Topic[]>([])
const services = ref<string[]>([])
const actions = ref<string[]>([])
const nodesError = ref('')
const topicsError = ref('')
const servicesError = ref('')
const actionsError = ref('')
const logLines = ref<LogLine[]>([])
const subscribedTopics = ref<Set<string>>(new Set())
const messages = ref<Record<string, SubMessage[]>>({})
const wsConnected = ref(false)

const serviceForm = ref<{ open: boolean; name: string; type: string; request: string }>({
  open: false, name: '', type: '', request: '{}',
})
const serviceResult = ref('')
const publishForm = ref<{ topic: string; type: string; message: string }>({
  topic: '', type: '', message: '{}',
})
const publishResult = ref('')

const strategyLabel = computed(() => {
  switch (status.value.strategy) {
    case 'rclnodejs': return '原生节点 rclnodejs'
    case 'cli': return 'CLI 通道'
    case 'bridge': return '远程桥接'
    default: return '未连接'
  }
})

function now(): string {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(new Date().getMilliseconds()).padStart(3, '0')
}

function log(level: 'info' | 'warn' | 'error', text: string) {
  logLines.value.push({ time: now(), level, text })
  if (logLines.value.length > 500) logLines.value.splice(0, logLines.value.length - 500)
}

function clearLog() { logLines.value = [] }

function subCount(topic: string): number {
  return (messages.value[topic] || []).length
}

function prettyPayload(payload: unknown): string {
  if (typeof payload === 'string') return payload
  try { return JSON.stringify(payload) } catch { return String(payload) }
}

// ===== REST 数据加载 =====
async function refreshStatus() {
  try {
    status.value = await getRos2Status()
  } catch (err: any) {
    log('error', `状态获取失败: ${err?.message ?? err}`)
  }
}

async function refreshNodes() {
  const r = await getRos2Nodes()
  if (r.success) { nodes.value = r.data || []; nodesError.value = '' }
  else { nodes.value = []; nodesError.value = r.error || '加载失败' }
}

async function refreshTopics() {
  const r = await getRos2Topics()
  if (r.success) { topics.value = r.data || []; topicsError.value = '' }
  else { topics.value = []; topicsError.value = r.error || '加载失败' }
}

async function refreshServices() {
  const r = await getRos2Services()
  if (r.success) { services.value = r.data || []; servicesError.value = '' }
  else { services.value = []; servicesError.value = r.error || '加载失败' }
}

async function refreshActions() {
  const r = await getRos2Actions()
  if (r.success) { actions.value = r.data || []; actionsError.value = '' }
  else { actions.value = []; actionsError.value = r.error || '加载失败' }
}

async function refreshAll() {
  await refreshStatus()
  await Promise.all([refreshNodes(), refreshTopics(), refreshServices(), refreshActions()])
}

function switchTab(id: string) {
  activeTab.value = id
  if (id === 'nodes') refreshNodes()
  if (id === 'topics') refreshTopics()
  if (id === 'services') refreshServices()
  if (id === 'actions') refreshActions()
}

// ===== WebSocket 实时订阅 =====
let ws: WebSocket | null = null
let reconnectTimer: ReturnType<typeof setTimeout> | null = null
let alive = false

function wsSend(obj: unknown) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(obj))
  }
}

function connectWs() {
  if (!alive) return
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return
  try {
    ws = new WebSocket(ROS2_WS_URL)
  } catch (err: any) {
    log('error', `WS 连接失败: ${err?.message ?? err}`)
    return
  }
  ws.onopen = () => {
    wsConnected.value = true
    log('info', 'WebSocket 已连接')
    for (const topic of subscribedTopics.value) wsSend({ type: 'subscribe', topic })
  }
  ws.onmessage = (e) => {
    try {
      const { event, data } = JSON.parse(e.data)
      if (event === 'status') {
        wsConnected.value = true
      } else if (event === 'subscribed') {
        log('info', `已订阅 ${data}`)
      } else if (event === 'error') {
        log('error', `WS: ${data}`)
      } else if (event === 'message') {
        const { topic, payload } = data || {}
        if (topic && messages.value[topic]) {
          messages.value[topic].push({ time: now(), payload })
          if (messages.value[topic].length > 200) messages.value[topic].splice(0, messages.value[topic].length - 200)
        }
      }
    } catch {}
  }
  ws.onclose = () => {
    wsConnected.value = false
    ws = null
    if (alive && !reconnectTimer) {
      reconnectTimer = setTimeout(() => { reconnectTimer = null; connectWs() }, 2000)
    }
  }
  ws.onerror = () => { /* onclose 会触发重连 */ }
}

function toggleSubscribe(topic: string) {
  if (subscribedTopics.value.has(topic)) {
    subscribedTopics.value.delete(topic)
    wsSend({ type: 'unsubscribe', topic })
    log('info', `已取消订阅 ${topic}`)
  } else {
    subscribedTopics.value.add(topic)
    if (!messages.value[topic]) messages.value[topic] = []
    wsSend({ type: 'subscribe', topic })
    log('info', `请求订阅 ${topic}`)
  }
}

// ===== 发布 =====
async function doPublish() {
  if (!publishForm.value.topic || !publishForm.value.type) {
    publishResult.value = '请选择话题并填写类型'
    return
  }
  let message: Record<string, unknown>
  try {
    message = JSON.parse(publishForm.value.message || '{}')
  } catch (err: any) {
    publishResult.value = `消息 JSON 解析失败: ${err?.message ?? err}`
    return
  }
  const r = await ros2Publish(publishForm.value.topic, publishForm.value.type, message)
  if (r.success) {
    publishResult.value = `已发布到 ${publishForm.value.topic}`
    log('info', `发布 ${publishForm.value.topic} [${publishForm.value.type}]`)
  } else {
    publishResult.value = `发布失败: ${r.error}`
    log('error', `发布 ${publishForm.value.topic} 失败: ${r.error}`)
  }
}

// ===== 调用服务 =====
function pickService(name: string) {
  serviceForm.value = { open: true, name, type: '', request: '{}' }
  serviceResult.value = ''
  const s = services.value.find((x) => x === name)
  if (s && s.includes('/')) {
    const base = s.split('/')[0]
    if (base === 'spawn_entity') serviceForm.value.type = 'gazebo_msgs/srv/SpawnEntity'
  }
}

async function callService() {
  if (!serviceForm.value.name || !serviceForm.value.type) {
    serviceResult.value = '请填写服务名与类型'
    return
  }
  let request: Record<string, unknown>
  try {
    request = JSON.parse(serviceForm.value.request || '{}')
  } catch (err: any) {
    serviceResult.value = `请求 JSON 解析失败: ${err?.message ?? err}`
    return
  }
  const r = await ros2Call(serviceForm.value.name, serviceForm.value.type, request)
  serviceResult.value = r.success ? String(r.data ?? '调用成功') : `调用失败: ${r.error}`
  log(r.success ? 'info' : 'error', `调用服务 ${serviceForm.value.name} ${r.success ? '成功' : '失败'}`)
}

onMounted(() => {
  alive = true
  refreshAll()
  connectWs()
})

onUnmounted(() => {
  alive = false
  if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null }
  try { ws?.close() } catch {}
  ws = null
})
</script>

<style scoped>
.ros2-debug {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--surface);
}
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
  gap: 8px;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 8px;
}
.toolbar-label {
  font-size: 12px;
  color: var(--on-surface-variant);
}
.ws-state {
  padding: 2px 8px;
  border-radius: 10px;
  background: var(--surface-variant);
}
.ws-state.online {
  color: #4caf50;
}
.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: #888;
}
.status-dot.none { background: #c62828; }
.status-dot.cli { background: #1e88e5; }
.status-dot.rclnodejs { background: #4caf50; }
.status-dot.bridge { background: #fb8c00; }
.strategy-badge {
  font-size: 12px;
  font-weight: 600;
  padding: 2px 10px;
  border-radius: 10px;
  color: #fff;
  background: #888;
}
.strategy-badge.none { background: #c62828; }
.strategy-badge.cli { background: #1e88e5; }
.strategy-badge.rclnodejs { background: #4caf50; }
.strategy-badge.bridge { background: #fb8c00; }
.status-card {
  padding: 8px 12px;
  border-bottom: 1px solid var(--outline-variant);
  background: var(--surface-container);
}
.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 4px 16px;
}
.status-item {
  display: flex;
  gap: 6px;
  font-size: 12px;
}
.status-item .k {
  color: var(--on-surface-variant);
  flex-shrink: 0;
}
.status-item .v {
  color: var(--on-surface);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.status-desc {
  margin-top: 4px;
  font-size: 11px;
  color: var(--on-surface-variant);
}
.tabs {
  display: flex;
  gap: 2px;
  padding: 6px 12px 0;
  border-bottom: 1px solid var(--outline-variant);
}
.tab {
  padding: 6px 14px;
  font-size: 12px;
  cursor: pointer;
  color: var(--on-surface-variant);
  border-radius: 6px 6px 0 0;
  user-select: none;
}
.tab.active {
  color: var(--primary);
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-bottom-color: var(--surface-container);
}
.tab-content {
  flex: 1;
  overflow-y: auto;
}
.list-pane {
  padding: 8px 12px;
}
.item-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.list-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 10px;
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  background: var(--surface-container);
  font-size: 12px;
}
.list-item .material-symbols-outlined {
  font-size: 16px;
  color: var(--on-surface-variant);
}
.item-name {
  font-family: 'JetBrains Mono', monospace;
  flex-shrink: 0;
}
.item-types {
  color: var(--on-surface-variant);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  flex: 1;
}
.empty-hint {
  padding: 24px;
  text-align: center;
  color: var(--on-surface-variant);
  font-size: 12px;
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
.btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.btn .material-symbols-outlined {
  font-size: 14px;
}
.btn-small {
  padding: 2px 8px;
  font-size: 11px;
}
.btn-primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}
.btn.subscribed {
  background: #c62828;
  color: #fff;
  border-color: #c62828;
}
.form-pane {
  margin-top: 8px;
  padding: 12px;
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  background: var(--surface-container);
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.form-title {
  font-size: 13px;
  font-weight: 600;
}
.form-title .mono {
  font-weight: 400;
  color: var(--primary);
}
.form-row {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.form-row label {
  font-size: 11px;
  color: var(--on-surface-variant);
}
.input-field {
  background: var(--surface);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  padding: 6px 10px;
  font-size: 12px;
  outline: none;
}
.input-field:focus {
  border-color: var(--primary);
}
.textarea {
  resize: vertical;
  font-family: 'JetBrains Mono', monospace;
}
.mono {
  font-family: 'JetBrains Mono', monospace;
}
.result-box {
  padding: 8px;
  background: #1e1e1e;
  color: #4caf50;
  border-radius: 4px;
  font-size: 12px;
  white-space: pre-wrap;
  word-break: break-all;
  max-height: 200px;
  overflow-y: auto;
}
.sub-block {
  margin-bottom: 12px;
}
.sub-head {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px 10px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 6px 6px 0 0;
  font-size: 12px;
}
.sub-topic {
  color: var(--primary);
  font-weight: 600;
}
.sub-count {
  color: var(--on-surface-variant);
  font-size: 11px;
  flex: 1;
}
.sub-msgs {
  border: 1px solid var(--outline-variant);
  border-top: none;
  border-radius: 0 0 6px 6px;
  background: #1e1e1e;
  max-height: 260px;
  overflow-y: auto;
  font-size: 12px;
}
.sub-msg {
  display: flex;
  gap: 10px;
  padding: 3px 10px;
  border-bottom: 1px solid #2a2a2a;
}
.sub-time {
  color: #666;
  flex-shrink: 0;
}
.sub-payload {
  color: #4caf50;
  word-break: break-all;
  white-space: pre-wrap;
}
.log-pane {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.log-line {
  display: flex;
  gap: 10px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
  padding: 2px 0;
}
.log-line .log-time {
  color: #666;
  flex-shrink: 0;
}
.log-line.info .log-text { color: var(--on-surface); }
.log-line.warn .log-text { color: #fb8c00; }
.log-line.error .log-text { color: #e53935; }
</style>
