<template>
  <div class="st-editor" ref="editorContainer" />
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch } from "vue";
import editorLoader from "@monaco-editor/loader";
import { useEditorStore } from "../../../stores/editor";
import type { EditorTab } from "@smart-plc/shared";
import { registerSTLanguage } from "./stLanguage";
import { registerSTCompletion } from "./stCompletion";

const props = defineProps<{
  tab: EditorTab;
}>();

const editorStore = useEditorStore();
const editorContainer = ref<HTMLDivElement | null>(null);

let editor: any = null;
let monacoInstance: any = null;
let languageRegistered = false;

onMounted(async () => {
  await initEditor();
});

onUnmounted(() => {
  if (editor) {
    editor.dispose();
  }
});

watch(
  () => props.tab.content,
  (newContent) => {
    if (editor && monacoInstance) {
      const model = editor.getModel();
      if (model && model.getValue() !== newContent) {
        model.setValue(newContent || "");
      }
    }
  },
);

async function initEditor() {
  if (!editorContainer.value) return;

  const monaco = await editorLoader.init();
  monacoInstance = monaco;

  // 注册 ST 语言（只注册一次）
  if (!languageRegistered) {
    registerSTLanguage(monaco);
    registerSTCompletion(monaco);
    languageRegistered = true;
  }

  // 创建模型
  const modelUri = monaco.Uri.parse(`inmemory://pou/${props.tab.id}.st`);
  const model = monaco.editor.createModel(
    props.tab.content || "",
    "st",
    modelUri,
  );

  // 创建编辑器
  editor = monaco.editor.create(editorContainer.value, {
    model,
    theme: "vs-dark",
    minimap: { enabled: false },
    fontSize: 13,
    fontFamily: "'JetBrains Mono', 'Cascadia Code', Consolas, monospace",
    lineNumbers: "on",
    roundedSelection: true,
    scrollBeyondLastLine: false,
    automaticLayout: true,
    tabSize: 2,
    wordWrap: "on",
    bracketPairColorization: { enabled: true },
    glyphMargin: true,
    folding: true,
    foldingHighlight: true,
    showFoldingControls: "mouseover",
    renderLineHighlight: "all",
    cursorBlinking: "smooth",
    smoothScrolling: true,
    padding: { top: 8, bottom: 8 },
  });

  // 监听内容变化
  editor.onDidChangeModelContent(() => {
    const value = editor.getValue();
    editorStore.updateTabContent(props.tab.id, value);
  });

  // Ctrl+S 保存
  editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, () => {
    editorStore.markTabSaved(props.tab.id);
  });
}
</script>

<style scoped>
.st-editor {
  width: 100%;
  height: 100%;
  overflow: hidden;
}
</style>
