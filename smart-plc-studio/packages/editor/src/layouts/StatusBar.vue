<template>
  <footer class="status-bar">
    <div class="status-left">
      <span class="status-item project-name" v-if="projectStore.projectName">
        {{ projectStore.projectName }}
      </span>
      <span class="status-item modified" v-if="projectStore.projectModified">
        ● 已修改
      </span>
      <span
        class="status-divider"
        v-if="debugStore.runtimeStatus !== RuntimeStatus.Disconnected"
      />
      <span
        class="status-item"
        :class="runtimeStatusClass"
        v-if="debugStore.runtimeStatus !== RuntimeStatus.Disconnected"
      >
        <span class="status-dot" :class="runtimeStatusClass" />
        {{ runtimeStatusText }}
      </span>
      <span
        class="status-item"
        v-if="debugStore.runtimeStatus === RuntimeStatus.Running"
      >
        <span class="status-divider-inline" />
        任务时间: {{ debugStore.taskTime }}ms
      </span>
    </div>
    <div class="status-right">
      <span class="status-item">
        行 {{ currentLine }}, 列 {{ currentColumn }}
      </span>
      <span class="status-divider" />
      <span class="status-item">{{ currentLanguage }}</span>
      <span class="status-divider" />
      <span class="status-item version">v1.0.0</span>
    </div>
  </footer>
</template>

<script setup lang="ts">
import { ref, computed } from "vue";
import { useProjectStore } from "../stores/project";
import { useEditorStore } from "../stores/editor";
import { useDebugStore } from "../stores/debug";
import { RuntimeStatus } from "@smart-plc/shared";

const projectStore = useProjectStore();
const editorStore = useEditorStore();
const debugStore = useDebugStore();

const currentLine = ref(1);
const currentColumn = ref(1);

const runtimeStatusText = computed(() => {
  switch (debugStore.runtimeStatus) {
    case RuntimeStatus.Connected:
      return "已连接";
    case RuntimeStatus.Running:
      return "PLC: RUNNING";
    case RuntimeStatus.Paused:
      return "已暂停";
    case RuntimeStatus.Error:
      return "错误";
    default:
      return "未连接";
  }
});

const runtimeStatusClass = computed(() => {
  switch (debugStore.runtimeStatus) {
    case RuntimeStatus.Running:
      return "status-running";
    case RuntimeStatus.Error:
      return "status-error";
    default:
      return "status-default";
  }
});

const currentLanguage = computed(() => {
  const tab = editorStore.getActiveTab();
  return tab?.language || "未知";
});
</script>

<style scoped>
.status-bar {
  height: var(--statusbar-height);
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: var(--surface-dim);
  padding: 0 var(--padding-md);
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  border-top: 1px solid var(--outline-variant);
  z-index: 50;
  flex-shrink: 0;
}

.status-left,
.status-right {
  display: flex;
  align-items: center;
  gap: var(--padding-sm);
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
  color: var(--on-surface-variant);
  white-space: nowrap;
}

.status-item.project-name {
  color: var(--on-surface);
}

.status-item.modified {
  color: var(--tertiary);
}

.status-item.status-running {
  color: var(--secondary);
}

.status-item.status-error {
  color: var(--error);
}

.status-item.version {
  color: var(--on-surface-variant);
}

.status-dot {
  width: 6px;
  height: 6px;
  border-radius: var(--radius-full);
  background: currentColor;
  flex-shrink: 0;
}

.status-dot.status-running {
  background: var(--secondary);
  animation: pulse-anim 2s ease-in-out infinite;
}

.status-divider {
  width: 1px;
  height: 12px;
  background: var(--outline-variant);
}

.status-divider-inline {
  width: 1px;
  height: 12px;
  background: var(--outline-variant);
  margin: 0 2px;
}
</style>
