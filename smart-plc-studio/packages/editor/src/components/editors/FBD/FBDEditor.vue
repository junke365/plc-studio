<template>
  <div class="fbd-editor-layout">
    <!-- 左侧库面板 -->
    <div class="fbd-library">
      <div class="library-header">
        <span class="panel-title">FBD 库</span>
      </div>
      <div class="library-content">
        <div class="lib-section">
          <div class="section-label">输入/输出</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'variable_in')"
            >
              <span class="lib-icon">▶</span>
              <span>输入变量</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'variable_out')"
            >
              <span class="lib-icon">◀</span>
              <span>输出变量</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'constant')"
            >
              <span class="lib-icon">K</span>
              <span>常量</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">定时器</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'TON')"
            >
              <span class="lib-icon">⏱</span>
              <span>TON</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'TOF')"
            >
              <span class="lib-icon">⏱</span>
              <span>TOF</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'TP')"
            >
              <span class="lib-icon">⏱</span>
              <span>TP</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'TONR')"
            >
              <span class="lib-icon">⏱</span>
              <span>TONR</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">计数器</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'CTU')"
            >
              <span class="lib-icon">#</span>
              <span>CTU</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'CTD')"
            >
              <span class="lib-icon">#</span>
              <span>CTD</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'CTUD')"
            >
              <span class="lib-icon">#</span>
              <span>CTUD</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">触发器</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'R_TRIG')"
            >
              <span class="lib-icon">↑</span>
              <span>R_TRIG</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'F_TRIG')"
            >
              <span class="lib-icon">↓</span>
              <span>F_TRIG</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'SR')"
            >
              <span class="lib-icon">S</span>
              <span>SR</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'RS')"
            >
              <span class="lib-icon">R</span>
              <span>RS</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">数学运算</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'ADD')"
            >
              <span class="lib-icon">+</span><span>ADD</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'SUB')"
            >
              <span class="lib-icon">-</span><span>SUB</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'MUL')"
            >
              <span class="lib-icon">×</span><span>MUL</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'DIV')"
            >
              <span class="lib-icon">÷</span><span>DIV</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'MOD')"
            >
              <span class="lib-icon">%</span><span>MOD</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'LIMIT')"
            >
              <span class="lib-icon">◇</span><span>LIMIT</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">比较</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'GT')"
            >
              <span class="lib-icon">&gt;</span><span>GT</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'GE')"
            >
              <span class="lib-icon">&ge;</span><span>GE</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'EQ')"
            >
              <span class="lib-icon">=</span><span>EQ</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'LE')"
            >
              <span class="lib-icon">&le;</span><span>LE</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'LT')"
            >
              <span class="lib-icon">&lt;</span><span>LT</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">逻辑</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'AND')"
            >
              <span class="lib-icon">∧</span><span>AND</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'OR')"
            >
              <span class="lib-icon">∨</span><span>OR</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'XOR')"
            >
              <span class="lib-icon">⊕</span><span>XOR</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'NOT')"
            >
              <span class="lib-icon">¬</span><span>NOT</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">运动控制</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'MC_Power')"
            >
              <span class="lib-icon">⚡</span><span>MC_Power</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'MC_MoveAbsolute')"
            >
              <span class="lib-icon">→</span><span>MC_MoveAbs</span>
            </div>
            <div
              class="lib-item"
              draggable="true"
              @dragstart="onLibDrag($event, 'MC_Stop')"
            >
              <span class="lib-icon">■</span><span>MC_Stop</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 画布 -->
    <div
      class="fbd-canvas-wrapper"
      ref="wrapperRef"
      tabindex="0"
      @drop="onDrop"
      @dragover="onDragOver"
      @keydown="onKeyDown"
    >
      <div class="fbd-canvas">
        <VueFlow
          v-if="flowReady"
          :nodes="nodes"
          :edges="edges"
          :node-types="nodeTypes"
          :default-edge-options="defaultEdgeOptions"
          :nodes-draggable="true"
          :nodes-connectable="true"
          :elements-selectable="true"
          :pan-on-drag="true"
          :zoom-on-scroll="true"
          :zoom-on-pinch="true"
          :min-zoom="0.1"
          :max-zoom="4"
          fit-view-on-init
          @nodes-change="onNodesChange"
          @edges-change="onEdgesChange"
          @connect="onConnect"
          @node-click="onNodeClick"
          @edge-click="onEdgeClick"
          @pane-click="onPaneClick"
        >
          <Background :gap="20" :size="0.5" pattern-color="#1a2340" />
        </VueFlow>
      </div>
    </div>

    <!-- 属性面板 -->
    <div class="fbd-properties" v-if="selectedNode || selectedEdge">
      <div class="properties-header">
        <span class="panel-title">{{
          selectedEdge ? "连线属性" : "属性"
        }}</span>
      </div>
      <div class="properties-content">
        <template v-if="selectedEdge">
          <div class="prop-row">
            <span class="prop-label">来源</span
            ><span class="prop-value prop-small">{{
              selectedEdge.source
            }}</span>
          </div>
          <div class="prop-row">
            <span class="prop-label">目标</span
            ><span class="prop-value prop-small">{{
              selectedEdge.target
            }}</span>
          </div>
          <button class="delete-btn" @click="deleteSelected">
            <span class="material-symbols-outlined">delete</span>删除连线
          </button>
        </template>
        <template v-else-if="selectedNode">
          <div class="prop-row">
            <span class="prop-label">类型</span
            ><span class="prop-value">{{ selectedNode.type }}</span>
          </div>
          <div class="prop-row" v-if="selectedNode.data?.label !== undefined">
            <span class="prop-label">名称</span>
            <input
              class="prop-input"
              :value="selectedNode.data?.label || ''"
              @input="
                updateNodeProp(
                  'label',
                  ($event.target as HTMLInputElement).value,
                )
              "
              placeholder="名称"
            />
          </div>
          <div
            class="prop-row"
            v-if="selectedNode.data?.variable !== undefined"
          >
            <span class="prop-label">变量</span>
            <input
              class="prop-input"
              :value="selectedNode.data?.variable || ''"
              @input="
                updateNodeProp(
                  'variable',
                  ($event.target as HTMLInputElement).value,
                )
              "
              placeholder="变量名"
            />
          </div>
          <div class="prop-row" v-if="selectedNode.data?.value !== undefined">
            <span class="prop-label">值</span>
            <input
              class="prop-input"
              :value="selectedNode.data?.value || ''"
              @input="
                updateNodeProp(
                  'value',
                  ($event.target as HTMLInputElement).value,
                )
              "
              placeholder="值"
            />
          </div>
          <button class="delete-btn" @click="deleteSelected">
            <span class="material-symbols-outlined">delete</span>删除节点
          </button>
        </template>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import {
  ref,
  computed,
  onMounted,
  nextTick,
  markRaw,
  h,
  defineComponent,
} from "vue";
import { VueFlow, useVueFlow } from "@vue-flow/core";
import { Background } from "@vue-flow/background";
import type {
  Node,
  Edge,
  Connection,
  NodeChange,
  EdgeChange,
} from "@vue-flow/core";
import { Position, Handle } from "@vue-flow/core";
import { useEditorStore } from "../../../stores/editor";
import type { EditorTab } from "@smart-plc/shared";

// ========== 功能块引脚定义 ==========

const FB_PINS: Record<string, { inputs: string[]; outputs: string[] }> = {
  TON: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
  TOF: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
  TP: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
  TONR: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
  CTU: { inputs: ["CU", "R", "PV"], outputs: ["Q", "CV"] },
  CTD: { inputs: ["CD", "LD", "PV"], outputs: ["Q", "CV"] },
  CTUD: { inputs: ["CU", "CD", "R", "LD", "PV"], outputs: ["QU", "QD", "CV"] },
  R_TRIG: { inputs: ["CLK"], outputs: ["Q"] },
  F_TRIG: { inputs: ["CLK"], outputs: ["Q"] },
  SR: { inputs: ["S1", "R"], outputs: ["Q1"] },
  RS: { inputs: ["S", "R1"], outputs: ["Q1"] },
  ADD: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  SUB: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  MUL: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  DIV: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  MOD: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  GT: { inputs: ["IN1", "IN2"], outputs: ["GT"] },
  GE: { inputs: ["IN1", "IN2"], outputs: ["GE"] },
  EQ: { inputs: ["IN1", "IN2"], outputs: ["EQ"] },
  LE: { inputs: ["IN1", "IN2"], outputs: ["LE"] },
  LT: { inputs: ["IN1", "IN2"], outputs: ["LT"] },
  LIMIT: { inputs: ["EN", "MN", "IN", "MX"], outputs: ["ENO", "OUT"] },
  AND: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  OR: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  XOR: { inputs: ["EN", "IN1", "IN2"], outputs: ["ENO", "OUT"] },
  NOT: { inputs: ["EN", "IN1"], outputs: ["ENO", "OUT"] },
  MC_Power: { inputs: ["Enable", "Axis"], outputs: ["Done", "Error"] },
  MC_MoveAbsolute: {
    inputs: ["Execute", "Axis", "Position", "Velocity"],
    outputs: ["Done", "Error"],
  },
  MC_Stop: { inputs: ["Execute", "Axis"], outputs: ["Done", "Error"] },
};

// ========== 节点组件 ==========

const FunctionBlockNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () => {
      const label = props.data?.label || "FB";
      const inputs = props.data?.inputs || ["IN"];
      const outputs = props.data?.outputs || ["Q"];
      const maxPins = Math.max(inputs.length, outputs.length);
      const blockHeight = Math.max(40, maxPins * 22 + 28);
      return h(
        "div",
        { class: "fbd-node fbd-block", style: { height: blockHeight + "px" } },
        [
          h("div", { class: "fbd-block-header" }, label),
          h("div", { class: "fbd-block-body" }, [
            h(
              "div",
              { class: "fbd-block-pins-left" },
              inputs.map((name: string, i: number) =>
                h("div", { class: "fbd-pin-row", key: "i" + i }, [
                  h(Handle, {
                    type: "target",
                    position: Position.Left,
                    id: "in-" + i,
                    style: { top: (i + 1) * 22 + 10 + "px" },
                  }),
                  h("span", { class: "fbd-pin-label" }, name),
                ]),
              ),
            ),
            h(
              "div",
              { class: "fbd-block-pins-right" },
              outputs.map((name: string, i: number) =>
                h("div", { class: "fbd-pin-row", key: "o" + i }, [
                  h("span", { class: "fbd-pin-label" }, name),
                  h(Handle, {
                    type: "source",
                    position: Position.Right,
                    id: "out-" + i,
                    style: { top: (i + 1) * 22 + 10 + "px" },
                  }),
                ]),
              ),
            ),
          ]),
        ],
      );
    };
  },
});

const VariableInNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () =>
      h("div", { class: "fbd-node fbd-variable" }, [
        h("div", { class: "fbd-var-label" }, props.data?.variable || "???"),
        h(Handle, { type: "source", position: Position.Right }),
      ]);
  },
});

const VariableOutNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () =>
      h("div", { class: "fbd-node fbd-variable" }, [
        h("div", { class: "fbd-var-label" }, props.data?.variable || "???"),
        h(Handle, { type: "target", position: Position.Left }),
      ]);
  },
});

const ConstantNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () =>
      h("div", { class: "fbd-node fbd-constant" }, [
        h("div", { class: "fbd-const-value" }, props.data?.value || "0"),
        h(Handle, { type: "source", position: Position.Right }),
      ]);
  },
});

const nodeTypes = {
  fbdBlock: markRaw(FunctionBlockNode),
  fbdVarIn: markRaw(VariableInNode),
  fbdVarOut: markRaw(VariableOutNode),
  fbdConst: markRaw(ConstantNode),
};

const defaultEdgeOptions = {
  type: "smoothstep",
  deletable: true,
  selectable: true,
};

// ========== 数据模型 ==========

const props = defineProps<{ tab: EditorTab }>();
const editorStore = useEditorStore();
const wrapperRef = ref<HTMLDivElement | null>(null);
const flowReady = ref(false);
const { project, fitView } = useVueFlow();

const nodes = ref<Node[]>([]);
const edges = ref<Edge[]>([]);
const selectedNode = ref<Node | null>(null);
const selectedEdge = ref<Edge | null>(null);

// ========== 生命周期 ==========

onMounted(async () => {
  loadData();
  await nextTick();
  flowReady.value = true;
  await nextTick();
  fitView({ padding: 0.2 });
});

// ========== 持久化 ==========

function loadData() {
  try {
    if (props.tab.content) {
      const data = JSON.parse(props.tab.content);
      if (data.nodes) nodes.value = data.nodes;
      if (data.edges) edges.value = data.edges;
      return;
    }
  } catch {
    /* 忽略 */
  }
  nodes.value = [];
  edges.value = [];
}

function saveData() {
  editorStore.updateTabContent(
    props.tab.id,
    JSON.stringify({
      nodes: nodes.value,
      edges: edges.value,
    }),
  );
}

// ========== 库拖放 ==========

function onLibDrag(event: DragEvent, type: string) {
  if (!event.dataTransfer) return;
  event.dataTransfer.setData(
    "application/json",
    JSON.stringify({ fbdType: type }),
  );
  event.dataTransfer.effectAllowed = "copy";
}

function onDragOver(event: DragEvent) {
  event.preventDefault();
  if (event.dataTransfer) event.dataTransfer.dropEffect = "copy";
}

function onDrop(event: DragEvent) {
  if (!event.dataTransfer) return;
  const bounds = wrapperRef.value?.getBoundingClientRect();
  if (!bounds) return;
  const position = project({
    x: event.clientX - bounds.left,
    y: event.clientY - bounds.top,
  });
  const raw = event.dataTransfer.getData("application/json");
  if (!raw) return;
  try {
    const item = JSON.parse(raw);
    if (!item.fbdType) return;
    createNode(item.fbdType, position.x, position.y);
  } catch {
    /* 忽略 */
  }
}

function createNode(fbdType: string, x: number, y: number) {
  const id = `fbd-${Date.now()}`;
  let nodeType = "fbdBlock";
  let data: any = {};

  if (fbdType === "variable_in") {
    nodeType = "fbdVarIn";
    data = { variable: "", label: "输入" };
  } else if (fbdType === "variable_out") {
    nodeType = "fbdVarOut";
    data = { variable: "", label: "输出" };
  } else if (fbdType === "constant") {
    nodeType = "fbdConst";
    data = { value: "0", label: "常量" };
  } else {
    // 功能块
    const pins = FB_PINS[fbdType] || { inputs: ["EN"], outputs: ["ENO"] };
    data = {
      label: fbdType,
      inputs: [...pins.inputs],
      outputs: [...pins.outputs],
    };
  }

  nodes.value.push({ id, type: nodeType, position: { x, y }, data });
  saveData();
}

// ========== 连线 ==========

function onConnect(connection: Connection) {
  if (!connection.source || !connection.target) return;
  edges.value.push({
    id: `e-${connection.source}-${connection.target}-${Date.now()}`,
    source: connection.source,
    target: connection.target,
    sourceHandle: connection.sourceHandle || undefined,
    targetHandle: connection.targetHandle || undefined,
    type: "smoothstep",
  });
  saveData();
}

// ========== 变更处理 ==========

function onNodesChange(changes: NodeChange[]) {
  for (const change of changes) {
    if (change.type === "position" && change.position) {
      const node = nodes.value.find((n) => n.id === change.id);
      if (node) node.position = { ...change.position };
    }
    if (change.type === "remove") {
      const idx = nodes.value.findIndex((n) => n.id === change.id);
      if (idx >= 0) {
        nodes.value.splice(idx, 1);
        edges.value = edges.value.filter(
          (e) => e.source !== change.id && e.target !== change.id,
        );
      }
      if (selectedNode.value?.id === change.id) selectedNode.value = null;
    }
  }
  saveData();
}

function onEdgesChange(changes: EdgeChange[]) {
  for (const change of changes) {
    if (change.type === "remove") {
      const idx = edges.value.findIndex((e) => e.id === change.id);
      if (idx >= 0) edges.value.splice(idx, 1);
      if (selectedEdge.value?.id === change.id) selectedEdge.value = null;
    }
  }
  saveData();
}

// ========== 选择和删除 ==========

function onNodeClick({ node }: { node: Node }) {
  selectedNode.value = node;
  selectedEdge.value = null;
  wrapperRef.value?.focus();
}

function onEdgeClick({ edge }: { edge: Edge }) {
  selectedEdge.value = edge;
  selectedNode.value = null;
  wrapperRef.value?.focus();
}

function onPaneClick() {
  selectedNode.value = null;
  selectedEdge.value = null;
  wrapperRef.value?.focus();
}

function onKeyDown(event: KeyboardEvent) {
  const tag = (event.target as HTMLElement)?.tagName;
  if (tag === "INPUT" || tag === "TEXTAREA") return;
  if (event.key === "Delete" || event.key === "Backspace") {
    deleteSelected();
  }
}

function deleteSelected() {
  if (selectedEdge.value) {
    const idx = edges.value.findIndex((e) => e.id === selectedEdge.value!.id);
    if (idx >= 0) edges.value.splice(idx, 1);
    selectedEdge.value = null;
    saveData();
    return;
  }
  if (selectedNode.value) {
    const id = selectedNode.value.id;
    const idx = nodes.value.findIndex((n) => n.id === id);
    if (idx >= 0) {
      nodes.value.splice(idx, 1);
      edges.value = edges.value.filter(
        (e) => e.source !== id && e.target !== id,
      );
    }
    selectedNode.value = null;
    saveData();
  }
}

function updateNodeProp(key: string, value: string) {
  if (!selectedNode.value) return;
  const idx = nodes.value.findIndex((n) => n.id === selectedNode.value!.id);
  if (idx >= 0) {
    nodes.value[idx] = {
      ...nodes.value[idx],
      data: { ...nodes.value[idx].data, [key]: value },
    };
    nodes.value = [...nodes.value];
    selectedNode.value = nodes.value[idx];
    saveData();
  }
}
</script>

<style>
/* VueFlow 全局样式 */
.fbd-canvas .vue-flow {
  background: #0f172a;
}

/* 节点基础 */
.fbd-node {
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  font-size: 11px;
  color: var(--on-surface);
  text-align: center;
  min-width: 80px;
  transition: border-color 0.15s;
}

.fbd-canvas .vue-flow__node.selected .fbd-node {
  border-color: var(--primary);
  box-shadow: 0 0 0 2px var(--primary-container);
}

/* 功能块 */
.fbd-block {
  min-width: 140px;
  padding: 0;
  display: flex;
  flex-direction: column;
}

.fbd-block-header {
  padding: 4px 8px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  font-weight: 700;
  font-size: 11px;
  border-radius: var(--radius) var(--radius) 0 0;
  border-bottom: 1px solid var(--outline-variant);
}

.fbd-block-body {
  display: flex;
  justify-content: space-between;
  padding: 4px 0;
  flex: 1;
}

.fbd-block-pins-left,
.fbd-block-pins-right {
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.fbd-pin-row {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 1px 6px;
}

.fbd-pin-label {
  font-size: 10px;
  color: var(--on-surface-variant);
  font-family: "JetBrains Mono", monospace;
}

.fbd-block-pins-right .fbd-pin-row {
  justify-content: flex-end;
}

/* 变量节点 */
.fbd-variable {
  padding: 8px 16px;
  min-width: 80px;
  background: var(--tertiary-container);
  border-color: var(--tertiary);
}

.fbd-var-label {
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-tertiary-container);
}

/* 常量节点 */
.fbd-constant {
  padding: 8px 12px;
  min-width: 60px;
  background: var(--secondary-container);
  border-color: var(--secondary);
}

.fbd-const-value {
  font-family: "JetBrains Mono", monospace;
  font-size: 12px;
  font-weight: 700;
  color: var(--on-secondary-container);
}

/* Handle 样式 */
.fbd-canvas .vue-flow__handle {
  width: 10px;
  height: 10px;
  border: 2px solid var(--primary);
  background: var(--surface-container);
}

.fbd-canvas .vue-flow__handle:hover {
  background: var(--primary);
  transform: scale(1.3);
}

/* 连线样式 */
.fbd-canvas .vue-flow__edge.selected .vue-flow__edge-path {
  stroke: var(--error) !important;
  stroke-width: 3;
}

.fbd-canvas .vue-flow__edge:hover .vue-flow__edge-path {
  stroke: var(--primary);
  stroke-width: 2.5;
}
</style>

<style scoped>
.fbd-editor-layout {
  width: 100%;
  height: 100%;
  display: flex;
  overflow: hidden;
}

/* 库面板 */
.fbd-library {
  width: 180px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.library-header {
  padding: 8px 12px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.panel-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface);
}

.library-content {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.lib-section {
  margin-bottom: 12px;
}

.section-label {
  font-size: 10px;
  font-weight: 600;
  color: var(--on-surface-variant);
  margin-bottom: 4px;
  padding-left: 2px;
}

.lib-items {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.lib-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 8px;
  background: var(--surface-variant);
  border: 1px solid transparent;
  border-radius: var(--radius);
  font-size: 11px;
  color: var(--on-surface);
  cursor: grab;
  transition: all 0.15s;
}

.lib-item:hover {
  background: var(--surface-container-high);
  border-color: var(--primary);
}

.lib-item:active {
  cursor: grabbing;
}

.lib-icon {
  font-family: "JetBrains Mono", monospace;
  font-size: 10px;
  font-weight: 700;
  color: var(--primary);
  width: 16px;
  text-align: center;
  flex-shrink: 0;
}

/* 画布 */
.fbd-canvas-wrapper {
  flex: 1;
  min-width: 0;
  outline: none;
}

.fbd-canvas {
  width: 100%;
  height: 100%;
}

/* 属性面板 */
.fbd-properties {
  width: 220px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-left: 1px solid var(--outline-variant);
}

.properties-header {
  padding: 8px 12px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
}

.properties-content {
  padding: 8px;
}

.prop-row {
  display: grid;
  grid-template-columns: 50px 1fr;
  align-items: center;
  gap: 6px;
  margin-bottom: 8px;
}

.prop-label {
  font-size: 11px;
  color: var(--on-surface-variant);
  text-align: right;
}

.prop-value {
  font-size: 11px;
  font-family: "JetBrains Mono", monospace;
  color: var(--primary);
}

.prop-small {
  font-size: 10px;
  word-break: break-all;
  color: var(--on-surface-variant);
}

.prop-input {
  width: 100%;
  padding: 2px 6px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 11px;
  font-family: "JetBrains Mono", monospace;
  outline: none;
}

.prop-input:focus {
  border-color: var(--primary);
}

.delete-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  width: 100%;
  padding: 6px 8px;
  margin-top: 8px;
  background: none;
  border: 1px solid var(--error);
  border-radius: var(--radius);
  color: var(--error);
  font-size: 11px;
  cursor: pointer;
  transition: background 0.15s;
}

.delete-btn:hover {
  background: var(--error-container, rgba(244, 67, 54, 0.1));
}
.delete-btn .material-symbols-outlined {
  font-size: 14px;
}
</style>
