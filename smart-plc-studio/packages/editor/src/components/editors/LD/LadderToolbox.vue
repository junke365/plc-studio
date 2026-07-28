<template>
  <div class="ladder-toolbox">
    <div class="toolbox-header">
      <span class="panel-title">LD 工具箱</span>
    </div>
    <div class="toolbox-content">
      <div class="toolbox-section">
        <div class="section-label">触点</div>
        <div class="toolbox-items">
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'contact')"
          >
            <span class="item-symbol">─┤ ├─</span>
            <span class="item-label">常开触点</span>
          </div>
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'contact_nc')"
          >
            <span class="item-symbol">─┤/├─</span>
            <span class="item-label">常闭触点</span>
          </div>
        </div>
      </div>

      <div class="toolbox-section">
        <div class="section-label">线圈</div>
        <div class="toolbox-items">
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'coil')"
          >
            <span class="item-symbol">─( )─</span>
            <span class="item-label">普通线圈</span>
          </div>
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'coil_nc')"
          >
            <span class="item-symbol">─(/)─</span>
            <span class="item-label">取反线圈</span>
          </div>
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'coil_set')"
          >
            <span class="item-symbol">─(S)─</span>
            <span class="item-label">置位线圈</span>
          </div>
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'coil_reset')"
          >
            <span class="item-symbol">─(R)─</span>
            <span class="item-label">复位线圈</span>
          </div>
        </div>
      </div>

      <div class="toolbox-section">
        <div class="section-label">功能块</div>
        <div class="toolbox-items">
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'functionBlock')"
          >
            <span class="item-symbol">[TON]</span>
            <span class="item-label">定时器</span>
          </div>
          <div
            class="toolbox-item"
            draggable="true"
            @dragstart="handleDragStart($event, 'functionBlock')"
          >
            <span class="item-symbol">[CTU]</span>
            <span class="item-label">计数器</span>
          </div>
        </div>
      </div>

      <div class="toolbox-divider" />

      <div class="toolbox-section">
        <div class="section-label">快捷添加</div>
        <div class="toolbox-items">
          <button class="quick-btn" @click="$emit('addElement', 'contact')">
            <span class="material-symbols-outlined">add</span>
            触点
          </button>
          <button class="quick-btn" @click="$emit('addElement', 'coil')">
            <span class="material-symbols-outlined">add</span>
            线圈
          </button>
          <button
            class="quick-btn"
            @click="$emit('addElement', 'functionBlock')"
          >
            <span class="material-symbols-outlined">add</span>
            功能块
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
const emit = defineEmits<{
  addElement: [type: string];
}>();

function handleDragStart(event: DragEvent, type: string) {
  if (!event.dataTransfer) return;

  const payload = JSON.stringify({ type, label: type });
  event.dataTransfer.setData("application/json", payload);
  event.dataTransfer.setData("text/plain", payload);
  event.dataTransfer.effectAllowed = "copy";
}
</script>

<style scoped>
.ladder-toolbox {
  width: 180px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.toolbox-header {
  padding: 8px 12px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
}

.panel-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface);
}

.toolbox-content {
  padding: 8px;
  flex: 1;
}

.toolbox-section {
  margin-bottom: 12px;
}

.section-label {
  font-size: 10px;
  font-weight: 600;
  color: var(--on-surface-variant);
  margin-bottom: 6px;
}

.toolbox-items {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.toolbox-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 8px;
  background: var(--surface-variant);
  border: 1px solid transparent;
  border-radius: var(--radius);
  cursor: grab;
  transition: all 0.15s;
}

.toolbox-item:hover {
  background: var(--surface-container-high);
  border-color: var(--primary);
}

.toolbox-item:active {
  cursor: grabbing;
}

.item-symbol {
  font-family: "JetBrains Mono", monospace;
  font-size: 10px;
  color: var(--secondary);
  white-space: nowrap;
}

.item-label {
  font-size: 11px;
  color: var(--on-surface);
}

.toolbox-divider {
  height: 1px;
  background: var(--outline-variant);
  margin: 8px 0;
}

.quick-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  background: none;
  border: 1px dashed var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 11px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: all 0.15s;
}

.quick-btn:hover {
  background: var(--surface-variant);
  border-color: var(--primary);
  color: var(--primary);
}

.quick-btn .material-symbols-outlined {
  font-size: 14px;
}
</style>
