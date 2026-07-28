<template>
  <div class="device-panel">
    <div class="panel-header">设备列表</div>
    <div class="device-list">
      <div
        v-for="dt in deviceTypes"
        :key="dt.type"
        class="device-card"
        draggable="true"
        @dragstart="handleDragStart($event, dt)"
      >
        <span class="material-symbols-outlined device-icon">{{ dt.icon }}</span>
        <div class="device-info">
          <span class="device-label">{{ dt.label }}</span>
          <span class="device-desc">{{ dt.desc }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
const deviceTypes = [
  { type: "plc", icon: "dns", label: "PLC 控制器", desc: "IEC 61131-3 标准 PLC" },
  { type: "surgical_robot", icon: "biotech", label: "手术机器人", desc: "dVRK 手术机器人" },
  { type: "vision_camera", icon: "visibility", label: "视觉相机", desc: "OpenCV 视觉处理" },
  { type: "px4_drone", icon: "flight", label: "PX4 无人机", desc: "PX4 飞控" },
  { type: "cnc_machine", icon: "precision_manufacturing", label: "CNC 机床", desc: "数控机床" },
]

const deviceTypeIcons: Record<string, string> = {
  plc: "dns",
  surgical_robot: "biotech",
  vision_camera: "visibility",
  px4_drone: "flight",
  cnc_machine: "precision_manufacturing",
}

function handleDragStart(event: DragEvent, dt: typeof deviceTypes[0]) {
  event.dataTransfer?.setData("application/json", JSON.stringify({
    type: dt.type,
    icon: dt.icon,
    label: dt.label,
  }))
  event.dataTransfer!.effectAllowed = "copy"
}

defineExpose({ deviceTypeIcons })
</script>

<style scoped>
.device-panel {
  width: 200px;
  min-width: 200px;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.panel-header {
  padding: 12px 16px;
  font-size: 12px;
  font-weight: 600;
  color: var(--on-surface-variant);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  border-bottom: 1px solid var(--outline-variant);
}

.device-list {
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.device-card {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 12px;
  border-radius: var(--radius);
  cursor: grab;
  transition: background 0.15s;
  user-select: none;
}

.device-card:hover {
  background: var(--surface-variant);
}

.device-card:active {
  cursor: grabbing;
  background: var(--primary-container);
}

.device-icon {
  font-size: 22px;
  color: var(--primary);
  flex-shrink: 0;
}

.device-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}

.device-label {
  font-size: 12px;
  font-weight: 600;
  color: var(--on-surface);
}

.device-desc {
  font-size: 10px;
  color: var(--on-surface-variant);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
</style>
