<template>
  <div class="ide-layout">
    <TopNavBar />
    <div class="ide-main">
      <ActivityBar />
      <div
        v-show="uiStore.sidebarVisible"
        class="ide-sidebar"
        :style="{ width: uiStore.sidebarWidth + 'px' }"
      >
        <ProjectTree v-if="uiStore.sidebarTab === 'project'" />
        <LibraryPanel v-else-if="uiStore.sidebarTab === 'library'" />
        <DebugPanel v-else-if="uiStore.sidebarTab === 'debug'" />
      </div>
      <div class="ide-editor-area">
        <EditorTabs />
        <div class="editor-content">
          <Welcome v-if="editorStore.tabs.length === 0" />
          <template v-else>
            <template v-for="tab in editorStore.tabs" :key="tab.id">
              <STEditor
                v-if="
                  tab.language === EditorLanguage.ST ||
                  tab.language === EditorLanguage.IL
                "
                v-show="tab.id === editorStore.activeTabId"
                :tab="tab"
              />
              <FBDEditor
                v-else-if="tab.language === EditorLanguage.FBD"
                v-show="tab.id === editorStore.activeTabId"
                :tab="tab"
              />
              <LDEditor
                v-else-if="
                  tab.language === EditorLanguage.LD ||
                  tab.language === EditorLanguage.SFC
                "
                v-show="tab.id === editorStore.activeTabId"
                :tab="tab"
              />
              <HMIDesigner
                v-else-if="tab.language === EditorLanguage.HMI"
                v-show="tab.id === editorStore.activeTabId"
                :tab="tab"
              />
              <KinematicsEditor
                v-else-if="tab.language === EditorLanguage.Kinematics"
                v-show="tab.id === editorStore.activeTabId"
                :tab="tab"
              />
            </template>
          </template>
        </div>
        <div
          v-show="uiStore.bottomPanelVisible"
          class="ide-bottom-panel"
          :style="{ height: uiStore.bottomPanelHeight + 'px' }"
        >
          <div class="bottom-panel-tabs">
            <button
              class="bottom-tab"
              :class="{ active: uiStore.bottomPanelTab === 'output' }"
              @click="uiStore.bottomPanelTab = 'output'"
            >
              消息
            </button>
            <button
              class="bottom-tab"
              :class="{ active: uiStore.bottomPanelTab === 'debug' }"
              @click="uiStore.bottomPanelTab = 'debug'"
            >
              调试输出
            </button>
            <button
              class="bottom-tab"
              :class="{ active: uiStore.bottomPanelTab === 'breakpoints' }"
              @click="uiStore.bottomPanelTab = 'breakpoints'"
            >
              断点
            </button>
            <button
              class="bottom-tab"
              :class="{ active: uiStore.bottomPanelTab === 'watch' }"
              @click="uiStore.bottomPanelTab = 'watch'"
            >
              监视
            </button>
            <div class="bottom-tab-spacer" />
            <button
              class="bottom-tab-close"
              @click="uiStore.bottomPanelVisible = false"
              title="隐藏底部面板"
            >
              <span class="material-symbols-outlined">close</span>
            </button>
          </div>
          <OutputPanel v-if="uiStore.bottomPanelTab === 'output'" />
          <DebugOutput v-else-if="uiStore.bottomPanelTab === 'debug'" />
        </div>
      </div>
    </div>
    <StatusBar />
  </div>
</template>

<script setup lang="ts">
import { useEditorStore } from "../stores/editor";
import { useUiStore } from "../stores/ui";
import { EditorLanguage } from "@smart-plc/shared";
import TopNavBar from "../layouts/TopNavBar.vue";
import ActivityBar from "../layouts/ActivityBar.vue";
import ProjectTree from "../components/project/ProjectTree.vue";
import LibraryPanel from "../components/library/LibraryPanel.vue";
import DebugPanel from "../components/debug/DebugPanel.vue";
import EditorTabs from "../components/editors/EditorTabs.vue";
import LDEditor from "../components/editors/LD/LDEditor.vue";
import STEditor from "../components/editors/ST/STEditor.vue";
import FBDEditor from "../components/editors/FBD/FBDEditor.vue";
import HMIDesigner from "../components/editors/HMI/HMIDesigner.vue";
import KinematicsEditor from "../components/editors/Kinematics/KinematicsEditor.vue";
import Welcome from "./Welcome.vue";
import OutputPanel from "../components/debug/OutputPanel.vue";
import DebugOutput from "../components/debug/DebugOutput.vue";
import StatusBar from "../layouts/StatusBar.vue";

const editorStore = useEditorStore();
const uiStore = useUiStore();
</script>

<style scoped>
.ide-layout {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.ide-main {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.ide-sidebar {
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  border-right: 1px solid var(--outline-variant);
  background: var(--surface-container);
  overflow: hidden;
}

.ide-editor-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: var(--surface-container-lowest);
  overflow: hidden;
  min-width: 0;
}

.editor-content {
  flex: 1;
  overflow: auto;
  background: var(--editor-bg);
}

.ide-bottom-panel {
  display: flex;
  flex-direction: column;
  border-top: 1px solid var(--outline-variant);
  background: var(--surface-container);
  overflow: hidden;
}

.bottom-panel-tabs {
  display: flex;
  align-items: center;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  gap: 0;
  flex-shrink: 0;
}

.bottom-tab {
  padding: 0 var(--padding-sm);
  height: 28px;
  display: flex;
  align-items: center;
  background: none;
  border: none;
  border-bottom: 2px solid transparent;
  font-family: "Inter", sans-serif;
  font-size: 11px;
  color: var(--on-surface-variant);
  cursor: pointer;
  transition:
    background var(--transition-fast),
    color var(--transition-fast);
}

.bottom-tab:hover {
  color: var(--on-surface);
}

.bottom-tab.active {
  color: var(--primary);
  border-bottom-color: var(--primary);
}

.bottom-tab-spacer {
  flex: 1;
}

.bottom-tab-close {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: background var(--transition-fast);
}

.bottom-tab-close:hover {
  background: var(--surface-variant);
  color: var(--error);
}

.bottom-tab-close .material-symbols-outlined {
  font-size: 14px;
}
</style>
