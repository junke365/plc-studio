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
        <div class="lib-section">
          <div class="section-label">OpenCV5 视觉</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              v-for="it in fbdVisionLibItems"
              :key="it.name"
              @dragstart="onLibDrag($event, it.name)"
            >
              <span class="lib-icon">{{ it.emoji }}</span><span>{{ it.label }}</span>
            </div>
          </div>
        </div>
        <div class="lib-section">
          <div class="section-label">ROS 2</div>
          <div class="lib-items">
            <div
              class="lib-item"
              draggable="true"
              v-for="it in fbdRos2LibItems"
              :key="it.name"
              @dragstart="onLibDrag($event, it.name)"
            >
              <span class="lib-icon">{{ it.emoji }}</span><span>{{ it.label }}</span>
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
          <div
            class="prop-row"
            v-if="
              selectedNode.data?.label !== undefined &&
              !['fbdVision', 'fbdCamera', 'fbdImageView', 'fbdRos2'].includes(
                selectedNode.type,
              )
            "
          >
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
          <template v-if="selectedNode && nodeParamList.length">
            <div class="prop-row" v-for="p in nodeParamList" :key="p.key">
              <span class="prop-label">{{ p.label }}</span>
              <template v-if="p.type === 'select'">
                <select
                  class="prop-input"
                  :value="
                    (selectedNode.data?.params || {})[p.key] ?? p.default
                  "
                  @change="
                    updateNodeParam(
                      p.key,
                      ($event.target as HTMLSelectElement).value,
                    )
                  "
                >
                  <option
                    v-for="o in (typeof p.options === 'function'
                      ? p.options()
                      : p.options)"
                    :key="o.value"
                    :value="o.value"
                  >
                    {{ o.label }}
                  </option>
                </select>
                <button
                  v-if="p.key === 'model'"
                  class="prop-mini-btn"
                  type="button"
                  @click="refreshVisionModels"
                >
                  刷新
                </button>
              </template>
              <template v-else-if="p.type === 'image'">
                <input
                  type="file"
                  accept="image/png,image/jpeg"
                  class="prop-input"
                  @change="onTemplateUpload"
                />
                <span class="prop-small" v-if="(selectedNode.data?.params || {}).template">
                  已设置
                </span>
              </template>
              <template v-else-if="p.type === 'bool'">
                <input
                  type="checkbox"
                  class="prop-input"
                  :checked="!!(selectedNode.data?.params || {})[p.key]"
                  @change="
                    updateNodeParam(
                      p.key,
                      ($event.target as HTMLInputElement).checked,
                    )
                  "
                />
              </template>
              <template v-else-if="p.type === 'text' || p.type === 'textarea'">
                <textarea
                  v-if="p.type === 'textarea'"
                  rows="3"
                  class="prop-input prop-textarea"
                  :value="(selectedNode.data?.params || {})[p.key] ?? p.default"
                  @input="
                    updateNodeParam(
                      p.key,
                      ($event.target as HTMLTextAreaElement).value,
                    )
                  "
                ></textarea>
                <input
                  v-else
                  class="prop-input"
                  :value="(selectedNode.data?.params || {})[p.key] ?? p.default"
                  @input="
                    updateNodeParam(
                      p.key,
                      ($event.target as HTMLInputElement).value,
                    )
                  "
                  placeholder="值"
                />
              </template>
              <template v-else>
                <input
                  type="number"
                  class="prop-input"
                  :value="
                    (selectedNode.data?.params || {})[p.key] ?? p.default
                  "
                  @input="
                    updateNodeParam(
                      p.key,
                      ($event.target as HTMLInputElement).value,
                    )
                  "
                />
              </template>
            </div>
            <div class="prop-hint" v-if="selectedNode.type === 'fbdVision'">
              输入引脚连上信号源（相机/常量）后自动处理；IMAGE 引脚接收图像。
            </div>
            <div class="prop-hint" v-if="selectedNode.type === 'fbdRos2'">
              ROS2 块走后端执行：发布/调服务用 /api/ros2，订阅用 /ws/ros2 实时转发；
              需要 ROS 2 环境已连接（可先到 工具 → ROS 2 调试 查看状态）。
            </div>
          </template>
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
  watch,
  reactive,
  onMounted,
  onBeforeUnmount,
  onUnmounted,
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
import {
  processVisionImage,
  getVisionModels,
  type VisionOp,
} from "../../../api/vision";
import { VISION_LIB } from "../../library/visionLib";
import { ROS2_LIB } from "../../library/ros2Lib";
import {
  ros2Publish,
  ros2Call,
  ROS2_WS_URL,
} from "../../../api/ros2";

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
  VISION_CAMERA: { inputs: [], outputs: ["IMAGE"] },
  VISION_IMAGE_VIEW: { inputs: ["IMAGE"], outputs: [] },
  VISION_GRAYSCALE: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "GRAY"] },
  VISION_BLUR: { inputs: ["EN", "IMAGE", "KSIZE"], outputs: ["ENO", "OUT"] },
  VISION_THRESHOLD: { inputs: ["EN", "IMAGE", "THRESH", "MAX"], outputs: ["ENO", "BINARY"] },
  VISION_CANNY: { inputs: ["EN", "IMAGE", "LOW", "HIGH"], outputs: ["ENO", "EDGE"] },
  VISION_MATCH_TEMPLATE: { inputs: ["EN", "IMAGE", "TEMPLATE", "THRESH"], outputs: ["ENO", "X", "Y", "SCORE", "FOUND"] },
  VISION_FIND_CONTOURS: { inputs: ["EN", "IMAGE", "MIN_AREA"], outputs: ["ENO", "OUT", "COUNT", "RECTS"] },
  VISION_CVT_COLOR: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_FLIP: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_CROP: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_ERODE: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_DILATE: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_MORPH: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_MEDIAN_BLUR: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_BILATERAL: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_ADAPTIVE_THRESHOLD: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_SOBEL: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_LAPLACIAN: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_EQUALIZE_HIST: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_PYRAMID: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_BRIGHTNESS: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT"] },
  VISION_INRANGE: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "MASK"] },
  VISION_HOUGH_LINES: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT", "COUNT"] },
  VISION_HOUGH_CIRCLES: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT", "COUNT"] },
  VISION_YOLO26: { inputs: ["EN", "IMAGE"], outputs: ["ENO", "OUT", "COUNT", "BOXES"] },
  // ROS 2 功能块
  ROS2_PUBLISH: { inputs: ["EN", "TOPIC"], outputs: ["ENO", "DONE", "COUNT"] },
  ROS2_SUBSCRIBE: { inputs: ["EN"], outputs: ["ENO", "DATA", "AVAIL", "COUNT"] },
  ROS2_SERVICE: { inputs: ["EN"], outputs: ["ENO", "DONE", "RESP", "CODE"] },
};

// ========== OpenCV5 视觉信号流 ==========
// 信号值约定：image 信号为不带 data: 前缀的 base64 PNG/JPEG；
// 数值/其他信号为原始 JS 值。键: `${nodeId}:${PIN}`

const visionSignals = reactive<Record<string, any>>({});
// 参数修改后触发对应节点重新处理
const visionBump = reactive<Record<string, number>>({});
// ROS2 参数修改后触发对应节点重新执行/重新订阅
const ros2Bump = reactive<Record<string, number>>({});

function looksLikeImage(v: any): boolean {
  return (
    typeof v === "string" &&
    (v.startsWith("iVBOR") || v.startsWith("/9j/"))
  );
}

// 向某节点的输出引脚写值，并通过连线传播到下游节点输入引脚
function emitVisionSignal(nodeId: string, pinName: string, value: any) {
  const key = `${nodeId}:${pinName}`;
  if (visionSignals[key] === value) return;
  visionSignals[key] = value;
  const node = nodes.value.find((n) => n.id === nodeId);
  if (!node) return;
  const outIdx = (node.data?.outputs || []).indexOf(pinName);
  if (outIdx < 0) return;
  for (const e of edges.value) {
    if (e.source !== nodeId) continue;
    const sh = e.sourceHandle;
    // 只传播与本输出引脚匹配的连线（无 handle id / 默认 "source" 视为单输出）
    if (sh && sh !== "source") {
      const shIdx = Number(String(sh).replace(/^out-/, ""));
      if (!Number.isNaN(shIdx) && shIdx !== outIdx) continue;
    }
    const target = nodes.value.find((n) => n.id === e.target);
    if (!target) continue;
    const th = e.targetHandle;
    const inIdx = th && th !== "target" ? Number(String(th).replace(/^in-/, "")) : 0;
    if (Number.isNaN(inIdx)) continue;
    const inPin = (target.data?.inputs || [])[inIdx];
    if (inPin) visionSignals[`${e.target}:${inPin}`] = value;
  }
}

// 每个视觉功能块的属性（属性面板 + 处理参数）
interface VisionParamDef {
  key: string;
  label: string;
  type: "number" | "select" | "image" | "bool" | "text" | "textarea";
  default?: any;
  options?:
    | Array<{ value: string; label: string }>
    | (() => Array<{ value: string; label: string }>);
}

// 后端可用的 YOLO 模型列表（来自 GET /api/vision/models），用于 model 下拉
const visionModels = ref<string[]>([]);
const visionModelsDir = ref("");

const VISION_PARAMS: Record<string, VisionParamDef[]> = {
  VISION_CAMERA: [
    { key: "width", label: "宽度", type: "number", default: 640 },
    { key: "height", label: "高度", type: "number", default: 480 },
    { key: "fps", label: "输出帧率 (fps)", type: "number", default: 4 },
  ],
  VISION_GRAYSCALE: [],
  VISION_BLUR: [
    { key: "ksize", label: "核大小", type: "number", default: 5 },
    { key: "sigma", label: "标准差 σ", type: "number", default: 0 },
  ],
  VISION_THRESHOLD: [
    { key: "thresh", label: "阈值", type: "number", default: 127 },
    { key: "max", label: "最大值", type: "number", default: 255 },
    {
      key: "kind",
      label: "阈值类型",
      type: "select",
      default: "binary",
      options: [
        { value: "binary", label: "BINARY 二值" },
        { value: "binary_inv", label: "BINARY_INV 反相" },
        { value: "trunc", label: "TRUNC 截断" },
        { value: "tozero", label: "TOZERO 取零" },
        { value: "tozero_inv", label: "TOZERO_INV" },
        { value: "otsu", label: "OTSU 自动" },
      ],
    },
    { key: "invert", label: "反相", type: "bool", default: false },
  ],
  VISION_CANNY: [
    { key: "low", label: "低阈值", type: "number", default: 50 },
    { key: "high", label: "高阈值", type: "number", default: 150 },
    { key: "aperture", label: "孔径", type: "number", default: 3 },
  ],
  VISION_MATCH_TEMPLATE: [
    { key: "template", label: "模板图片", type: "image" },
    {
      key: "method",
      label: "匹配方法",
      type: "select",
      default: "ccoeff_normed",
      options: [
        { value: "sqdiff_normed", label: "SQDIFF_NORMED" },
        { value: "ccorr_normed", label: "CCORR_NORMED" },
        { value: "ccoeff_normed", label: "CCOEFF_NORMED" },
      ],
    },
    { key: "thresh", label: "得分阈值", type: "number", default: 0.8 },
  ],
  VISION_FIND_CONTOURS: [
    { key: "minArea", label: "最小面积", type: "number", default: 50 },
    {
      key: "mode",
      label: "轮廓模式",
      type: "select",
      default: "external",
      options: [
        { value: "external", label: "EXTERNAL 外部" },
        { value: "list", label: "LIST 全部" },
        { value: "tree", label: "TREE 层级" },
      ],
    },
    {
      key: "approx",
      label: "逼近方式",
      type: "select",
      default: "simple",
      options: [
        { value: "simple", label: "SIMPLE" },
        { value: "none", label: "NONE" },
        { value: "l1", label: "TC89_L1" },
      ],
    },
  ],
  VISION_CVT_COLOR: [
    {
      key: "code",
      label: "色彩空间",
      type: "select",
      default: "hsv",
      options: [
        { value: "hsv", label: "HSV" },
        { value: "lab", label: "Lab" },
        { value: "luv", label: "Luv" },
        { value: "ycc", label: "YCrCb" },
        { value: "gray", label: "灰度" },
      ],
    },
  ],
  VISION_FLIP: [
    {
      key: "flipCode",
      label: "翻转方向",
      type: "select",
      default: "1",
      options: [
        { value: "0", label: "垂直 (上下)" },
        { value: "1", label: "水平 (左右)" },
        { value: "-1", label: "双轴" },
      ],
    },
  ],
  VISION_CROP: [
    { key: "x", label: "起点 X", type: "number", default: 0 },
    { key: "y", label: "起点 Y", type: "number", default: 0 },
    { key: "width", label: "宽度", type: "number", default: 200 },
    { key: "height", label: "高度", type: "number", default: 200 },
  ],
  VISION_ERODE: [
    { key: "ksize", label: "核大小", type: "number", default: 3 },
    { key: "iterations", label: "迭代次数", type: "number", default: 1 },
  ],
  VISION_DILATE: [
    { key: "ksize", label: "核大小", type: "number", default: 3 },
    { key: "iterations", label: "迭代次数", type: "number", default: 1 },
  ],
  VISION_MORPH: [
    {
      key: "op",
      label: "形态学操作",
      type: "select",
      default: "open",
      options: [
        { value: "open", label: "OPEN 开运算" },
        { value: "close", label: "CLOSE 闭运算" },
        { value: "gradient", label: "GRADIENT 梯度" },
        { value: "tophat", label: "TOPHAT 顶帽" },
        { value: "blackhat", label: "BLACKHAT 黑帽" },
      ],
    },
    { key: "ksize", label: "核大小", type: "number", default: 3 },
    { key: "iterations", label: "迭代次数", type: "number", default: 1 },
  ],
  VISION_MEDIAN_BLUR: [
    { key: "ksize", label: "核大小", type: "number", default: 5 },
  ],
  VISION_BILATERAL: [
    { key: "d", label: "直径", type: "number", default: 9 },
    { key: "sigmaColor", label: "色彩 σ", type: "number", default: 75 },
    { key: "sigmaSpace", label: "空间 σ", type: "number", default: 75 },
  ],
  VISION_ADAPTIVE_THRESHOLD: [
    { key: "blockSize", label: "块大小", type: "number", default: 11 },
    { key: "c", label: "常数 C", type: "number", default: 2 },
    { key: "invert", label: "反相", type: "bool", default: false },
  ],
  VISION_SOBEL: [
    { key: "dx", label: "X 阶", type: "number", default: 1 },
    { key: "dy", label: "Y 阶", type: "number", default: 0 },
    { key: "ksize", label: "核大小", type: "number", default: 3 },
  ],
  VISION_LAPLACIAN: [
    { key: "ksize", label: "核大小", type: "number", default: 3 },
  ],
  VISION_EQUALIZE_HIST: [],
  VISION_PYRAMID: [
    {
      key: "dir",
      label: "方向",
      type: "select",
      default: "down",
      options: [
        { value: "down", label: "缩小 (pyrDown)" },
        { value: "up", label: "放大 (pyrUp)" },
      ],
    },
  ],
  VISION_BRIGHTNESS: [
    { key: "alpha", label: "对比度 α", type: "number", default: 1 },
    { key: "beta", label: "亮度 β", type: "number", default: 0 },
  ],
  VISION_INRANGE: [
    { key: "lowH", label: "H 低", type: "number", default: 0 },
    { key: "highH", label: "H 高", type: "number", default: 30 },
    { key: "lowS", label: "S 低", type: "number", default: 60 },
    { key: "highS", label: "S 高", type: "number", default: 255 },
    { key: "lowV", label: "V 低", type: "number", default: 60 },
    { key: "highV", label: "V 高", type: "number", default: 255 },
  ],
  VISION_HOUGH_LINES: [
    { key: "threshold", label: "累加阈值", type: "number", default: 80 },
    { key: "minLen", label: "最小长度", type: "number", default: 40 },
    { key: "maxGap", label: "最大间距", type: "number", default: 10 },
  ],
  VISION_HOUGH_CIRCLES: [
    { key: "minDist", label: "最小距离", type: "number", default: 20 },
    { key: "param1", label: "Canny 阈值", type: "number", default: 100 },
    { key: "param2", label: "圆心阈值", type: "number", default: 30 },
    { key: "minR", label: "最小半径", type: "number", default: 5 },
    { key: "maxR", label: "最大半径", type: "number", default: 40 },
  ],
  VISION_YOLO26: [
    {
      key: "model",
      label: "模型",
      type: "select",
      default: "yolo26n.onnx",
      options: () => {
        const list = visionModels.value;
        return list.length
          ? list.map((m) => ({ value: m, label: m }))
          : [{ value: "", label: "（未找到模型，请放入 models 目录）" }];
      },
    },
    { key: "conf", label: "置信度阈值", type: "number", default: 0.25 },
    { key: "iou", label: "IoU 阈值", type: "number", default: 0.45 },
    { key: "imgsz", label: "推理尺寸", type: "number", default: 640 },
  ],
  VISION_CAMERA: [],
  VISION_IMAGE_VIEW: [],
};

function defaultVisionParams(fbdType: string): Record<string, any> {
  const params: Record<string, any> = {};
  for (const def of VISION_PARAMS[fbdType] || []) {
    params[def.key] = def.default;
  }
  return params;
}

function numOr(v: any, d: number): number {
  const n = Number(v);
  return Number.isFinite(n) ? n : d;
}

// 每个视觉功能块：后端操作管线 + 输出引脚 + 结果文本
interface VisionNodeCfg {
  imageOut?: string;
  needsTemplate?: boolean;
  ops: (params: Record<string, any>) => VisionOp[];
  outputText?: (results: any) => string;
}

const VISION_FRIENDLY: Record<string, string> = {
  VISION_CAMERA: "相机",
  VISION_IMAGE_VIEW: "图像显示",
  VISION_GRAYSCALE: "灰度化",
  VISION_BLUR: "高斯模糊",
  VISION_THRESHOLD: "阈值化",
  VISION_CANNY: "边缘检测",
  VISION_MATCH_TEMPLATE: "模板匹配",
  VISION_FIND_CONTOURS: "轮廓检测",
  VISION_CVT_COLOR: "色彩空间转换",
  VISION_FLIP: "图像翻转",
  VISION_CROP: "图像裁剪",
  VISION_ERODE: "腐蚀",
  VISION_DILATE: "膨胀",
  VISION_MORPH: "形态学操作",
  VISION_MEDIAN_BLUR: "中值滤波",
  VISION_BILATERAL: "双边滤波",
  VISION_ADAPTIVE_THRESHOLD: "自适应阈值",
  VISION_SOBEL: "Sobel 梯度",
  VISION_LAPLACIAN: "Laplacian",
  VISION_EQUALIZE_HIST: "直方图均衡",
  VISION_PYRAMID: "金字塔缩放",
  VISION_BRIGHTNESS: "亮度对比度",
  VISION_INRANGE: "颜色范围提取",
  VISION_HOUGH_LINES: "直线检测",
  VISION_HOUGH_CIRCLES: "圆检测",
  VISION_YOLO26: "YOLO26 目标检测",
};

// 视觉节点机器类型：优先 fbdType（稳定），兼容旧数据的 label
function visionTypeKey(data: any): string {
  return data?.fbdType || data?.label || "";
}

const VISION_CFG: Record<string, VisionNodeCfg> = {
  VISION_GRAYSCALE: {
    imageOut: "GRAY",
    ops: () => [{ type: "grayscale" }],
  },
  VISION_BLUR: {
    imageOut: "OUT",
    ops: (p) => [{ type: "blur", ksize: numOr(p.ksize, 5), sigma: numOr(p.sigma, 0) }],
  },
  VISION_THRESHOLD: {
    imageOut: "BINARY",
    ops: (p) => [
      { type: "grayscale" },
      { type: "threshold", thresh: numOr(p.thresh, 127), maxval: numOr(p.max, 255), kind: p.kind || "binary", invert: !!p.invert },
    ],
  },
  VISION_CANNY: {
    imageOut: "EDGE",
    ops: (p) => [
      { type: "grayscale" },
      { type: "canny", low: numOr(p.low, 50), high: numOr(p.high, 150), aperture: numOr(p.aperture, 3) },
    ],
  },
  VISION_MATCH_TEMPLATE: {
    needsTemplate: true,
    ops: (p) => [
      { type: "matchTemplate", template: p.template, method: p.method || "ccoeff_normed", threshold: numOr(p.thresh, 0.8) },
    ],
    outputText: (results) => {
      const m = results?.templateMatches?.[0];
      return m
        ? `匹配 (${m.x}, ${m.y}) 得分=${m.score}`
        : "未找到目标";
    },
  },
  VISION_FIND_CONTOURS: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "grayscale" },
      { type: "threshold", thresh: 127, maxval: 255 },
      { type: "contours", minArea: numOr(p.minArea, 50), mode: p.mode || "external", approx: p.approx || "simple" },
    ],
    outputText: (results) => `轮廓: ${results?.contours?.length ?? 0} 个`,
  },
  VISION_CVT_COLOR: {
    imageOut: "OUT",
    ops: (p) => [{ type: "cvtColor", code: (p.code as any) || "hsv" }],
  },
  VISION_FLIP: {
    imageOut: "OUT",
    ops: (p) => [{ type: "flip", flipCode: (Number(p.flipCode) || 0) as 0 | 1 | -1 }],
  },
  VISION_CROP: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "crop", x: numOr(p.x, 0), y: numOr(p.y, 0), width: numOr(p.width, 200), height: numOr(p.height, 200) },
    ],
  },
  VISION_ERODE: {
    imageOut: "OUT",
    ops: (p) => [{ type: "erode", ksize: numOr(p.ksize, 3), iterations: numOr(p.iterations, 1) }],
  },
  VISION_DILATE: {
    imageOut: "OUT",
    ops: (p) => [{ type: "dilate", ksize: numOr(p.ksize, 3), iterations: numOr(p.iterations, 1) }],
  },
  VISION_MORPH: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "morph", op: (p.op as any) || "open", ksize: numOr(p.ksize, 3), iterations: numOr(p.iterations, 1) },
    ],
  },
  VISION_MEDIAN_BLUR: {
    imageOut: "OUT",
    ops: (p) => [{ type: "medianBlur", ksize: numOr(p.ksize, 5) }],
  },
  VISION_BILATERAL: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "bilateral", d: numOr(p.d, 9), sigmaColor: numOr(p.sigmaColor, 75), sigmaSpace: numOr(p.sigmaSpace, 75) },
    ],
  },
  VISION_ADAPTIVE_THRESHOLD: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "grayscale" },
      { type: "adaptiveThreshold", blockSize: numOr(p.blockSize, 11), c: numOr(p.c, 2), invert: !!p.invert },
    ],
  },
  VISION_SOBEL: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "grayscale" },
      { type: "sobel", dx: numOr(p.dx, 1), dy: numOr(p.dy, 0), ksize: numOr(p.ksize, 3) },
    ],
  },
  VISION_LAPLACIAN: {
    imageOut: "OUT",
    ops: (p) => [{ type: "grayscale" }, { type: "laplacian", ksize: numOr(p.ksize, 3) }],
  },
  VISION_EQUALIZE_HIST: {
    imageOut: "OUT",
    ops: () => [{ type: "grayscale" }, { type: "equalizeHist" }],
  },
  VISION_PYRAMID: {
    imageOut: "OUT",
    ops: (p) => [{ type: p.dir === "up" ? "pyrUp" : "pyrDown" }],
  },
  VISION_BRIGHTNESS: {
    imageOut: "OUT",
    ops: (p) => [{ type: "brightnessContrast", alpha: numOr(p.alpha, 1), beta: numOr(p.beta, 0) }],
  },
  VISION_INRANGE: {
    imageOut: "MASK",
    ops: (p) => [
      { type: "cvtColor", code: "hsv" },
      {
        type: "inRange",
        low: [numOr(p.lowH, 0), numOr(p.lowS, 60), numOr(p.lowV, 60)],
        high: [numOr(p.highH, 30), numOr(p.highS, 255), numOr(p.highV, 255)],
        space: "hsv",
      },
    ],
  },
  VISION_HOUGH_LINES: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "grayscale" },
      { type: "houghLinesP", threshold: numOr(p.threshold, 80), minLineLength: numOr(p.minLen, 40), maxLineGap: numOr(p.maxGap, 10) },
    ],
    outputText: (results) => `直线: ${results?.lines?.length ?? 0} 条`,
  },
  VISION_HOUGH_CIRCLES: {
    imageOut: "OUT",
    ops: (p) => [
      { type: "grayscale" },
      { type: "houghCircles", minDist: numOr(p.minDist, 20), param1: numOr(p.param1, 100), param2: numOr(p.param2, 30), minRadius: numOr(p.minR, 5), maxRadius: numOr(p.maxR, 40) },
    ],
    outputText: (results) => `圆: ${results?.circles?.length ?? 0} 个`,
  },
  VISION_YOLO26: {
    imageOut: "OUT",
    ops: (p) => [
      {
        type: "yolo",
        model: p.model || "yolo26n.onnx",
        conf: numOr(p.conf, 0.25),
        iou: numOr(p.iou, 0.45),
        imgsz: numOr(p.imgsz, 640),
      },
    ],
    outputText: (results) => {
      const dets = results?.detections;
      if (!Array.isArray(dets)) return "YOLO26 未执行";
      return `检测: ${dets.length} 个目标`;
    },
  },
};

// ========== ROS 2 功能块 ==========

function isRos2Type(t: string): boolean {
  return typeof t === "string" && t.startsWith("ROS2_");
}

const ROS2_FRIENDLY: Record<string, string> = {
  ROS2_PUBLISH: "ROS2 发布",
  ROS2_SUBSCRIBE: "ROS2 订阅",
  ROS2_SERVICE: "ROS2 调服务",
};

const ROS2_PARAMS: Record<string, VisionParamDef[]> = {
  ROS2_PUBLISH: [
    { key: "topic", label: "话题", type: "text", default: "/chatter" },
    { key: "type", label: "类型", type: "text", default: "std_msgs/msg/String" },
    { key: "message", label: "消息(JSON)", type: "textarea", default: '{"data": "hello"}' },
  ],
  ROS2_SUBSCRIBE: [
    { key: "topic", label: "话题", type: "text", default: "/chatter" },
  ],
  ROS2_SERVICE: [
    { key: "service", label: "服务", type: "text", default: "/set_bool" },
    { key: "type", label: "类型", type: "text", default: "std_srvs/srv/SetBool" },
    { key: "request", label: "请求(JSON)", type: "textarea", default: '{"data": true}' },
  ],
};

function defaultRos2Params(fbdType: string): Record<string, any> {
  const out: Record<string, any> = {};
  for (const def of ROS2_PARAMS[fbdType] || []) out[def.key] = def.default;
  return out;
}

function emitRos2Outputs(nodeId: string, data: any, out: Record<string, any>) {
  for (const name of data?.outputs || []) {
    if (name in out) emitVisionSignal(nodeId, name, out[name]);
  }
}

function emitVisionNumericOutputs(nodeId: string, data: any, results: any) {
  const outputs = (data?.outputs || []) as string[];
  const m = results?.templateMatches?.[0];
  const count = results?.contours?.length ?? results?.lines?.length ?? results?.circles?.length ?? 0;
  for (const name of outputs) {
    if (name === "X") emitVisionSignal(nodeId, name, m?.x ?? 0);
    else if (name === "Y") emitVisionSignal(nodeId, name, m?.y ?? 0);
    else if (name === "SCORE") emitVisionSignal(nodeId, name, m?.score ?? 0);
    else if (name === "FOUND") emitVisionSignal(nodeId, name, m ? 1 : 0);
    else if (name === "COUNT") emitVisionSignal(nodeId, name, count);
    else if (name === "RECTS") emitVisionSignal(nodeId, name, results?.contours || []);
    else if (name === "BOXES")
      emitVisionSignal(nodeId, name, results?.detections || []);
  }
}

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
    watch(
      () => props.data?.value,
      (v: any) => emitVisionSignal(props.id as string, "VALUE", v),
      { immediate: true },
    );
    return () =>
      h("div", { class: "fbd-node fbd-constant" }, [
        h("div", { class: "fbd-const-value" }, props.data?.value || "0"),
        h(Handle, { type: "source", position: Position.Right }),
      ]);
  },
});

// OpenCV5 相机节点：打开摄像头实时显示原图，并持续输出 image 信号。
// 处理/调试请连后续视觉功能块，不在相机节点内做。
const CameraNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    const videoRef = ref<HTMLVideoElement | null>(null);
    const canvasRef = ref<HTMLCanvasElement | null>(null);
    const stream = ref<MediaStream | null>(null);
    const open = ref(false);
    const error = ref("");
    let streamTimer: number | null = null;

    function params() {
      return props.data?.params || {};
    }

    // 抓取当前帧为 base64（无 data: 前缀）
    function grabBase64(): string | null {
      const video = videoRef.value;
      const canvas = canvasRef.value;
      if (!video || !canvas || video.videoWidth === 0) return null;
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const ctx = canvas.getContext("2d");
      if (!ctx) return null;
      ctx.drawImage(video, 0, 0);
      return canvas.toDataURL("image/png").split(",")[1];
    }

    // 相机打开后按 fps 参数持续输出 image 信号
    function startStreaming() {
      stopStreaming();
      const b64 = grabBase64();
      if (b64) emitVisionSignal(props.id, "IMAGE", b64);
      const fps = Math.min(30, Math.max(1, numOr(params().fps, 4)));
      streamTimer = window.setInterval(() => {
        const b = grabBase64();
        if (b) emitVisionSignal(props.id, "IMAGE", b);
      }, 1000 / fps);
    }

    function stopStreaming() {
      if (streamTimer !== null) {
        window.clearInterval(streamTimer);
        streamTimer = null;
      }
    }

    async function toggleCamera() {
      if (open.value) {
        closeCamera();
        return;
      }
      if (!navigator.mediaDevices?.getUserMedia) {
        error.value = "当前环境不支持摄像头访问（需 HTTPS / Electron）";
        return;
      }
      try {
        const p = params();
        stream.value = await navigator.mediaDevices.getUserMedia({
          video: {
            width: { ideal: numOr(p.width, 640) },
            height: { ideal: numOr(p.height, 480) },
          },
        });
        open.value = true;
        error.value = "";
        requestAnimationFrame(() => {
          if (videoRef.value && stream.value) {
            videoRef.value.srcObject = stream.value;
            videoRef.value.play().catch(() => {});
          }
          startStreaming();
        });
      } catch (e: any) {
        error.value = "无法打开相机: " + (e?.message || String(e));
      }
    }

    function closeCamera() {
      stopStreaming();
      stream.value?.getTracks().forEach((t) => t.stop());
      stream.value = null;
      open.value = false;
      if (videoRef.value) videoRef.value.srcObject = null;
    }

    // fps / 分辨率参数变化后重新按新频率输出
    watch(
      () => [props.data?.params?.fps, props.data?.params?.width, props.data?.params?.height],
      () => {
        if (open.value) startStreaming();
      },
    );

    onBeforeUnmount(() => {
      stopStreaming();
      closeCamera();
    });

    return () =>
      h("div", { class: "fbd-node fbd-camera" }, [
        h(Handle, {
          type: "source",
          position: Position.Right,
          id: "out-0",
          style: { top: "50%" },
        }),
        h("div", { class: "fbd-block-header" }, [
          "相机 ",
          open.value
            ? `${videoRef.value?.videoWidth || "-"}x${videoRef.value?.videoHeight || "-"}`
            : "",
        ]),
        h("div", { class: "fbd-camera-body" }, [
          open.value
            ? h("video", {
                ref: videoRef,
                class: "fbd-camera-video",
                playsInline: "",
                muted: "",
              })
            : h("div", { class: "fbd-camera-placeholder" }, "相机未开启"),
          h("canvas", { ref: canvasRef, class: "fbd-camera-canvas", style: { display: "none" } }),
        ]),
        h("div", { class: "fbd-camera-tools" }, [
          h("button", { class: "fbd-camera-btn", onClick: toggleCamera }, [
            open.value ? "关闭相机" : "打开相机",
          ]),
        ]),
        error.value
          ? h("div", { class: "fbd-camera-error" }, error.value)
          : null,
      ]);
  },
});

// 图像显示节点：接收 IMAGE 信号并显示图像
const ImageViewNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    const imgSrc = ref("");
    const meta = ref("等待图像...");
    watch(
      () => visionSignals[`${props.id}:IMAGE`],
      (v: any) => {
        if (v && looksLikeImage(v)) {
          imgSrc.value = `data:image/png;base64,${v}`;
          meta.value = "接收中";
        } else {
          imgSrc.value = "";
          meta.value = "等待图像...";
        }
      },
      { immediate: true },
    );
    return () =>
      h("div", { class: "fbd-node fbd-imageview" }, [
        h("div", { class: "fbd-block-header" }, "图像显示"),
        h(Handle, {
          type: "target",
          position: Position.Left,
          id: "in-0",
          style: { top: "35%" },
        }),
        imgSrc.value
          ? h("img", { class: "fbd-imageview-img", src: imgSrc.value, alt: "图像" })
          : h("div", { class: "fbd-imageview-placeholder" }, meta.value),
      ]);
  },
});

// OpenCV5 视觉处理节点：灰度化 / 高斯模糊 / 阈值化 / 边缘检测 / 模板匹配 / 轮廓检测
const VisionProcessNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    const status = ref("");
    const busy = ref(false);
    const preview = ref("");

    const cfg = computed(() => VISION_CFG[visionTypeKey(props.data) as string]);

    async function run() {
      const c = cfg.value;
      if (!c || busy.value) return;
      const img = visionSignals[`${props.id}:IMAGE`];
      if (!img || !looksLikeImage(img)) {
        status.value = "等待图像输入";
        preview.value = "";
        return;
      }
      const params = props.data?.params || {};
      if (c.needsTemplate && !looksLikeImage(params.template)) {
        status.value = "请先在属性面板设置模板图片";
        return;
      }
      busy.value = true;
      try {
        const res = await processVisionImage(img, c.ops(params));
        if (res.success && res.data) {
          preview.value = `data:image/png;base64,${res.data.image}`;
          if (c.imageOut) emitVisionSignal(props.id, c.imageOut, res.data.image);
          emitVisionNumericOutputs(props.id, props.data, res.data.results);
          status.value = c.outputText
            ? c.outputText(res.data.results)
            : `${res.data.width}x${res.data.height}`;
        } else {
          status.value = res.error || "处理失败";
          preview.value = "";
        }
      } catch (e: any) {
        status.value = "处理失败: " + (e?.message || String(e));
        preview.value = "";
      } finally {
        busy.value = false;
      }
    }

    watch(
      () => {
        const inputs = (props.data?.inputs || []) as string[];
        return inputs.map((p: string) => visionSignals[`${props.id}:${p}`]);
      },
      () => run(),
      { immediate: true },
    );

    // 参数被属性面板修改后重新处理
    watch(
      () => visionBump[props.id as string] || 0,
      () => run(),
    );

    return () => {
      const typeKey = visionTypeKey(props.data) || "VISION_GRAYSCALE";
      const label = props.data?.label || VISION_FRIENDLY[typeKey] || "视觉";
      const inputs = (props.data?.inputs || []) as string[];
      const outputs = (props.data?.outputs || []) as string[];
      const maxPins = Math.max(inputs.length, outputs.length);
      const bodyH = Math.max(52, maxPins * 22 + 20);
      return h("div", { class: "fbd-node fbd-vision", style: { width: "200px" } }, [
        h("div", { class: "fbd-block-header" }, [
          VISION_FRIENDLY[typeKey] || label,
          busy.value ? " (处理中...)" : "",
        ]),
        h("div", { class: "fbd-block-body", style: { minHeight: bodyH + "px" } }, [
          h(
            "div",
            { class: "fbd-block-pins-left" },
            inputs.map((name: string, i: number) =>
              h("div", { class: "fbd-pin-row", key: "i" + i }, [
                h(Handle, {
                  type: "target",
                  position: Position.Left,
                  id: "in-" + i,
                  style: { top: (i + 1) * 22 + 8 + "px" },
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
                  style: { top: (i + 1) * 22 + 8 + "px" },
                }),
              ]),
            ),
          ),
        ]),
        preview.value
          ? h("img", { class: "fbd-vision-preview", src: preview.value, alt: "" })
          : null,
        status.value
          ? h("div", { class: "fbd-vision-status" }, status.value)
          : null,
      ]);
    };
  },
});

// ROS 2 功能块节点：发布 / 订阅 / 调服务
const Ros2ProcessNode = defineComponent({
  props: ["id", "data", "selected"],
  setup(props) {
    const status = ref("");
    const busy = ref(false);
    let ws: WebSocket | null = null;
    let alive = false;
    let count = 0;

    const typeKey = computed(() => visionTypeKey(props.data) || "ROS2_PUBLISH");

    function params() {
      return props.data?.params || {};
    }

    function setOut(name: string, value: any) {
      const out = (props.data?.outputs || []) as string[];
      if (out.includes(name)) emitVisionSignal(props.id, name, value);
    }

    function setDone(ok: boolean, extra?: Record<string, any>) {
      setOut("ENO", ok ? 1 : 0);
      setOut("DONE", ok ? 1 : 0);
      if (extra) {
        for (const k of Object.keys(extra)) setOut(k, extra[k]);
      }
    }

    async function run() {
      if (busy.value) return;
      const p = params();
      const topic = visionSignals[`${props.id}:TOPIC`] || p.topic;
      if (typeKey.value === "ROS2_PUBLISH") {
        if (!topic) { status.value = "请配置话题"; return; }
        if (!p.type) { status.value = "请配置类型"; return; }
        let msg: Record<string, unknown>;
        try {
          msg = JSON.parse(p.message || "{}");
        } catch {
          status.value = "消息 JSON 解析失败";
          return;
        }
        busy.value = true;
        try {
          const r = await ros2Publish(topic, p.type, msg);
          if (r.success) {
            count++;
            setDone(true, { COUNT: count });
            status.value = `已发布 ${topic} (${count}次)`;
          } else {
            setDone(false);
            status.value = `发布失败: ${r.error}`;
          }
        } catch (e: any) {
          setDone(false);
          status.value = "发布异常: " + (e?.message || String(e));
        } finally {
          busy.value = false;
        }
      } else if (typeKey.value === "ROS2_SERVICE") {
        const svc = p.service;
        if (!svc) { status.value = "请配置服务"; return; }
        if (!p.type) { status.value = "请配置类型"; return; }
        let req: Record<string, unknown>;
        try {
          req = JSON.parse(p.request || "{}");
        } catch {
          status.value = "请求 JSON 解析失败";
          return;
        }
        busy.value = true;
        try {
          const r = await ros2Call(svc, p.type, req);
          if (r.success) {
            setDone(true, { RESP: String(r.data ?? ""), CODE: 0 });
            status.value = "服务调用成功";
          } else {
            setDone(false, { CODE: 1 });
            status.value = `调用失败: ${r.error}`;
          }
        } catch (e: any) {
          setDone(false, { CODE: 1 });
          status.value = "调用异常: " + (e?.message || String(e));
        } finally {
          busy.value = false;
        }
      }
    }

    // 订阅：WS 实时转发
    function startSubscribe() {
      stopSubscribe();
      const topic = params().topic;
      if (!topic) { status.value = "请配置话题"; return; }
      alive = true;
      status.value = `订阅 ${topic} ...`;
      try {
        ws = new WebSocket(ROS2_WS_URL);
      } catch (e: any) {
        status.value = "WS 连接失败: " + (e?.message || String(e));
        return;
      }
      ws.onopen = () => {
        try { ws!.send(JSON.stringify({ type: "subscribe", topic })) } catch {}
      };
      ws.onmessage = (e) => {
        try {
          const { event, data } = JSON.parse(e.data);
          if (event === "message" && data?.topic === topic) {
            count++;
            setOut("DATA", data.payload);
            setOut("AVAIL", 1);
            setOut("COUNT", count);
            setOut("ENO", 1);
            status.value = `已收 ${count} 条`;
          } else if (event === "error") {
            status.value = `WS: ${data}`;
          } else if (event === "subscribed") {
            status.value = `订阅中 ${data}`;
          }
        } catch {}
      };
      ws.onclose = () => {
        if (alive) setTimeout(() => { if (alive) startSubscribe() }, 2000);
      };
    }

    function stopSubscribe() {
      alive = false;
      try {
        if (ws && ws.readyState === WebSocket.OPEN) {
          ws.send(JSON.stringify({ type: "unsubscribe", topic: params().topic }));
        }
      } catch {}
      try { ws?.close() } catch {}
      ws = null;
    }

    // 发布/服务：EN 输入变化时执行
    watch(
      () => {
        const inputs = (props.data?.inputs || []) as string[];
        return inputs.map((p: string) => visionSignals[`${props.id}:${p}`]);
      },
      () => {
        if (typeKey.value === "ROS2_SUBSCRIBE") return;
        if (visionSignals[`${props.id}:EN`]) run();
      },
      { immediate: true },
    );

    // 参数被属性面板修改后重新执行/重新订阅
    watch(
      () => ros2Bump[props.id as string] || 0,
      () => {
        if (typeKey.value === "ROS2_SUBSCRIBE") startSubscribe();
        else run();
      },
    );

    onMounted(() => {
      if (typeKey.value === "ROS2_SUBSCRIBE") startSubscribe();
    });

    onUnmounted(() => {
      stopSubscribe();
    });

    return () => {
      const label = props.data?.label || ROS2_FRIENDLY[typeKey.value] || "ROS2";
      const inputs = (props.data?.inputs || []) as string[];
      const outputs = (props.data?.outputs || []) as string[];
      const maxPins = Math.max(inputs.length, outputs.length);
      const bodyH = Math.max(52, maxPins * 22 + 20);
      return h("div", { class: "fbd-node fbd-ros2", style: { width: "210px" } }, [
        h("div", { class: "fbd-block-header" }, [
          label,
          busy.value ? " (执行中...)" : "",
        ]),
        h("div", { class: "fbd-block-body", style: { minHeight: bodyH + "px" } }, [
          h(
            "div",
            { class: "fbd-block-pins-left" },
            inputs.map((name: string, i: number) =>
              h("div", { class: "fbd-pin-row", key: "i" + i }, [
                h(Handle, {
                  type: "target",
                  position: Position.Left,
                  id: "in-" + i,
                  style: { top: (i + 1) * 22 + 8 + "px" },
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
                  style: { top: (i + 1) * 22 + 8 + "px" },
                }),
              ]),
            ),
          ),
        ]),
        status.value
          ? h("div", { class: "fbd-ros2-status" }, status.value)
          : null,
      ]);
    };
  },
});

const nodeTypes = {
  fbdBlock: markRaw(FunctionBlockNode),
  fbdVarIn: markRaw(VariableInNode),
  fbdVarOut: markRaw(VariableOutNode),
  fbdConst: markRaw(ConstantNode),
  fbdCamera: markRaw(CameraNode),
  fbdImageView: markRaw(ImageViewNode),
  fbdVision: markRaw(VisionProcessNode),
  fbdRos2: markRaw(Ros2ProcessNode),
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
  refreshVisionModels();
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
      if (data.nodes) {
        nodes.value = data.nodes.map((n: any) => normalizeVisionNode(n));
      }
      if (data.edges) edges.value = data.edges;
      return;
    }
  } catch {
    /* 忽略 */
  }
  nodes.value = [];
  edges.value = [];
}

// 兼容旧保存数据：补齐视觉节点的 inputs/outputs/params，并固化机器类型 fbdType
function normalizeVisionNode(n: any): any {
  if (!n || !n.data) return n;
  const merged = { ...n, data: { ...n.data } };
  const typeKey = visionTypeKey(merged.data);
  if (merged.data.fbdType) {
    // 已有 fbdType：若 label 为空则用友好名回填，保证名称框有显示
    if (!merged.data.label) merged.data.label = VISION_FRIENDLY[typeKey] || typeKey;
  }
  if (n.type === "fbdCamera" && !merged.data.outputs) {
    merged.data.outputs = ["IMAGE"];
    merged.data.inputs = merged.data.inputs || [];
  }
  if (n.type === "fbdImageView" && !merged.data.inputs) {
    merged.data.inputs = ["IMAGE"];
    merged.data.outputs = merged.data.outputs || [];
  }
  if (VISION_CFG[typeKey]) {
    merged.data.fbdType = merged.data.fbdType || typeKey;
    const pins = FB_PINS[typeKey] || { inputs: [], outputs: [] };
    merged.data.inputs = merged.data.inputs?.length ? merged.data.inputs : [...pins.inputs];
    merged.data.outputs = merged.data.outputs?.length ? merged.data.outputs : [...pins.outputs];
    merged.data.params = {
      ...defaultVisionParams(typeKey),
      ...(merged.data.params || {}),
    };
  }
  if (isRos2Type(typeKey)) {
    merged.data.fbdType = merged.data.fbdType || typeKey;
    const pins = FB_PINS[typeKey] || { inputs: [], outputs: [] };
    merged.data.inputs = merged.data.inputs?.length ? merged.data.inputs : [...pins.inputs];
    merged.data.outputs = merged.data.outputs?.length ? merged.data.outputs : [...pins.outputs];
    merged.data.params = {
      ...defaultRos2Params(typeKey),
      ...(merged.data.params || {}),
    };
  }
  return merged;
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
    // FBD 内部库：{ fbdType }
    if (item.fbdType) {
      createNode(item.fbdType, position.x, position.y);
      return;
    }
    // 全局库管理器：OpenCV5 视觉项以 VISION_ 开头，ROS 2 项以 ROS2_ 开头
    if (
      typeof item.name === "string" &&
      (item.name.startsWith("VISION_") || item.name.startsWith("ROS2_"))
    ) {
      createNode(item.name, position.x, position.y);
    }
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
    data = { value: "0", label: "常量", inputs: [], outputs: ["VALUE"] };
  } else if (fbdType === "VISION_CAMERA") {
    nodeType = "fbdCamera";
    data = {
      label: "VISION_CAMERA",
      fbdType: "VISION_CAMERA",
      width: 260,
      inputs: [],
      outputs: ["IMAGE"],
      params: defaultVisionParams(fbdType),
    };
  } else if (fbdType === "VISION_IMAGE_VIEW") {
    nodeType = "fbdImageView";
    data = {
      label: "VISION_IMAGE_VIEW",
      fbdType: "VISION_IMAGE_VIEW",
      inputs: ["IMAGE"],
      outputs: [],
      params: defaultVisionParams(fbdType),
    };
  } else if (VISION_CFG[fbdType]) {
    // OpenCV5 视觉处理功能块
    nodeType = "fbdVision";
    const pins = FB_PINS[fbdType] || { inputs: ["EN", "IMAGE"], outputs: ["ENO"] };
    data = {
      label: fbdType,
      fbdType,
      inputs: [...pins.inputs],
      outputs: [...pins.outputs],
      params: defaultVisionParams(fbdType),
    };
  } else if (isRos2Type(fbdType)) {
    // ROS 2 功能块：发布 / 订阅 / 调服务
    nodeType = "fbdRos2";
    const pins = FB_PINS[fbdType] || { inputs: ["EN"], outputs: ["ENO"] };
    data = {
      label: fbdType,
      fbdType,
      inputs: [...pins.inputs],
      outputs: [...pins.outputs],
      params: defaultRos2Params(fbdType),
    };
  } else {
    // 功能块（含 OpenCV5 视觉功能块）
    const pins = FB_PINS[fbdType] || { inputs: ["EN"], outputs: ["ENO"] };
    data = {
      label: fbdType,
      fbdType,
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

// ========== OpenCV5 视觉节点属性 ==========

const fbdVisionLibItems = VISION_LIB;
const fbdRos2LibItems = ROS2_LIB;

const nodeParamList = computed(() => {
  const typeKey = visionTypeKey(selectedNode.value?.data);
  return VISION_PARAMS[typeKey] || ROS2_PARAMS[typeKey] || [];
});

function updateNodeParam(key: string, value: any) {
  if (!selectedNode.value) return;
  const id = selectedNode.value.id;
  const idx = nodes.value.findIndex((n) => n.id === id);
  if (idx < 0) return;
  const node = nodes.value[idx];
  const params = { ...(node.data?.params || {}), [key]: value };
  nodes.value[idx] = { ...node, data: { ...node.data, params } };
  nodes.value = [...nodes.value];
  selectedNode.value = nodes.value[idx];
  saveData();
  if (VISION_CFG[visionTypeKey(selectedNode.value.data)]) {
    visionBump[id] = (visionBump[id] || 0) + 1;
  }
  if (isRos2Type(visionTypeKey(selectedNode.value.data))) {
    ros2Bump[id] = (ros2Bump[id] || 0) + 1;
  }
}

function onTemplateUpload(event: Event) {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = () => {
    const dataUrl = reader.result as string;
    const b64 = dataUrl.split(",")[1] || "";
    updateNodeParam("template", b64);
  };
  reader.readAsDataURL(file);
}

async function refreshVisionModels() {
  const res = await getVisionModels();
  visionModels.value = res?.models || [];
  visionModelsDir.value = res?.dir || "";
}

watch(
  () => visionTypeKey(selectedNode.value?.data),
  (t) => {
    if (t === "VISION_YOLO26") refreshVisionModels();
  },
);
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

/* OpenCV5 相机节点 */
.fbd-camera {
  min-width: 260px;
  width: 260px;
  background: #101828;
  border-color: var(--primary-container);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.fbd-camera-body {
  position: relative;
  background: #060e20;
  min-height: 120px;
  max-height: 240px;
  overflow: hidden;
}

.fbd-camera-video,
.fbd-camera-result-img {
  width: 100%;
  max-height: 240px;
  display: block;
  object-fit: contain;
  background: #000;
}

.fbd-camera-placeholder {
  min-height: 120px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--on-surface-variant);
  font-size: 11px;
}

.fbd-camera-tools {
  display: flex;
  gap: 4px;
  padding: 6px;
  flex-wrap: wrap;
}

.fbd-camera-btn {
  flex: 1;
  padding: 4px 6px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  border: none;
  border-radius: var(--radius-sm);
  font-size: 11px;
  cursor: pointer;
  white-space: nowrap;
}

.fbd-camera-btn:hover {
  filter: brightness(1.1);
}

.fbd-camera-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.fbd-camera-select {
  flex: 1.2;
  padding: 4px 6px;
  background: var(--surface-container-high);
  color: var(--on-surface);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-sm);
  font-size: 11px;
}

.fbd-camera-result {
  border-top: 1px solid var(--outline-variant);
  padding: 4px;
  background: #060e20;
}

.fbd-camera-error {
  padding: 4px 6px;
  font-size: 10px;
  color: var(--error);
  border-top: 1px solid var(--outline-variant);
  word-break: break-all;
}

/* OpenCV5 图像显示节点 */
.fbd-imageview {
  min-width: 180px;
  width: 180px;
  background: #101828;
  border-color: var(--primary-container);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.fbd-imageview-img {
  width: 100%;
  max-height: 180px;
  display: block;
  object-fit: contain;
  background: #000;
}

.fbd-imageview-placeholder {
  min-height: 90px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--on-surface-variant);
  font-size: 11px;
  background: #060e20;
}

/* OpenCV5 视觉处理节点 */
.fbd-vision {
  display: flex;
  flex-direction: column;
  background: #101828;
  border-color: var(--primary-container);
  overflow: hidden;
}

.fbd-vision-preview {
  width: 100%;
  max-height: 120px;
  display: block;
  object-fit: contain;
  background: #000;
  border-top: 1px solid var(--outline-variant);
}

.fbd-vision-status {
  padding: 3px 6px;
  font-size: 10px;
  color: var(--on-surface-variant);
  background: var(--surface-container-high);
  border-top: 1px solid var(--outline-variant);
  word-break: break-all;
}

/* ROS 2 功能块节点 */
.fbd-ros2 {
  display: flex;
  flex-direction: column;
  background: #0f1b2e;
  border-color: var(--tertiary-container);
  overflow: hidden;
}

.fbd-ros2-status {
  padding: 3px 6px;
  font-size: 10px;
  color: var(--on-surface-variant);
  background: var(--surface-container-high);
  border-top: 1px solid var(--outline-variant);
  word-break: break-all;
}

/* 属性面板多行文本 */
.prop-textarea {
  resize: vertical;
  font-family: 'JetBrains Mono', monospace;
}

/* 属性面板提示 */
.prop-hint {
  padding: 4px 2px;
  font-size: 10px;
  color: var(--on-surface-variant);
  line-height: 1.4;
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

.prop-mini-btn {
  margin-top: 4px;
  padding: 2px 10px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 10px;
  cursor: pointer;
}

.prop-mini-btn:hover {
  border-color: var(--primary);
  color: var(--primary);
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
