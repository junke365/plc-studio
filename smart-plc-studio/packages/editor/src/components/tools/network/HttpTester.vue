<script setup lang="ts">
import { ref, reactive } from 'vue'

interface RequestHeader {
  id: number
  key: string
  value: string
  enabled: boolean
}

interface HistoryItem {
  id: number
  method: string
  url: string
  statusCode: number
  time: number
  timestamp: string
}

const method = ref('GET')
const urlStr = ref('https://api.example.com/data')
const headers = ref<RequestHeader[]>([
  { id: 1, key: 'Content-Type', value: 'application/json', enabled: true },
  { id: 2, key: 'Authorization', value: 'Bearer token123', enabled: true },
])
const bodyType = ref<'json' | 'text'>('json')
const bodyContent = ref('{\n  "key": "value"\n}')
const sending = ref(false)
let nextHeaderId = 3

const response = ref<{
  status: number
  statusText: string
  time: number
  headers: Record<string, string>
  body: string
} | null>(null)

const history = ref<HistoryItem[]>([])
let nextHistoryId = 1

function getTimestamp(): string {
  const now = new Date()
  return now.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

function addHeader() {
  headers.value.push({
    id: nextHeaderId++,
    key: '',
    value: '',
    enabled: true,
  })
}

function removeHeader(id: number) {
  headers.value = headers.value.filter(h => h.id !== id)
}

function sendRequest() {
  sending.value = true
  const startTime = Date.now()

  setTimeout(() => {
    const elapsed = Date.now() - startTime + Math.floor(Math.random() * 100)
    const statusCodes = [200, 200, 200, 201, 204, 400, 401, 403, 404, 500]
    const status = statusCodes[Math.floor(Math.random() * statusCodes.length)]

    const respHeaders: Record<string, string> = {
      'Content-Type': 'application/json',
      'X-Request-Id': Math.random().toString(36).substring(2, 10),
      'Cache-Control': 'no-cache',
    }

    let body = ''
    if (status === 200) {
      body = JSON.stringify({
        success: true,
        data: {
          id: 12345,
          name: 'Sample Response',
          timestamp: new Date().toISOString(),
          values: [10, 20, 30],
        },
      }, null, 2)
    } else if (status === 201) {
      body = JSON.stringify({ success: true, id: 12346 }, null, 2)
    } else if (status === 404) {
      body = JSON.stringify({ error: 'Not Found', message: 'Resource does not exist' }, null, 2)
    } else {
      body = JSON.stringify({ error: `HTTP ${status}`, message: 'Request failed' }, null, 2)
    }

    response.value = {
      status,
      statusText: getStatusText(status),
      time: elapsed,
      headers: respHeaders,
      body,
    }

    history.value.unshift({
      id: nextHistoryId++,
      method: method.value,
      url: urlStr.value,
      statusCode: status,
      time: elapsed,
      timestamp: getTimestamp(),
    })
    if (history.value.length > 50) {
      history.value = history.value.slice(0, 50)
    }

    sending.value = false
  }, 200 + Math.random() * 500)
}

function getStatusText(code: number): string {
  const map: Record<number, string> = {
    200: 'OK', 201: 'Created', 204: 'No Content',
    400: 'Bad Request', 401: 'Unauthorized', 403: 'Forbidden',
    404: 'Not Found', 500: 'Internal Server Error',
  }
  return map[code] || 'Unknown'
}

function getStatusClass(code: number): string {
  if (code >= 200 && code < 300) return 'status-ok'
  if (code >= 400 && code < 500) return 'status-warn'
  return 'status-error'
}

function loadHistory(item: HistoryItem) {
  method.value = item.method
  urlStr.value = item.url
}

function clearHistory() {
  history.value = []
}

const methods = ['GET', 'POST', 'PUT', 'DELETE', 'PATCH']
</script>

<template>
  <div class="http-tester">
    <div class="toolbar">
      <div class="toolbar-group" style="flex:1">
        <select v-model="method" class="select-sm method-select">
          <option v-for="m in methods" :key="m" :value="m" :class="['method-' + m.toLowerCase()]">{{ m }}</option>
        </select>
        <input v-model="urlStr" class="input-sm" style="flex:1" placeholder="输入请求 URL..." @keydown.enter="sendRequest" />
      </div>
      <button class="btn btn-primary" @click="sendRequest" :disabled="sending">
        <span class="material-symbols-outlined" style="font-size:14px">{{ sending ? 'hourglass_empty' : 'send' }}</span>
        {{ sending ? '发送中...' : '发送' }}
      </button>
    </div>

    <div class="main-area">
      <div class="request-panel">
        <div class="tab-header">请求配置</div>
        <div class="headers-section">
          <div class="section-label">
            请求头
            <button class="btn-add" @click="addHeader">+ 添加</button>
          </div>
          <div class="header-row header-title">
            <span style="width:30px"></span>
            <span style="flex:1">Key</span>
            <span style="flex:1">Value</span>
            <span style="width:30px"></span>
          </div>
          <div v-for="h in headers" :key="h.id" class="header-row">
            <input type="checkbox" v-model="h.enabled" />
            <input v-model="h.key" class="input-sm" style="flex:1" placeholder="Header name" />
            <input v-model="h.value" class="input-sm" style="flex:1" placeholder="Header value" />
            <button class="btn-icon" @click="removeHeader(h.id)">
              <span class="material-symbols-outlined" style="font-size:12px">close</span>
            </button>
          </div>
        </div>
        <div class="body-section">
          <div class="section-label">请求体</div>
          <div class="body-type-row">
            <label class="radio-label"><input type="radio" v-model="bodyType" value="json" /> JSON</label>
            <label class="radio-label"><input type="radio" v-model="bodyType" value="text" /> Text</label>
          </div>
          <textarea v-model="bodyContent" class="body-textarea" rows="8"></textarea>
        </div>
      </div>

      <div class="response-panel">
        <div v-if="response" class="response-content">
          <div class="resp-status-bar">
            <span :class="['status-code', getStatusClass(response.status)]">{{ response.status }}</span>
            <span class="status-text">{{ response.statusText }}</span>
            <span class="resp-time">{{ response.time }}ms</span>
          </div>
          <div class="resp-section">
            <div class="resp-section-title">响应头</div>
            <div class="resp-headers">
              <div v-for="(val, key) in response.headers" :key="key" class="resp-header-row">
                <span class="header-key mono">{{ key }}:</span>
                <span class="header-val mono">{{ val }}</span>
              </div>
            </div>
          </div>
          <div class="resp-section resp-body-section">
            <div class="resp-section-title">响应体</div>
            <pre class="resp-body mono">{{ response.body }}</pre>
          </div>
        </div>
        <div v-else class="empty-response">
          <span class="material-symbols-outlined" style="font-size:48px; color: var(--on-surface-variant);">send</span>
          <span>发送请求查看响应</span>
        </div>
      </div>
    </div>

    <div class="history-panel">
      <div class="section-label" style="padding:4px 8px">请求历史 <button class="btn-add" @click="clearHistory">清空</button></div>
      <div class="history-list">
        <div v-for="item in history" :key="item.id" class="history-item" @click="loadHistory(item)">
          <span :class="['method-badge', item.method.toLowerCase()]">{{ item.method }}</span>
          <span class="hist-url mono" style="flex:1">{{ item.url }}</span>
          <span :class="['hist-status', getStatusClass(item.statusCode)]">{{ item.statusCode }}</span>
          <span class="hist-time mono">{{ item.time }}ms</span>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.http-tester {
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
.method-select { min-width: 80px; }
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
.btn-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.btn-icon:hover { background: rgba(244, 67, 54, 0.15); color: #f44336; }
.btn-add {
  border: none;
  background: none;
  color: var(--primary);
  cursor: pointer;
  font-size: 11px;
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
.input-sm:focus { border-color: var(--primary); outline: none; }
.mono { font-family: 'JetBrains Mono', monospace; }
.main-area {
  flex: 1;
  display: flex;
  overflow: hidden;
  min-height: 0;
}
.request-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  border-right: 1px solid var(--outline-variant);
}
.response-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.tab-header {
  padding: 6px 10px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  font-weight: 600;
  font-size: 12px;
}
.section-label {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 10px;
  font-weight: 600;
  font-size: 11px;
  color: var(--on-surface-variant);
}
.headers-section {
  overflow-y: auto;
  max-height: 180px;
}
.header-row {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px 10px;
}
.header-title { font-size: 10px; color: var(--on-surface-variant); }
.body-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-height: 0;
}
.body-type-row {
  display: flex;
  gap: 12px;
  padding: 4px 10px;
}
.radio-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  cursor: pointer;
}
.body-textarea {
  flex: 1;
  margin: 4px 10px;
  padding: 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  border-radius: 4px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  resize: none;
  outline: none;
}
.body-textarea:focus { border-color: var(--primary); }
.response-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.resp-status-bar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 10px;
  border-bottom: 1px solid var(--outline-variant);
}
.status-code {
  font-weight: 700;
  font-size: 14px;
  font-family: 'JetBrains Mono', monospace;
}
.status-ok { color: #2e7d32; }
.status-warn { color: #ff9800; }
.status-error { color: #c62828; }
.status-text { font-size: 12px; color: var(--on-surface-variant); }
.resp-time {
  margin-left: auto;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  color: var(--on-surface-variant);
}
.resp-section {
  border-bottom: 1px solid var(--outline-variant);
}
.resp-section-title {
  padding: 4px 10px;
  font-weight: 600;
  font-size: 11px;
  color: var(--on-surface-variant);
  background: var(--surface-container);
}
.resp-headers { padding: 4px 10px; }
.resp-header-row {
  display: flex;
  gap: 6px;
  font-size: 11px;
  padding: 2px 0;
}
.header-key { color: var(--primary); font-weight: 600; }
.header-val { color: var(--on-surface); }
.resp-body-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.resp-body {
  flex: 1;
  overflow-y: auto;
  padding: 8px 10px;
  margin: 0;
  font-size: 11px;
  white-space: pre-wrap;
  word-break: break-all;
  color: var(--on-surface);
}
.empty-response {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  color: var(--on-surface-variant);
}
.history-panel {
  max-height: 120px;
  overflow-y: auto;
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
}
.history-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 8px;
  cursor: pointer;
  border-bottom: 1px solid var(--outline-variant);
  font-size: 11px;
}
.history-item:hover { background: var(--surface-variant); }
.method-badge {
  padding: 1px 6px;
  border-radius: 3px;
  font-size: 9px;
  font-weight: 700;
}
.method-badge.get { background: rgba(76, 175, 80, 0.15); color: #4caf50; }
.method-badge.post { background: rgba(33, 150, 243, 0.15); color: #2196f3; }
.method-badge.put { background: rgba(255, 152, 0, 0.15); color: #ff9800; }
.method-badge.delete { background: rgba(244, 67, 54, 0.15); color: #f44336; }
.method-badge.patch { background: rgba(156, 39, 176, 0.15); color: #9c27b0; }
.hist-url {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.hist-status { font-weight: 600; font-family: 'JetBrains Mono', monospace; }
.hist-time { color: var(--on-surface-variant); }
</style>
