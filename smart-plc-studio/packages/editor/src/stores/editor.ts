import { defineStore } from "pinia";
import { ref, computed } from "vue";
import type { EditorTab } from "@smart-plc/shared";

interface HistoryEntry {
  tabId: string;
  content: string;
}

export const useEditorStore = defineStore("editor", () => {
  // 编辑器标签页
  const tabs = ref<EditorTab[]>([]);
  const activeTabId = ref<string>("");

  // 编辑器状态
  const isReadOnly = ref<boolean>(false);
  const showMinimap = ref<boolean>(true);
  const fontSize = ref<number>(14);
  const tabSize = ref<number>(2);

  // 撤销/重做历史栈
  const undoStack = ref<HistoryEntry[]>([]);
  const redoStack = ref<HistoryEntry[]>([]);
  const maxHistorySize = 100;

  // 剪贴板
  const clipboard = ref<string>("");

  // 计算属性
  const canUndo = computed(() => undoStack.value.length > 0);
  const canRedo = computed(() => redoStack.value.length > 0);

  // 打开标签页
  function openTab(tab: EditorTab) {
    const existing = tabs.value.find((t) => t.path === tab.path);
    if (existing) {
      activeTabId.value = existing.id;
    } else {
      tabs.value.push(tab);
      activeTabId.value = tab.id;
    }
  }

  // 关闭标签页
  function closeTab(id: string) {
    const index = tabs.value.findIndex((t) => t.id === id);
    if (index >= 0) {
      tabs.value.splice(index, 1);
      if (activeTabId.value === id) {
        activeTabId.value =
          tabs.value[index]?.id || tabs.value[index - 1]?.id || "";
      }
    }
  }

  // 切换标签页
  function switchTab(id: string) {
    activeTabId.value = id;
  }

  // 推送历史记录（在修改内容前调用）
  function pushHistory(tabId: string, content: string) {
    undoStack.value.push({ tabId, content });
    if (undoStack.value.length > maxHistorySize) {
      undoStack.value.shift();
    }
    // 新操作清空重做栈
    redoStack.value = [];
  }

  // 更新标签页内容
  function updateTabContent(id: string, content: string) {
    const tab = tabs.value.find((t) => t.id === id);
    if (tab) {
      pushHistory(id, tab.content);
      tab.content = content;
      tab.modified = true;
    }
  }

  // 撤消
  function undo(): boolean {
    const entry = undoStack.value.pop();
    if (!entry) return false;
    const tab = tabs.value.find((t) => t.id === entry.tabId);
    if (tab) {
      redoStack.value.push({ tabId: tab.id, content: tab.content });
      tab.content = entry.content;
      tab.modified = true;
    }
    return true;
  }

  // 重做
  function redo(): boolean {
    const entry = redoStack.value.pop();
    if (!entry) return false;
    const tab = tabs.value.find((t) => t.id === entry.tabId);
    if (tab) {
      undoStack.value.push({ tabId: tab.id, content: tab.content });
      tab.content = entry.content;
      tab.modified = true;
    }
    return true;
  }

  // 剪切
  function cutSelection(): boolean {
    const tab = getActiveTab();
    if (tab) {
      clipboard.value = tab.content;
      return true;
    }
    return false;
  }

  // 拷贝
  function copySelection(): boolean {
    const tab = getActiveTab();
    if (tab) {
      clipboard.value = tab.content;
      return true;
    }
    return false;
  }

  // 粘贴
  function pasteClipboard(): boolean {
    const tab = getActiveTab();
    if (tab && clipboard.value) {
      pushHistory(tab.id, tab.content);
      tab.content = clipboard.value;
      tab.modified = true;
      return true;
    }
    return false;
  }

  // 标记标签页已保存
  function markTabSaved(id: string) {
    const tab = tabs.value.find((t) => t.id === id);
    if (tab) {
      tab.modified = false;
    }
  }

  // 获取当前活动标签页
  function getActiveTab(): EditorTab | undefined {
    return tabs.value.find((t) => t.id === activeTabId.value);
  }

  // 关闭当前标签页
  function closeActiveTab() {
    if (activeTabId.value) {
      closeTab(activeTabId.value);
    }
  }

  // 关闭所有标签页
  function closeAllTabs() {
    tabs.value = [];
    activeTabId.value = "";
  }

  // 关闭其他标签页
  function closeOtherTabs(id: string) {
    tabs.value = tabs.value.filter((t) => t.id === id);
    activeTabId.value = id;
  }

  // 全选（当前标签页内容全部选中）
  function selectAll(): boolean {
    const tab = getActiveTab();
    if (tab) {
      return true;
    }
    return false;
  }

  return {
    tabs,
    activeTabId,
    isReadOnly,
    showMinimap,
    fontSize,
    tabSize,
    undoStack,
    redoStack,
    clipboard,
    canUndo,
    canRedo,
    openTab,
    closeTab,
    switchTab,
    pushHistory,
    updateTabContent,
    undo,
    redo,
    cutSelection,
    copySelection,
    pasteClipboard,
    markTabSaved,
    getActiveTab,
    closeActiveTab,
    closeAllTabs,
    closeOtherTabs,
    selectAll,
  };
});
