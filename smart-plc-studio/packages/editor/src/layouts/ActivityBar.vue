<template>
  <aside class="activity-bar">
    <div class="activity-items">
      <button
        v-for="item in activityItems"
        :key="item.id"
        class="activity-btn"
        :class="{
          active: uiStore.sidebarTab === item.id && uiStore.sidebarVisible,
        }"
        :title="item.label"
        @click="handleActivityClick(item.id)"
      >
        <span class="material-symbols-outlined">{{ item.icon }}</span>
      </button>
    </div>
    <div class="activity-spacer" />
    <button class="activity-btn" title="设置" @click="handleSettings">
      <span class="material-symbols-outlined">settings</span>
    </button>
  </aside>
</template>

<script setup lang="ts">
import { useUiStore } from "../stores/ui";
import { useEditorStore } from "../stores/editor";
import { EditorLanguage } from "@smart-plc/shared";

const uiStore = useUiStore();
const editorStore = useEditorStore();

const activityItems = [
  { id: "project", icon: "folder", label: "项目资源管理器" },
  { id: "search", icon: "search", label: "搜索" },
  { id: "debug", icon: "pest_control", label: "调试" },
  { id: "library", icon: "menu_book", label: "库" },
  { id: "simulation", icon: "play_circle", label: "仿真" },
  { id: "hmi", icon: "dashboard", label: "HMI 设计器" },
  { id: "kinematics", icon: "precision_manufacturing", label: "运动学配置" },
];

function handleActivityClick(id: string) {
  // HMI 和 Kinematics 直接打开编辑器标签
  if (id === "hmi") {
    editorStore.openTab({
      id: "hmi-designer",
      title: "HMI 设计器",
      language: EditorLanguage.HMI,
      path: "hmi/designer",
      modified: false,
      content: "",
    });
    return;
  }
  if (id === "kinematics") {
    editorStore.openTab({
      id: "kinematics-config",
      title: "运动学配置",
      language: EditorLanguage.Kinematics,
      path: "kinematics/config",
      modified: false,
      content: "",
    });
    return;
  }
  if (uiStore.sidebarTab === id && uiStore.sidebarVisible) {
    uiStore.toggleSidebar();
  } else {
    uiStore.sidebarTab = id;
    if (!uiStore.sidebarVisible) {
      uiStore.toggleSidebar();
    }
  }
}

function handleSettings() {
  // TODO: 打开设置
}
</script>

<style scoped>
.activity-bar {
  width: 48px;
  min-width: 48px;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  padding: var(--padding-sm) 0;
}

.activity-items {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.activity-btn {
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition:
    background var(--transition-fast),
    color var(--transition-fast);
  position: relative;
}

.activity-btn:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}

.activity-btn.active {
  color: var(--primary);
}

.activity-btn.active::before {
  content: "";
  position: absolute;
  left: 0;
  top: 8px;
  bottom: 8px;
  width: 2px;
  background: var(--primary);
  border-radius: 1px;
}

.activity-btn .material-symbols-outlined {
  font-size: 20px;
}

.activity-spacer {
  flex: 1;
}
</style>
