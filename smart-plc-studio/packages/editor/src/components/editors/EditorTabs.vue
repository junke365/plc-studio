<template>
  <div class="editor-tabs-bar">
    <div
      v-for="tab in editorStore.tabs"
      :key="tab.id"
      class="tab"
      :class="{ active: tab.id === editorStore.activeTabId }"
      @click="editorStore.switchTab(tab.id)"
    >
      <span class="material-symbols-outlined tab-icon">{{
        getTabIcon(tab.language)
      }}</span>
      <span class="tab-title">{{ tab.title }}</span>
      <span v-if="tab.modified" class="tab-modified">●</span>
      <span
        class="material-symbols-outlined tab-close"
        @click.stop="editorStore.closeTab(tab.id)"
        >close</span
      >
    </div>
  </div>
</template>

<script setup lang="ts">
import { useEditorStore } from "../../stores/editor";
import { EditorLanguage } from "@smart-plc/shared";

const editorStore = useEditorStore();

function getTabIcon(language: EditorLanguage): string {
  switch (language) {
    case EditorLanguage.LD:
      return "cable";
    case EditorLanguage.FBD:
      return "account_tree";
    case EditorLanguage.ST:
      return "code";
    case EditorLanguage.SFC:
      return "schema";
    case EditorLanguage.IL:
      return "list";
    case EditorLanguage.HMI:
      return "dashboard";
    case EditorLanguage.Kinematics:
      return "precision_manufacturing";
    default:
      return "description";
  }
}
</script>

<style scoped>
.editor-tabs-bar {
  display: flex;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  overflow-x: auto;
  flex-shrink: 0;
}

.editor-tabs-bar::-webkit-scrollbar {
  height: 0;
}

.tab {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 0 var(--padding-md);
  height: 32px;
  font-size: 13px;
  color: var(--on-surface-variant);
  border-right: 1px solid var(--outline-variant);
  cursor: pointer;
  white-space: nowrap;
  transition: background var(--transition-fast);
  user-select: none;
  flex-shrink: 0;
}

.tab:hover {
  background: var(--surface-variant);
}

.tab.active {
  background: var(--surface-container-lowest);
  color: var(--primary);
  border-top: 2px solid var(--primary);
  font-weight: 500;
}

.tab-icon {
  font-size: 14px;
}

.tab-title {
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
}

.tab-modified {
  font-size: 10px;
  color: var(--tertiary);
}

.tab-close {
  font-size: 14px;
  cursor: pointer;
  opacity: 0;
  transition:
    opacity var(--transition-fast),
    color var(--transition-fast);
  border-radius: var(--radius);
}

.tab:hover .tab-close {
  opacity: 0.6;
}

.tab-close:hover {
  opacity: 1 !important;
  color: var(--error);
}
</style>
