<template>
  <div class="ld-editor-layout">
    <LadderToolbox @add-element="handleAddElement" />

    <div
      class="ld-canvas-wrapper"
      ref="wrapperRef"
      tabindex="0"
      @drop="onDrop"
      @dragover="onDragOver"
      @keydown="onKeyDown"
    >
      <div class="ld-canvas">
        <VueFlow
          v-if="flowReady"
          :nodes="allNodes"
          :edges="allEdges"
          :node-types="nodeTypes"
          :default-edge-options="defaultEdgeOptions"
          :nodes-draggable="true"
          :nodes-connectable="true"
          :elements-selectable="true"
          :pan-on-drag="true"
          :zoom-on-scroll="true"
          :zoom-on-pinch="true"
          :min-zoom="0.2"
          :max-zoom="3"
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

      <div class="rung-bar">
        <button
          v-for="(rung, idx) in rungs"
          :key="rung.id"
          class="rung-tab"
          :class="{ active: currentRungIndex === idx }"
          @click="switchRung(idx)"
        >
          Rung {{ idx + 1 }}
        </button>
        <button class="rung-add" @click="addRung">
          <span class="material-symbols-outlined">add</span>
        </button>
        <div class="rung-spacer" />
        <button
          class="rail-toggle"
          :class="{ locked: railsLocked }"
          @click="toggleRailsLock"
          :title="railsLocked ? '解锁电源轨（可拖动）' : '锁定电源轨'"
        >
          <span class="material-symbols-outlined">{{
            railsLocked ? "lock" : "lock_open"
          }}</span>
          <span class="rail-toggle-label">{{
            railsLocked ? "锁定" : "解锁"
          }}</span>
        </button>
      </div>
    </div>

    <div class="ld-properties" v-if="selectedNode || selectedEdge">
      <div class="properties-header">
        <span class="panel-title">{{
          selectedEdge
            ? "连线属性"
            : selectedNode.type === "powerRail"
              ? "电源轨属性"
              : "属性"
        }}</span>
      </div>
      <div class="properties-content">
        <!-- 连线属性 -->
        <template v-if="selectedEdge">
          <div class="prop-row">
            <span class="prop-label">类型</span>
            <span class="prop-value">连线</span>
          </div>
          <div class="prop-row">
            <span class="prop-label">来源</span>
            <span class="prop-value prop-mono">{{ selectedEdge.source }}</span>
          </div>
          <div class="prop-row">
            <span class="prop-label">目标</span>
            <span class="prop-value prop-mono">{{ selectedEdge.target }}</span>
          </div>
          <button class="delete-btn" @click="deleteSelected">
            <span class="material-symbols-outlined">delete</span>
            删除连线
          </button>
        </template>
        <!-- 节点属性 -->
        <template v-else-if="selectedNode">
          <div class="prop-row">
            <span class="prop-label">类型</span>
            <span class="prop-value">{{
              selectedNode.type === "powerRail"
                ? selectedNode.data?.side === "left"
                  ? "L+ 左轨"
                  : "N 右轨"
                : selectedNode.type
            }}</span>
          </div>
          <template v-if="selectedNode.type === 'powerRail'">
            <div class="prop-row">
              <span class="prop-label">端子数</span>
              <input
                class="prop-input"
                type="number"
                min="1"
                max="64"
                :value="selectedNode.data?.portCount || 16"
                @input="
                  updateRailPortCount(
                    ($event.target as HTMLInputElement).valueAsNumber,
                  )
                "
              />
            </div>
          </template>
          <template v-else>
            <div class="prop-row">
              <span class="prop-label">变量</span>
              <input
                class="prop-input"
                :value="selectedNode.data?.variable || ''"
                @input="
                  updateNodeVariable(($event.target as HTMLInputElement).value)
                "
                placeholder="输入变量名"
              />
            </div>
            <div class="prop-row" v-if="selectedNode.data?.label !== undefined">
              <span class="prop-label">标签</span>
              <input
                class="prop-input"
                :value="selectedNode.data?.label || ''"
                @input="
                  updateNodeLabel(($event.target as HTMLInputElement).value)
                "
                placeholder="输入标签"
              />
            </div>
            <button class="delete-btn" @click="deleteSelected">
              <span class="material-symbols-outlined">delete</span>
              删除节点
            </button>
          </template>
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
import LadderToolbox from "./LadderToolbox.vue";

// ========== 节点组件 ==========

const ContactNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () =>
      h("div", { class: "ld-node contact-node" }, [
        h("div", { class: "node-label" }, props.data?.variable || "???"),
        h(
          "div",
          { class: "node-symbol" },
          !props.data?.negated ? "─┤ ├─" : "─┤/├─",
        ),
        h(Handle, { type: "target", position: Position.Left }),
        h(Handle, { type: "source", position: Position.Right }),
      ]);
  },
});

const CoilNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () =>
      h("div", { class: "ld-node coil-node" }, [
        h("div", { class: "node-label" }, props.data?.variable || "???"),
        h(
          "div",
          { class: "node-symbol" },
          !props.data?.negated ? "─( )─" : "─(/)─",
        ),
        h(Handle, { type: "target", position: Position.Left }),
        h(Handle, { type: "source", position: Position.Right }),
      ]);
  },
});

const BlockNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () => {
      const inputs = props.data?.inputs || ["IN"];
      const outputs = props.data?.outputs || ["Q"];
      return h("div", { class: "ld-node block-node" }, [
        h("div", { class: "block-header" }, props.data?.label || "FB"),
        h("div", { class: "block-body" }, [
          h(
            "div",
            { class: "block-inputs" },
            inputs.map((input: string, i: number) =>
              h("div", { class: "block-pin", key: "i" + i }, [
                h(Handle, {
                  type: "target",
                  position: Position.Left,
                  id: "in-" + i,
                  style: { top: (i + 1) * 25 + "px" },
                }),
                h("span", {}, input),
              ]),
            ),
          ),
          h(
            "div",
            { class: "block-outputs" },
            outputs.map((output: string, i: number) =>
              h("div", { class: "block-pin", key: "o" + i }, [
                h(Handle, {
                  type: "source",
                  position: Position.Right,
                  id: "out-" + i,
                  style: { top: (i + 1) * 25 + "px" },
                }),
                h("span", {}, output),
              ]),
            ),
          ),
        ]),
      ]);
    };
  },
});

const RAIL_PORT_COUNT = ref(16);

const PowerRailNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    return () => {
      const isLeft = props.data?.side === "left";
      const label = isLeft ? "L+" : "N";
      const portCount = props.data?.portCount || RAIL_PORT_COUNT.value;
      return h("div", { class: "ld-node power-rail-node" }, [
        h("div", { class: "rail-label" }, label),
        h(
          "div",
          { class: "rail-body" },
          Array.from({ length: portCount }, (_, i) =>
            h("div", { class: "rail-port", key: i }, [
              h("div", { class: "rail-dot" }),
              isLeft
                ? h(Handle, {
                    type: "source",
                    position: Position.Right,
                    id: "p-" + i,
                  })
                : h(Handle, {
                    type: "target",
                    position: Position.Left,
                    id: "p-" + i,
                  }),
            ]),
          ),
        ),
      ]);
    };
  },
});

const nodeTypes = {
  contact: markRaw(ContactNode),
  coil: markRaw(CoilNode),
  block: markRaw(BlockNode),
  powerRail: markRaw(PowerRailNode),
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

// 左右电源轨（全局共享）
const leftRailId = computed(() => `rail-left`);
const rightRailId = computed(() => `rail-right`);

// 每个 Rung 只存元件节点和边（不含电源轨）
interface RungData {
  id: string;
  nodes: Node[];
  edges: Edge[];
}

const rungs = ref<RungData[]>([]);
const currentRungIndex = ref(0);
const selectedNode = ref<Node | null>(null);
const selectedEdge = ref<Edge | null>(null);
const railsLocked = ref(true);
const railLeftX = ref(0);
const railRightX = ref(500);

const RUNG_HEIGHT = 120;
const RAIL_X_LEFT_DEFAULT = 0;
const RAIL_X_RIGHT_DEFAULT = 500;
const ELEMENT_Y_BASE = 30;

// 电源轨节点
const railNodes = computed<Node[]>(() => {
  const totalHeight = Math.max(2, rungs.value.length) * RUNG_HEIGHT;
  const isDraggable = !railsLocked.value;
  return [
    {
      id: leftRailId.value,
      type: "powerRail",
      position: { x: railLeftX.value, y: 0 },
      data: { side: "left", portCount: RAIL_PORT_COUNT.value },
      draggable: isDraggable,
      selectable: true,
      style: { height: totalHeight + "px" },
    },
    {
      id: rightRailId.value,
      type: "powerRail",
      position: { x: railRightX.value, y: 0 },
      data: { side: "right", portCount: RAIL_PORT_COUNT.value },
      draggable: isDraggable,
      selectable: true,
      style: { height: totalHeight + "px" },
    },
  ];
});

// 所有节点 = 电源轨 + 各 Rung 元件
const allNodes = computed<Node[]>(() => {
  const nodes: Node[] = [...railNodes.value];
  for (let ri = 0; ri < rungs.value.length; ri++) {
    const rung = rungs.value[ri];
    const yOffset = ri * RUNG_HEIGHT + ELEMENT_Y_BASE;
    for (const node of rung.nodes) {
      nodes.push({
        ...node,
        position: {
          x: node.position.x,
          y: node.position.y + yOffset,
        },
      });
    }
  }
  return nodes;
});

// 所有边 = 各 Rung 边（不含自动轨道边，用户手动连接）
const allEdges = computed<Edge[]>(() => {
  const edges: Edge[] = [];
  for (let ri = 0; ri < rungs.value.length; ri++) {
    const rung = rungs.value[ri];
    // 元件之间的边
    for (const edge of rung.edges) {
      edges.push(edge);
    }
  }
  return edges;
});

function getSortedRungNodes(rung: RungData): Node[] {
  return [...rung.nodes].sort((a, b) => a.position.x - b.position.x);
}

// ========== 生命周期 ==========

onMounted(async () => {
  loadRungs();
  if (rungs.value.length === 0) {
    addRung();
  }
  await nextTick();
  flowReady.value = true;
  await nextTick();
  fitView({ padding: 0.2 });
});

// ========== 持久化 ==========

function loadRungs() {
  try {
    if (props.tab.content) {
      const data = JSON.parse(props.tab.content);
      if (data.rungs && Array.isArray(data.rungs)) {
        rungs.value = data.rungs;
      }
      if (typeof data.railPortCount === "number") {
        RAIL_PORT_COUNT.value = data.railPortCount;
      }
      if (typeof data.railLeftX === "number") railLeftX.value = data.railLeftX;
      if (typeof data.railRightX === "number")
        railRightX.value = data.railRightX;
      if (typeof data.railsLocked === "boolean")
        railsLocked.value = data.railsLocked;
      return;
    }
  } catch {
    /* 忽略 */
  }
  rungs.value = [];
}

function saveRungs() {
  const content = JSON.stringify({
    rungs: rungs.value,
    railPortCount: RAIL_PORT_COUNT.value,
    railLeftX: railLeftX.value,
    railRightX: railRightX.value,
    railsLocked: railsLocked.value,
  });
  editorStore.updateTabContent(props.tab.id, content);
}

// ========== Rung 管理 ==========

function switchRung(idx: number) {
  currentRungIndex.value = idx;
  selectedNode.value = null;
  selectedEdge.value = null;
}

function addRung() {
  const ts = Date.now();
  rungs.value.push({
    id: `rung-${ts}`,
    nodes: [],
    edges: [],
  });
  currentRungIndex.value = rungs.value.length - 1;
  saveRungs();
}

// ========== 元件操作 ==========

function handleAddElement(elementType: string, dropX?: number, dropY?: number) {
  const rung = rungs.value[currentRungIndex.value];
  if (!rung) return;

  const nodeId = `node-${Date.now()}`;
  const sorted = getSortedRungNodes(rung);
  const lastNode = sorted[sorted.length - 1];

  let nodeType = elementType;
  let data: any = {};

  switch (elementType) {
    case "contact":
      data = { variable: "", negated: false };
      break;
    case "contact_nc":
      nodeType = "contact";
      data = { variable: "", negated: true };
      break;
    case "coil":
      data = { variable: "", negated: false };
      break;
    case "coil_nc":
      nodeType = "coil";
      data = { variable: "", negated: true };
      break;
    case "coil_set":
      nodeType = "coil";
      data = { variable: "", coilType: "set" };
      break;
    case "coil_reset":
      nodeType = "coil";
      data = { variable: "", coilType: "reset" };
      break;
    case "functionBlock":
      nodeType = "block";
      data = { label: "TON", inputs: ["IN", "PT"], outputs: ["Q", "ET"] };
      break;
    default:
      data = { variable: "" };
  }

  let posX: number;
  let posY: number;
  if (dropX !== undefined && dropY !== undefined) {
    posX = dropX;
    posY = dropY - ELEMENT_Y_BASE;
    // 根据 y 偏移量判断属于哪个 Rung
    const rungIdx = Math.max(
      0,
      Math.floor((dropY - ELEMENT_Y_BASE) / RUNG_HEIGHT),
    );
    if (rungIdx !== currentRungIndex.value && rungIdx < rungs.value.length) {
      currentRungIndex.value = rungIdx;
    }
  } else {
    posX = lastNode ? lastNode.position.x + 120 : 100;
    posY = 0;
  }

  const newNode: Node = {
    id: nodeId,
    type: nodeType,
    position: { x: posX, y: posY },
    data,
  };

  rung.nodes.push(newNode);
  saveRungs();
}

// ========== 连线处理 ==========

function onConnect(connection: Connection) {
  const rung = rungs.value[currentRungIndex.value];
  if (!rung || !connection.source || !connection.target) return;
  // 允许连接到电源轨
  rung.edges.push({
    id: `e-${connection.source}-${connection.target}-${Date.now()}`,
    source: connection.source,
    target: connection.target,
    sourceHandle: connection.sourceHandle || undefined,
    targetHandle: connection.targetHandle || undefined,
    type: "smoothstep",
  });
  saveRungs();
}

// ========== 节点变更 ==========

function onNodesChange(changes: NodeChange[]) {
  for (const change of changes) {
    if (change.type === "position" && change.position) {
      // 电源轨位置更新（解锁状态）
      if (change.id === leftRailId.value) {
        railLeftX.value = change.position.x;
        saveRungs();
        continue;
      }
      if (change.id === rightRailId.value) {
        railRightX.value = change.position.x;
        saveRungs();
        continue;
      }
      // 元件位置更新
      for (const rung of rungs.value) {
        const node = rung.nodes.find((n) => n.id === change.id);
        if (node) {
          const ri = rungs.value.indexOf(rung);
          node.position = {
            x: change.position.x,
            y: change.position.y - ri * RUNG_HEIGHT - ELEMENT_Y_BASE,
          };
          break;
        }
      }
    }
    if (change.type === "remove") {
      for (const rung of rungs.value) {
        const idx = rung.nodes.findIndex((n) => n.id === change.id);
        if (idx >= 0) {
          rung.nodes.splice(idx, 1);
          rung.edges = rung.edges.filter(
            (e) => e.source !== change.id && e.target !== change.id,
          );
          break;
        }
      }
      if (selectedNode.value?.id === change.id) selectedNode.value = null;
    }
  }
  saveRungs();
}

function toggleRailsLock() {
  if (railsLocked.value) {
    // 解锁：允许拖动
    railsLocked.value = false;
  } else {
    // 锁定：固定位置
    railsLocked.value = true;
  }
  saveRungs();
}

function onEdgesChange(changes: EdgeChange[]) {
  for (const change of changes) {
    if (change.type === "remove") {
      for (const rung of rungs.value) {
        const idx = rung.edges.findIndex((e) => e.id === change.id);
        if (idx >= 0) {
          rung.edges.splice(idx, 1);
          break;
        }
      }
      if (selectedEdge.value?.id === change.id) selectedEdge.value = null;
    }
  }
  saveRungs();
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
  // 删除选中的边
  if (selectedEdge.value) {
    const edgeId = selectedEdge.value.id;
    for (const rung of rungs.value) {
      const idx = rung.edges.findIndex((e) => e.id === edgeId);
      if (idx >= 0) {
        rung.edges.splice(idx, 1);
        selectedEdge.value = null;
        saveRungs();
        return;
      }
    }
    selectedEdge.value = null;
    return;
  }
  // 删除选中的节点
  if (!selectedNode.value) return;
  if (selectedNode.value.type === "powerRail") return;
  const nodeId = selectedNode.value.id;
  for (const rung of rungs.value) {
    const idx = rung.nodes.findIndex((n) => n.id === nodeId);
    if (idx >= 0) {
      rung.nodes.splice(idx, 1);
      rung.edges = rung.edges.filter(
        (e) => e.source !== nodeId && e.target !== nodeId,
      );
      selectedNode.value = null;
      saveRungs();
      return;
    }
  }
}

// ========== 属性更新 ==========

function updateNodeVariable(value: string) {
  if (!selectedNode.value) return;
  for (const rung of rungs.value) {
    const idx = rung.nodes.findIndex((n) => n.id === selectedNode.value!.id);
    if (idx >= 0) {
      rung.nodes[idx] = {
        ...rung.nodes[idx],
        data: { ...rung.nodes[idx].data, variable: value },
      };
      rung.nodes = [...rung.nodes];
      selectedNode.value = rung.nodes[idx];
      saveRungs();
      return;
    }
  }
}

function updateNodeLabel(value: string) {
  if (!selectedNode.value) return;
  for (const rung of rungs.value) {
    const idx = rung.nodes.findIndex((n) => n.id === selectedNode.value!.id);
    if (idx >= 0) {
      rung.nodes[idx] = {
        ...rung.nodes[idx],
        data: { ...rung.nodes[idx].data, label: value },
      };
      rung.nodes = [...rung.nodes];
      selectedNode.value = rung.nodes[idx];
      saveRungs();
      return;
    }
  }
}

function updateRailPortCount(count: number) {
  if (!selectedNode.value) return;
  const clamped = Math.max(1, Math.min(64, count || 16));
  RAIL_PORT_COUNT.value = clamped;
  // 更新选中的轨道数据
  selectedNode.value = {
    ...selectedNode.value,
    data: { ...selectedNode.value.data, portCount: clamped },
  };
  saveRungs();
}

// ========== 拖放 ==========

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

  const ldElementTypes = [
    "contact",
    "contact_nc",
    "coil",
    "coil_nc",
    "coil_set",
    "coil_reset",
    "functionBlock",
  ];

  const jsonRaw = event.dataTransfer.getData("application/json");
  if (jsonRaw) {
    try {
      const item = JSON.parse(jsonRaw);
      if (ldElementTypes.includes(item.type)) {
        handleAddElement(item.type, position.x, position.y);
        return;
      }
      if (
        (item.type === "function" || item.type === "functionBlock") &&
        item.name
      ) {
        handleLibraryDrop(item, position.x, position.y);
        return;
      }
    } catch {
      /* 忽略 */
    }
  }

  const textRaw = event.dataTransfer.getData("text/plain");
  if (textRaw) {
    try {
      const item = JSON.parse(textRaw);
      if (item.type && ldElementTypes.includes(item.type)) {
        handleAddElement(item.type, position.x, position.y);
      }
    } catch {
      /* 忽略 */
    }
  }
}

function handleLibraryDrop(
  libItem: { name: string; type: string; category?: string; desc?: string },
  dropX?: number,
  dropY?: number,
) {
  const rung = rungs.value[currentRungIndex.value];
  if (!rung) return;

  const nodeId = `node-${Date.now()}`;
  const sorted = getSortedRungNodes(rung);
  const lastNode = sorted[sorted.length - 1];

  const data: any = {
    label: libItem.name,
    libType: libItem.type,
    category: libItem.category || "",
    inputs: [],
    outputs: [],
  };

  const fbPins: Record<string, { inputs: string[]; outputs: string[] }> = {
    TON: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
    TOF: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
    TP: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
    TONR: { inputs: ["IN", "PT"], outputs: ["Q", "ET"] },
    CTU: { inputs: ["CU", "R", "PV"], outputs: ["Q", "CV"] },
    CTD: { inputs: ["CD", "LD", "PV"], outputs: ["Q", "CV"] },
    CTUD: {
      inputs: ["CU", "CD", "R", "LD", "PV"],
      outputs: ["QU", "QD", "CV"],
    },
    SR: { inputs: ["S1", "R"], outputs: ["Q1"] },
    RS: { inputs: ["S", "R1"], outputs: ["Q1"] },
    R_TRIG: { inputs: ["CLK"], outputs: ["Q"] },
    F_TRIG: { inputs: ["CLK"], outputs: ["Q"] },
    SEMA: { inputs: ["S", "R"], outputs: ["Q1"] },
    MC_Power: { inputs: ["Enable", "Axis"], outputs: ["Done", "Error"] },
    MC_Home: { inputs: ["Execute", "Axis"], outputs: ["Done", "Error"] },
    MC_Stop: { inputs: ["Execute", "Axis"], outputs: ["Done", "Error"] },
    MC_MoveAbsolute: {
      inputs: ["Execute", "Axis", "Position", "Velocity"],
      outputs: ["Done", "Error"],
    },
    MC_MoveRelative: {
      inputs: ["Execute", "Axis", "Distance", "Velocity"],
      outputs: ["Done", "Error"],
    },
    PID: {
      inputs: ["Enable", "Setpoint", "Input"],
      outputs: ["Output", "Error"],
    },
  };

  const pins = fbPins[libItem.name];
  if (pins) {
    data.inputs = pins.inputs;
    data.outputs = pins.outputs;
  } else {
    data.inputs = ["EN"];
    data.outputs = ["ENO"];
  }

  let posX: number;
  let posY: number;
  if (dropX !== undefined && dropY !== undefined) {
    posX = dropX;
    posY = dropY - ELEMENT_Y_BASE;
  } else {
    posX = lastNode ? lastNode.position.x + 150 : 100;
    posY = 0;
  }

  const newNode: Node = {
    id: nodeId,
    type: "block",
    position: { x: posX, y: posY },
    data,
  };
  rung.nodes.push(newNode);
  saveRungs();
}
</script>

<style>
.vue-flow {
  background: #0f172a;
}

.ld-node {
  padding: 8px 12px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  font-size: 11px;
  color: var(--on-surface);
  text-align: center;
  min-width: 80px;
}

.vue-flow__node.selected .ld-node {
  border-color: var(--primary);
  box-shadow: 0 0 0 2px var(--primary-container);
}

.vue-flow__edge.selected .vue-flow__edge-path {
  stroke: var(--error) !important;
  stroke-width: 3;
}

.vue-flow__edge:hover .vue-flow__edge-path {
  stroke: var(--primary);
  stroke-width: 2.5;
}

.vue-flow__handle {
  width: 12px;
  height: 12px;
  border: 2px solid var(--primary);
  background: var(--surface-container);
}

.vue-flow__handle:hover {
  background: var(--primary);
  transform: scale(1.3);
}

.contact-node .node-symbol,
.coil-node .node-symbol {
  font-family: "JetBrains Mono", monospace;
  font-size: 10px;
  color: var(--secondary);
  margin-top: 2px;
}

.contact-node .node-label,
.coil-node .node-label {
  font-size: 10px;
  color: var(--primary);
  font-family: "JetBrains Mono", monospace;
}

.block-node {
  min-width: 120px;
  padding: 0;
}
.block-header {
  padding: 4px 8px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  font-weight: 700;
  font-size: 11px;
  border-radius: var(--radius) var(--radius) 0 0;
}
.block-body {
  display: flex;
  justify-content: space-between;
  padding: 4px 0;
}
.block-pin {
  padding: 2px 8px;
  font-size: 10px;
  color: var(--on-surface-variant);
}

.power-rail-node {
  width: 24px;
  padding: 0;
  background: none;
  border: none;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.rail-label {
  font-size: 9px;
  font-weight: 700;
  color: var(--secondary);
  margin-bottom: 2px;
  user-select: none;
}

.rail-body {
  display: flex;
  flex-direction: column;
  gap: 8px;
  position: relative;
  width: 8px;
  align-items: center;
  flex: 1;
}

.rail-port {
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
}

.rail-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--secondary);
  border: 2px solid var(--surface-container);
  z-index: 1;
  flex-shrink: 0;
}
</style>

<style scoped>
.ld-editor-layout {
  width: 100%;
  height: 100%;
  display: flex;
  overflow: hidden;
}

.ld-canvas-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.ld-canvas {
  flex: 1;
  min-height: 0;
}

.rung-bar {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 4px 8px;
  background: var(--surface-container-high);
  border-top: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.rung-tab {
  padding: 4px 12px;
  background: none;
  border: 1px solid transparent;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 11px;
  cursor: pointer;
}

.rung-tab:hover {
  background: var(--surface-variant);
}

.rung-tab.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-color: var(--primary);
}

.rung-add {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: none;
  border: 1px dashed var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
}

.rung-add:hover {
  border-color: var(--primary);
  color: var(--primary);
}
.rung-add .material-symbols-outlined {
  font-size: 14px;
}

.rung-spacer {
  flex: 1;
}

.rail-toggle {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px 8px;
  background: none;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 10px;
  cursor: pointer;
  transition: all 0.15s;
}

.rail-toggle:hover {
  border-color: var(--primary);
  color: var(--primary);
}

.rail-toggle.locked {
  background: var(--primary-container);
  border-color: var(--primary);
  color: var(--on-primary-container);
}

.rail-toggle .material-symbols-outlined {
  font-size: 14px;
}

.rail-toggle-label {
  font-family: "Inter", sans-serif;
}

.ld-properties {
  width: 220px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-left: 1px solid var(--outline-variant);
}

.properties-header {
  padding: var(--padding-sm);
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

.properties-content {
  padding: var(--padding-sm);
}

.prop-row {
  display: grid;
  grid-template-columns: 60px 1fr;
  align-items: center;
  gap: var(--padding-sm);
  margin-bottom: var(--padding-sm);
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

.prop-mono {
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
