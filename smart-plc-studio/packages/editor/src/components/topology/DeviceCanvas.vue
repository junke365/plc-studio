<template>
  <div
    class="canvas-container"
    ref="containerRef"
    @wheel.prevent="handleWheel"
    @mousemove="handleMouseMove"
    @mouseup="handleMouseUp"
    @mouseleave="handleMouseLeave"
    @contextmenu.prevent="handleCanvasContext"
    @drop="handleDrop"
    @dragover.prevent
  >
    <div class="canvas-world" :style="canvasWorldStyle">
      <!-- SVG 连线层 -->
      <svg class="links-layer">
        <defs>
          <marker
            id="arrowhead"
            markerWidth="10"
            markerHeight="7"
            refX="9"
            refY="3.5"
            orient="auto"
          >
            <polygon points="0 0, 10 3.5, 0 7" fill="var(--primary)" />
          </marker>
        </defs>

        <!-- 已有连线 -->
        <g v-for="link in links" :key="link.id">
          <path
            :d="getLinkPath(link)"
            class="link-line"
            :class="{ 'link-selected': selectedLinkId === link.id }"
            @contextmenu.prevent.stop="showLinkMenu($event, link)"
            @click.stop="selectLink(link.id)"
          />
          <text
            :x="getLinkLabelPos(link).x"
            :y="getLinkLabelPos(link).y"
            class="link-label"
            @click.stop="selectLink(link.id)"
          >
            {{ link.protocol }}
          </text>
        </g>

        <!-- 正在拖拽的临时连线 -->
        <path
          v-if="connectingSourceId"
          :d="tempConnectPath"
          class="link-line temp-link"
        />
      </svg>

      <!-- 设备节点层 -->
      <div
        v-for="device in devices"
        :key="device.id"
        class="device-node"
        :class="{ 'device-selected': selectedDeviceId === device.id }"
        :style="{ left: device.x + 'px', top: device.y + 'px' }"
        @mousedown.stop="startDrag($event, device)"
        @mouseup.stop="tryConnect(device)"
        @click.stop="selectDevice(device.id)"
        @contextmenu.prevent.stop="showDeviceMenu($event, device)"
      >
        <div class="device-body">
          <span class="material-symbols-outlined device-node-icon">{{ getDeviceIcon(device.type) }}</span>
          <div class="device-text">
            <span class="device-node-name">{{ device.name }}</span>
            <span class="device-node-type">{{ getDeviceLabel(device.type) }}</span>
          </div>
        </div>
        <!-- 连接手柄 -->
        <div class="connector connector-output" @mousedown.stop="startConnect(device)" title="拖出连线">
          <span class="material-symbols-outlined">circle</span>
        </div>
        <div class="connector connector-input" title="接入连线">
          <span class="material-symbols-outlined">circle</span>
        </div>
      </div>
    </div>

    <!-- 右键上下文菜单 -->
    <div
      v-if="contextMenu"
      class="context-menu"
      :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
    >
      <template v-if="contextMenu.type === 'device'">
        <button class="context-item" @click="renameDevice(contextMenu.id)">
          <span class="material-symbols-outlined">edit</span> 重命名
        </button>
        <button class="context-item" @click="configureDevice(contextMenu.id)">
          <span class="material-symbols-outlined">settings</span> 配置
        </button>
        <div class="context-divider" />
        <button class="context-item danger" @click="deleteDevice(contextMenu.id)">
          <span class="material-symbols-outlined">delete</span> 删除
        </button>
      </template>
      <template v-else-if="contextMenu.type === 'link'">
        <button class="context-item danger" @click="deleteLink(contextMenu.id)">
          <span class="material-symbols-outlined">delete</span> 删除连线
        </button>
      </template>
    </div>

    <!-- 画布空白区提示 -->
    <div v-if="devices.length === 0" class="empty-hint">
      从左侧面板拖拽设备到此处
    </div>

    <!-- 缩放控制 -->
    <div class="zoom-controls">
      <button class="zoom-btn" @click="zoomIn" title="放大">
        <span class="material-symbols-outlined">add</span>
      </button>
      <span class="zoom-level">{{ Math.round(zoom * 100) }}%</span>
      <button class="zoom-btn" @click="zoomOut" title="缩小">
        <span class="material-symbols-outlined">remove</span>
      </button>
      <button class="zoom-btn" @click="resetZoom" title="重置">
        <span class="material-symbols-outlined">open_in_full</span>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from "vue"
import type { TopologyDevice, TopologyLink } from "@smart-plc/shared"

const props = defineProps<{
  devices: TopologyDevice[]
  links: TopologyLink[]
}>()

const emit = defineEmits<{
  "update:devices": [devices: TopologyDevice[]]
  "update:links": [links: TopologyLink[]]
  "add-device": [type: string, x: number, y: number]
  "delete-device": [id: string]
  "rename-device": [id: string]
  "configure-device": [id: string]
  "add-link": [sourceId: string, targetId: string]
  "delete-link": [id: string]
}>()

const containerRef = ref<HTMLElement | null>(null)

// 设备图标/标签映射
const deviceTypeMap: Record<string, { icon: string; label: string }> = {
  plc: { icon: "dns", label: "PLC 控制器" },
  surgical_robot: { icon: "biotech", label: "手术机器人" },
  vision_camera: { icon: "visibility", label: "视觉相机" },
  px4_drone: { icon: "flight", label: "PX4 无人机" },
  cnc_machine: { icon: "precision_manufacturing", label: "CNC 机床" },
}

function getDeviceIcon(type: string): string {
  return deviceTypeMap[type]?.icon || "device_hub"
}

function getDeviceLabel(type: string): string {
  return deviceTypeMap[type]?.label || type
}

// 缩放与平移
const zoom = ref(1)
const panX = ref(0)
const panY = ref(0)

const canvasWorldStyle = computed(() => ({
  transform: `translate(${panX.value}px, ${panY.value}px) scale(${zoom.value})`,
  transformOrigin: "0 0",
}))

function handleWheel(event: WheelEvent) {
  const delta = event.deltaY > 0 ? -0.1 : 0.1
  const newZoom = Math.min(3, Math.max(0.2, zoom.value + delta))
  zoom.value = newZoom
}

function zoomIn() {
  zoom.value = Math.min(3, zoom.value + 0.2)
}

function zoomOut() {
  zoom.value = Math.max(0.2, zoom.value - 0.2)
}

function resetZoom() {
  zoom.value = 1
  panX.value = 0
  panY.value = 0
}

// 选中状态
const selectedDeviceId = ref<string | null>(null)
const selectedLinkId = ref<string | null>(null)

function selectDevice(id: string) {
  selectedDeviceId.value = id
  selectedLinkId.value = null
  contextMenu.value = null
}

function selectLink(id: string) {
  selectedLinkId.value = id
  selectedDeviceId.value = null
  contextMenu.value = null
}

// 拖拽设备
const draggingDevice = ref<TopologyDevice | null>(null)
const dragOffset = ref({ x: 0, y: 0 })

function startDrag(event: MouseEvent, device: TopologyDevice) {
  // 只有左键拖拽
  if (event.button !== 0) return
  draggingDevice.value = device
  dragOffset.value = {
    x: event.clientX - device.x * zoom.value - panX.value,
    y: event.clientY - device.y * zoom.value - panY.value,
  }
  selectedDeviceId.value = device.id
  contextMenu.value = null
}

function handleMouseMove(event: MouseEvent) {
  if (draggingDevice.value) {
    const newX = (event.clientX - panX.value - dragOffset.value.x) / zoom.value
    const newY = (event.clientY - panY.value - dragOffset.value.y) / zoom.value
    const idx = props.devices.findIndex(d => d.id === draggingDevice.value!.id)
    if (idx !== -1) {
      const updated = [...props.devices]
      updated[idx] = { ...updated[idx], x: Math.max(0, newX), y: Math.max(0, newY) }
      emit("update:devices", updated)
    }
  }

  // 更新连线拖拽鼠标位置
  if (connectingSourceId.value) {
    connectMousePos.value = {
      x: (event.clientX - panX.value) / zoom.value,
      y: (event.clientY - panY.value) / zoom.value,
    }
  }
}

function handleMouseUp() {
  draggingDevice.value = null
  // 连接拖拽在 tryConnect 中处理
}

function handleMouseLeave() {
  draggingDevice.value = null
  connectingSourceId.value = null
}

// 从面板拖入
function handleDrop(event: DragEvent) {
  const data = event.dataTransfer?.getData("application/json")
  if (!data) return

  const rect = containerRef.value?.getBoundingClientRect()
  if (!rect) return

  const parsed = JSON.parse(data)
  const x = (event.clientX - rect.left - panX.value) / zoom.value
  const y = (event.clientY - rect.top - panY.value) / zoom.value

  emit("add-device", parsed.type, Math.max(0, x), Math.max(0, y))
  selectedDeviceId.value = null
  selectedLinkId.value = null
}

// 连线拖拽
const connectingSourceId = ref<string | null>(null)
const connectMousePos = ref({ x: 0, y: 0 })

const tempConnectPath = computed(() => {
  if (!connectingSourceId.value) return ""
  const source = props.devices.find(d => d.id === connectingSourceId.value)
  if (!source) return ""
  const sx = source.x + 90
  const sy = source.y + 28
  const ex = connectMousePos.value.x
  const ey = connectMousePos.value.y
  return computeBezierPath(sx, sy, ex, ey)
})

function startConnect(device: TopologyDevice) {
  connectingSourceId.value = device.id
  contextMenu.value = null
}

function tryConnect(targetDevice: TopologyDevice) {
  if (connectingSourceId.value && connectingSourceId.value !== targetDevice.id) {
    // 检查是否已存在连线
    const exists = props.links.some(
      l => l.sourceId === connectingSourceId.value && l.targetId === targetDevice.id
    )
    if (!exists) {
      emit("add-link", connectingSourceId.value, targetDevice.id)
    }
  }
  connectingSourceId.value = null
}

// 贝塞尔曲线路径计算
function getDeviceCenter(device: TopologyDevice) {
  return { x: device.x + 90, y: device.y + 28 }
}

function computeBezierPath(sx: number, sy: number, ex: number, ey: number): string {
  const dx = Math.abs(ex - sx) * 0.5
  const cp1x = sx + dx
  const cp1y = sy
  const cp2x = ex - dx
  const cp2y = ey
  return `M ${sx} ${sy} C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${ex} ${ey}`
}

function getLinkPath(link: TopologyLink): string {
  const source = props.devices.find(d => d.id === link.sourceId)
  const target = props.devices.find(d => d.id === link.targetId)
  if (!source || !target) return ""
  const s = getDeviceCenter(source)
  const t = getDeviceCenter(target)
  return computeBezierPath(s.x, s.y, t.x, t.y)
}

function getLinkLabelPos(link: TopologyLink): { x: number; y: number } {
  const source = props.devices.find(d => d.id === link.sourceId)
  const target = props.devices.find(d => d.id === link.targetId)
  if (!source || !target) return { x: 0, y: 0 }
  const s = getDeviceCenter(source)
  const t = getDeviceCenter(target)
  return { x: (s.x + t.x) / 2 - 10, y: (s.y + t.y) / 2 - 6 }
}

// 右键菜单
const contextMenu = ref<{ x: number; y: number; type: "device" | "link"; id: string } | null>(null)

function showDeviceMenu(event: MouseEvent, device: TopologyDevice) {
  selectedDeviceId.value = device.id
  contextMenu.value = {
    x: event.clientX,
    y: event.clientY,
    type: "device",
    id: device.id,
  }
}

function showLinkMenu(event: MouseEvent, link: TopologyLink) {
  selectedLinkId.value = link.id
  contextMenu.value = {
    x: event.clientX,
    y: event.clientY,
    type: "link",
    id: link.id,
  }
}

function handleCanvasContext() {
  contextMenu.value = null
  selectedDeviceId.value = null
  selectedLinkId.value = null
}

function renameDevice(id: string) {
  emit("rename-device", id)
  contextMenu.value = null
}

function configureDevice(id: string) {
  emit("configure-device", id)
  contextMenu.value = null
}

function deleteDevice(id: string) {
  emit("delete-device", id)
  contextMenu.value = null
  if (selectedDeviceId.value === id) selectedDeviceId.value = null
}

function deleteLink(id: string) {
  emit("delete-link", id)
  contextMenu.value = null
  if (selectedLinkId.value === id) selectedLinkId.value = null
}

// 点击空白关闭右键菜单
document.addEventListener("click", (e) => {
  if (contextMenu.value) {
    contextMenu.value = null
  }
})
</script>

<style scoped>
.canvas-container {
  flex: 1;
  position: relative;
  overflow: hidden;
  background: var(--surface);
  background-image:
    radial-gradient(circle, var(--outline-variant) 1px, transparent 1px);
  background-size: 24px 24px;
  cursor: default;
}

.canvas-world {
  position: absolute;
  top: 0;
  left: 0;
  width: 4000px;
  height: 4000px;
}

.links-layer {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}

.link-line {
  fill: none;
  stroke: var(--outline);
  stroke-width: 2;
  pointer-events: stroke;
  cursor: pointer;
  transition: stroke 0.15s;
}

.link-line:hover,
.link-selected {
  stroke: var(--primary);
  stroke-width: 2.5;
}

.temp-link {
  stroke: var(--primary);
  stroke-dasharray: 6 3;
}

.link-label {
  font-size: 10px;
  fill: var(--on-surface-variant);
  pointer-events: all;
  cursor: pointer;
  user-select: none;
}

/* 设备节点 */
.device-node {
  position: absolute;
  width: 180px;
  background: var(--surface-container-high);
  border: 2px solid var(--outline-variant);
  border-radius: var(--radius);
  cursor: grab;
  user-select: none;
  transition: border-color 0.15s, box-shadow 0.15s;
  z-index: 1;
}

.device-node:hover {
  border-color: var(--primary);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
}

.device-selected {
  border-color: var(--primary);
  box-shadow: 0 0 0 2px var(--primary-container);
}

.device-body {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 12px;
}

.device-node-icon {
  font-size: 28px;
  color: var(--primary);
  flex-shrink: 0;
}

.device-text {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}

.device-node-name {
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.device-node-type {
  font-size: 10px;
  color: var(--on-surface-variant);
}

/* 连接手柄 */
.connector {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: crosshair;
  transition: color 0.15s;
}

.connector .material-symbols-outlined {
  font-size: 12px;
}

.connector-output {
  right: -8px;
  color: var(--primary);
}

.connector-input {
  left: -8px;
  color: var(--secondary);
}

.connector:hover {
  color: var(--tertiary);
}

/* 右键菜单 */
.context-menu {
  position: fixed;
  background: var(--surface-container-high);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  padding: 4px;
  min-width: 140px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
  z-index: 1000;
}

.context-item {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 6px 12px;
  background: none;
  border: none;
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 12px;
  font-family: inherit;
  cursor: pointer;
  text-align: left;
  transition: background 0.1s;
}

.context-item:hover {
  background: var(--surface-variant);
}

.context-item.danger {
  color: var(--error);
}

.context-item .material-symbols-outlined {
  font-size: 16px;
}

.context-divider {
  height: 1px;
  background: var(--outline-variant);
  margin: 4px 0;
}

/* 空提示 */
.empty-hint {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  color: var(--on-surface-variant);
  font-size: 14px;
  opacity: 0.6;
  pointer-events: none;
}

/* 缩放控制 */
.zoom-controls {
  position: absolute;
  bottom: 16px;
  right: 16px;
  display: flex;
  align-items: center;
  gap: 4px;
  background: var(--surface-container-high);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  padding: 4px;
  z-index: 10;
}

.zoom-btn {
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  border-radius: 4px;
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: background 0.1s;
}

.zoom-btn:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.zoom-btn .material-symbols-outlined {
  font-size: 16px;
}

.zoom-level {
  font-size: 11px;
  color: var(--on-surface-variant);
  min-width: 36px;
  text-align: center;
  font-variant-numeric: tabular-nums;
}
</style>
