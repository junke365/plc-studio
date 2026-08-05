<template>
  <Teleport to="body">
    <div v-if="visible" class="modal-overlay" @click.self="handleCancel">
      <div class="modal-dialog">
        <!-- 标题 -->
        <div class="modal-header">
          <span class="material-symbols-outlined">settings</span>
          <span>仿真调试设置</span>
          <button class="modal-close" @click="handleCancel">
            <span class="material-symbols-outlined">close</span>
          </button>
        </div>

        <!-- 模式选择 -->
        <div class="modal-body">
          <!-- 运行时目标 -->
          <div class="section">
            <div class="section-title">运行时目标</div>
            <div class="target-tabs">
              <button
                v-for="target in runtimeTargets"
                :key="target.value"
                class="target-tab"
                :class="{ active: localRuntimeTarget === target.value }"
                @click="localRuntimeTarget = target.value"
              >
                <span class="material-symbols-outlined">{{ target.icon }}</span>
                <span>{{ target.label }}</span>
              </button>
            </div>
          </div>

          <div class="section">
            <div class="section-title">通信方式</div>
            <div class="mode-tabs">
              <button
                v-for="mode in modes"
                :key="mode.value"
                class="mode-tab"
                :class="{ active: localDebugMode === mode.value }"
                @click="localDebugMode = mode.value"
              >
                <span class="material-symbols-outlined">{{ mode.icon }}</span>
                <span>{{ mode.label }}</span>
              </button>
            </div>
          </div>

          <!-- UART 配置 -->
          <div v-if="localDebugMode === 'uart'" class="section">
            <div class="section-title">UART 串口参数</div>
            <div class="form-grid">
              <div class="form-row">
                <label>端口</label>
                <div class="input-group">
                  <select v-model="localUart.port" class="input input-select">
                    <option v-if="portList.length === 0" value="" disabled>未检测到串口</option>
                    <option v-for="p in portList" :key="p.path" :value="p.path">
                      {{ p.path }}{{ p.manufacturer ? ' — ' + p.manufacturer : '' }}
                    </option>
                  </select>
                  <button class="btn-icon-refresh" :disabled="loadingPorts" @click="refreshPorts" title="刷新串口列表">
                    <span class="material-symbols-outlined">{{ loadingPorts ? 'sync' : 'refresh' }}</span>
                  </button>
                </div>
              </div>
              <div class="form-row">
                <label>波特率</label>
                <select v-model.number="localUart.baudRate" class="input">
                  <option v-for="br in baudRates" :key="br" :value="br">{{ br }}</option>
                </select>
              </div>
              <div class="form-row">
                <label>数据位</label>
                <select v-model.number="localUart.dataBits" class="input">
                  <option v-for="db in [5, 6, 7, 8]" :key="db" :value="db">{{ db }}</option>
                </select>
              </div>
              <div class="form-row">
                <label>停止位</label>
                <select v-model.number="localUart.stopBits" class="input">
                  <option v-for="sb in [1, 1.5, 2]" :key="sb" :value="sb">{{ sb }}</option>
                </select>
              </div>
              <div class="form-row">
                <label>校验位</label>
                <select v-model="localUart.parity" class="input">
                  <option v-for="p in parityOptions" :key="p.value" :value="p.value">{{ p.label }}</option>
                </select>
              </div>
            </div>
          </div>

          <!-- TCP 配置 -->
          <div v-if="localDebugMode === 'tcp'" class="section">
            <div class="section-title">TCP 参数</div>
            <div class="form-grid">
              <div class="form-row">
                <label>目标地址</label>
                <input v-model="localTcp.host" class="input" placeholder="127.0.0.1" />
              </div>
              <div class="form-row">
                <label>端口</label>
                <input v-model.number="localTcp.port" type="number" class="input" min="1" max="65535" />
              </div>
              <div class="form-row">
                <label>超时(ms)</label>
                <input v-model.number="localTcp.timeout" type="number" class="input" min="100" step="100" />
              </div>
            </div>
          </div>

          <!-- UDP 配置 -->
          <div v-if="localDebugMode === 'udp'" class="section">
            <div class="section-title">UDP 参数</div>
            <div class="form-grid">
              <div class="form-row">
                <label>本地端口</label>
                <input v-model.number="localUdp.localPort" type="number" class="input" min="0" max="65535" />
              </div>
              <div class="form-row">
                <label>远程地址</label>
                <input v-model="localUdp.remoteHost" class="input" placeholder="127.0.0.1" />
              </div>
              <div class="form-row">
                <label>远程端口</label>
                <input v-model.number="localUdp.remotePort" type="number" class="input" min="1" max="65535" />
              </div>
              <div class="form-row">
                <label>广播</label>
                <label class="checkbox-label">
                  <input type="checkbox" v-model="localUdp.broadcast" />
                  <span>启用广播</span>
                </label>
              </div>
            </div>
          </div>
        </div>

        <!-- 底部按钮 -->
        <div class="modal-footer">
          <button class="btn btn-text" @click="handleCancel">取消</button>
          <button class="btn btn-primary" @click="handleSave">保存</button>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import { ref, reactive, watch, onMounted } from "vue"
import { serialListPorts } from "@/serial/serialClient"
import type { PortInfo } from "@/serial/serialClient"
import { useSettingsStore, type DebugMode, type RuntimeTarget, type UartSettings, type TcpSettings, type UdpSettings } from "@/stores/settings"

const props = defineProps<{ visible: boolean }>()
const emit = defineEmits<{ (e: "update:visible", v: boolean): void; (e: "saved"): void }>()

const store = useSettingsStore()

const modes = [
  { value: "uart" as DebugMode, icon: "cable", label: "UART 串口" },
  { value: "tcp" as DebugMode, icon: "lan", label: "TCP" },
  { value: "udp" as DebugMode, icon: "wifi", label: "UDP" },
]

const baudRates = [1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
const parityOptions = [
  { value: "none", label: "无" },
  { value: "odd", label: "奇校验" },
  { value: "even", label: "偶校验" },
  { value: "mark", label: "标记校验" },
  { value: "space", label: "空格校验" },
] as const

const runtimeTargets = [
  { value: "stm32" as RuntimeTarget, icon: "memory", label: "STM32" },
  { value: "esp32" as RuntimeTarget, icon: "wifi", label: "ESP32" },
  { value: "linux-arm" as RuntimeTarget, icon: "developer_board", label: "Linux ARM" },
  { value: "linux-x86" as RuntimeTarget, icon: "dns", label: "Linux x86" },
]

// 本地副本，取消时恢复
const localDebugMode = ref<DebugMode>(store.debugMode)
const localRuntimeTarget = ref<RuntimeTarget>(store.runtimeTarget)
const localUart = reactive<UartSettings>({ ...store.uart })
const localTcp = reactive<TcpSettings>({ ...store.tcp })
const localUdp = reactive<UdpSettings>({ ...store.udp })

const portList = ref<PortInfo[]>([])
const loadingPorts = ref(false)

async function refreshPorts() {
  loadingPorts.value = true
  try {
    portList.value = await serialListPorts()
    if (portList.value.length > 0 && !portList.value.some((p) => p.path === localUart.port)) {
      localUart.port = portList.value[0].path
    } else if (portList.value.length === 0) {
      localUart.port = ""
    }
  } catch {
    portList.value = []
  } finally {
    loadingPorts.value = false
  }
}

watch(() => props.visible, (v) => {
  if (v) {
    localDebugMode.value = store.debugMode
    localRuntimeTarget.value = store.runtimeTarget
    Object.assign(localUart, store.uart)
    Object.assign(localTcp, store.tcp)
    Object.assign(localUdp, store.udp)
    refreshPorts()
  }
})

function handleSave() {
  store.setDebugMode(localDebugMode.value)
  store.setRuntimeTarget(localRuntimeTarget.value)
  Object.assign(store.uart, localUart)
  Object.assign(store.tcp, localTcp)
  Object.assign(store.udp, localUdp)
  emit("saved")
  emit("update:visible", false)
}

function handleCancel() {
  emit("update:visible", false)
}
</script>

<style scoped>
.modal-overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(2px);
}

.modal-dialog {
  width: 520px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  background: var(--surface-container-high);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg);
  box-shadow: 0 16px 48px rgba(0, 0, 0, 0.5);
}

.modal-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 14px 16px;
  font-size: 14px;
  font-weight: 600;
  color: var(--on-surface);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.modal-header .material-symbols-outlined {
  font-size: 20px;
  color: var(--primary);
}

.modal-close {
  margin-left: auto;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
}

.modal-close:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.modal-close .material-symbols-outlined {
  font-size: 18px;
  color: inherit;
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 12px 16px;
}

.section {
  margin-bottom: 16px;
}

.section-title {
  font-size: 11px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--on-surface-variant);
  margin-bottom: 8px;
}

.mode-tabs {
  display: flex;
  gap: 6px;
}

.mode-tab {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 10px 12px;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  background: var(--surface);
  color: var(--on-surface-variant);
  font-size: 12px;
  cursor: pointer;
  transition: all 0.15s;
}

.mode-tab:hover {
  border-color: var(--primary);
  color: var(--on-surface);
}

.mode-tab.active {
  border-color: var(--primary);
  background: var(--primary-container);
  color: var(--on-primary);
}

.mode-tab .material-symbols-outlined {
  font-size: 18px;
}

.target-tabs {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.target-tab {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 8px 12px;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  background: var(--surface);
  color: var(--on-surface-variant);
  font-size: 12px;
  cursor: pointer;
  transition: all 0.15s;
}

.target-tab:hover {
  border-color: var(--primary);
  color: var(--on-surface);
}

.target-tab.active {
  border-color: var(--primary);
  background: var(--primary-container);
  color: var(--on-primary);
}

.target-tab .material-symbols-outlined {
  font-size: 16px;
}

.form-grid {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.form-row {
  display: flex;
  align-items: center;
  gap: 10px;
}

.form-row > label {
  flex: 0 0 80px;
  font-size: 12px;
  color: var(--on-surface-variant);
  text-align: right;
}

.input {
  flex: 1;
  padding: 6px 10px;
  background: var(--surface);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  font-family: "JetBrains Mono", monospace;
  outline: none;
  transition: border-color 0.15s;
}

.input:focus {
  border-color: var(--primary);
}

select.input {
  font-family: "Inter", sans-serif;
  cursor: pointer;
  appearance: auto;
}

.checkbox-label {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: var(--on-surface);
  cursor: pointer;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  padding: 12px 16px;
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.btn {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 6px 16px;
  border-radius: var(--radius);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s;
  border: 1px solid transparent;
}

.btn-text {
  background: none;
  border-color: var(--outline-variant);
  color: var(--on-surface-variant);
}

.btn-text:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.btn-primary {
  background: var(--primary);
  border-color: var(--primary);
  color: var(--on-primary);
}

.btn-primary:hover {
  filter: brightness(1.1);
}

.input-group {
  flex: 1;
  display: flex;
  gap: 4px;
}

.input-select {
  flex: 1;
}

.btn-icon-refresh {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 30px;
  min-width: 30px;
  background: var(--surface);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: all 0.15s;
}

.btn-icon-refresh:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.btn-icon-refresh:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.btn-icon-refresh .material-symbols-outlined {
  font-size: 16px;
}
</style>
