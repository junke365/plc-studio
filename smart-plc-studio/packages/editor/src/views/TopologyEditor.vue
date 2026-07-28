<template>
  <div class="topology-editor">
    <!-- 顶部工具栏 -->
    <header class="topology-toolbar">
      <button class="back-btn" @click="goBack">
        <span class="material-symbols-outlined">arrow_back</span>
        返回IDE
      </button>
      <div class="toolbar-divider" />
      <button class="toolbar-btn" @click="saveTopology" title="保存拓扑">
        <span class="material-symbols-outlined">save</span>
        保存
      </button>
      <button class="toolbar-btn" @click="importTopology" title="导入拓扑">
        <span class="material-symbols-outlined">file_open</span>
        导入
      </button>
      <button class="toolbar-btn" @click="exportTopology" title="导出拓扑">
        <span class="material-symbols-outlined">file_download</span>
        导出
      </button>
      <div class="toolbar-spacer" />
      <span class="device-count">{{ devices.length }} 个设备 / {{ links.length }} 条连线</span>
    </header>

    <!-- 主内容区 -->
    <div class="topology-body">
      <DevicePanel />
      <DeviceCanvas
        :devices="devices"
        :links="links"
        @update:devices="devices = $event"
        @update:links="links = $event"
        @add-device="addDevice"
        @delete-device="deleteDevice"
        @rename-device="renameDevice"
        @configure-device="configureDevice"
        @add-link="addLink"
        @delete-link="deleteLink"
      />
    </div>

    <!-- 重命名对话框 -->
    <div v-if="showRenameDialog" class="dialog-overlay" @click.self="showRenameDialog = false">
      <div class="dialog">
        <div class="dialog-title">重命名设备</div>
        <input
          v-model="renameValue"
          class="dialog-input"
          placeholder="设备名称"
          @keyup.enter="confirmRename"
        />
        <div class="dialog-actions">
          <button class="dialog-btn" @click="showRenameDialog = false">取消</button>
          <button class="dialog-btn primary" @click="confirmRename">确定</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from "vue"
import { useRouter } from "vue-router"
import type { TopologyDevice, TopologyLink } from "@smart-plc/shared"
import DevicePanel from "../components/topology/DevicePanel.vue"
import DeviceCanvas from "../components/topology/DeviceCanvas.vue"

const router = useRouter()

const STORAGE_KEY = "plc-topology"

// 状态
const devices = ref<TopologyDevice[]>([])
const links = ref<TopologyLink[]>([])

// 重命名对话框
const showRenameDialog = ref(false)
const renameValue = ref("")
const renameTargetId = ref<string | null>(null)

let idCounter = 0
function generateId(): string {
  return `dev-${Date.now()}-${++idCounter}`
}

function linkId(): string {
  return `link-${Date.now()}-${++idCounter}`
}

// 持久化
function saveTopology() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify({
    devices: devices.value,
    links: links.value,
  }))
}

function loadTopology() {
  const raw = localStorage.getItem(STORAGE_KEY)
  if (raw) {
    try {
      const data = JSON.parse(raw)
      devices.value = data.devices || []
      links.value = data.links || []
    } catch {
      // ignore
    }
  }
}

// 导入/导出
function importTopology() {
  const input = document.createElement("input")
  input.type = "file"
  input.accept = ".json"
  input.onchange = async () => {
    const file = input.files?.[0]
    if (!file) return
    try {
      const text = await file.text()
      const data = JSON.parse(text)
      if (data.devices && data.links) {
        devices.value = data.devices
        links.value = data.links
      }
    } catch {
      alert("导入失败：JSON 格式错误")
    }
  }
  input.click()
}

function exportTopology() {
  const data = JSON.stringify({ devices: devices.value, links: links.value }, null, 2)
  const blob = new Blob([data], { type: "application/json" })
  const url = URL.createObjectURL(blob)
  const a = document.createElement("a")
  a.href = url
  a.download = "topology.json"
  a.click()
  URL.revokeObjectURL(url)
}

// 设备操作
function addDevice(type: string, x: number, y: number) {
  const count = devices.value.filter(d => d.type === type).length + 1
  const typeNames: Record<string, string> = {
    plc: "PLC",
    surgical_robot: "手术机器人",
    vision_camera: "视觉相机",
    px4_drone: "PX4 无人机",
    cnc_machine: "CNC 机床",
  }
  const device: TopologyDevice = {
    id: generateId(),
    name: `${typeNames[type] || type} ${count}`,
    type,
    x,
    y,
  }
  devices.value = [...devices.value, device]
}

function deleteDevice(id: string) {
  devices.value = devices.value.filter(d => d.id !== id)
  links.value = links.value.filter(l => l.sourceId !== id && l.targetId !== id)
}

function renameDevice(id: string) {
  const device = devices.value.find(d => d.id === id)
  if (device) {
    renameTargetId.value = id
    renameValue.value = device.name
    showRenameDialog.value = true
  }
}

function confirmRename() {
  if (renameTargetId.value && renameValue.value.trim()) {
    devices.value = devices.value.map(d =>
      d.id === renameTargetId.value ? { ...d, name: renameValue.value.trim() } : d
    )
  }
  showRenameDialog.value = false
  renameTargetId.value = null
}

function configureDevice(id: string) {
  const device = devices.value.find(d => d.id === id)
  if (device) {
    // 简单配置对话框：切换协议
    const protocols = ["ethercat", "modbus_tcp", "opc_ua", "virtual"]
    const currentProtocol = device.config?.protocol as string || "ethercat"
    const idx = protocols.indexOf(currentProtocol)
    const nextProtocol = protocols[(idx + 1) % protocols.length]
    devices.value = devices.value.map(d =>
      d.id === id ? { ...d, config: { ...(d.config || {}), protocol: nextProtocol } } : d
    )
  }
}

// 连线操作
function addLink(sourceId: string, targetId: string) {
  links.value = [...links.value, {
    id: linkId(),
    sourceId,
    targetId,
    protocol: "ethercat",
  }]
}

function deleteLink(id: string) {
  links.value = links.value.filter(l => l.id !== id)
}

function goBack() {
  router.push("/")
}

// 自动保存
watch([devices, links], () => {
  saveTopology()
}, { deep: true })

onMounted(() => {
  loadTopology()
})
</script>

<style scoped>
.topology-editor {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--surface);
  color: var(--on-surface);
}

.topology-toolbar {
  height: 48px;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 0 16px;
  background: var(--surface-container-highest);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.back-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 12px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: background 0.15s;
}

.back-btn:hover {
  background: var(--surface-container-high);
}

.back-btn .material-symbols-outlined {
  font-size: 16px;
}

.toolbar-divider {
  width: 1px;
  height: 24px;
  background: var(--outline-variant);
  margin: 0 4px;
}

.toolbar-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 12px;
  background: none;
  border: 1px solid transparent;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 12px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: all 0.15s;
}

.toolbar-btn:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
  border-color: var(--outline-variant);
}

.toolbar-btn .material-symbols-outlined {
  font-size: 16px;
}

.toolbar-spacer {
  flex: 1;
}

.device-count {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.topology-body {
  flex: 1;
  display: flex;
  overflow: hidden;
}

/* 对话框 */
.dialog-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.4);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 2000;
}

.dialog {
  background: var(--surface-container-high);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  padding: 20px;
  min-width: 320px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}

.dialog-title {
  font-size: 14px;
  font-weight: 600;
  margin-bottom: 16px;
}

.dialog-input {
  width: 100%;
  padding: 8px 12px;
  background: var(--surface);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 13px;
  font-family: "Inter", sans-serif;
  outline: none;
  box-sizing: border-box;
  transition: border-color 0.15s;
}

.dialog-input:focus {
  border-color: var(--primary);
}

.dialog-actions {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  margin-top: 16px;
}

.dialog-btn {
  padding: 6px 16px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: background 0.15s;
}

.dialog-btn:hover {
  background: var(--surface-container-high);
}

.dialog-btn.primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.dialog-btn.primary:hover {
  opacity: 0.9;
}
</style>
