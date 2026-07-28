<template>
  <div class="cncsim-panel">
    <div class="cncsim-layout">
      <!-- 左侧控制面板 -->
      <div class="control-panel">
        <!-- 机型选择 -->
        <div class="panel-section">
          <div class="section-title">机型选择</div>
          <select
            class="machine-select"
            v-model="currentMachine"
            @change="onMachineChange"
          >
            <optgroup label="铣床">
              <option v-for="m in millMachines" :key="m.id" :value="m.id">
                {{ m.name }}
              </option>
            </optgroup>
            <optgroup label="五轴">
              <option v-for="m in fiveAxisMachines" :key="m.id" :value="m.id">
                {{ m.name }}
              </option>
            </optgroup>
            <optgroup label="机器人">
              <option v-for="m in robotMachines" :key="m.id" :value="m.id">
                {{ m.name }}
              </option>
            </optgroup>
            <optgroup label="其他">
              <option v-for="m in otherMachines" :key="m.id" :value="m.id">
                {{ m.name }}
              </option>
            </optgroup>
            <optgroup label="自定义运动学" v-if="customKinMachines.length > 0">
              <option v-for="m in customKinMachines" :key="m.id" :value="m.id">
                {{ m.name }}
              </option>
            </optgroup>
          </select>
          <div class="machine-desc">{{ currentMachineInfo.desc }}</div>
          <div class="machine-axes">
            轴:
            <span
              v-for="axis in currentMachineInfo.axes"
              :key="axis"
              class="axis-tag"
              >{{ axis }}</span
            >
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">电源控制</div>
          <div class="control-row">
            <button
              class="ctrl-btn"
              :class="{ active: powerOn }"
              @click="togglePower"
            >
              <span class="material-symbols-outlined">power_settings_new</span>
              {{ powerOn ? "上电" : "断电" }}
            </button>
            <button class="ctrl-btn estop" @click="emergencyStop">
              <span class="material-symbols-outlined">emergency</span>
              急停
            </button>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">运行控制</div>
          <div class="control-row">
            <button class="ctrl-btn" :disabled="!powerOn" @click="startCycle">
              <span class="material-symbols-outlined">play_arrow</span>
              循环启动
            </button>
            <button class="ctrl-btn" :disabled="!powerOn" @click="stopCycle">
              <span class="material-symbols-outlined">stop</span>
              停止
            </button>
          </div>
          <div class="control-row" style="margin-top: 4px">
            <button class="ctrl-btn" :disabled="!powerOn" @click="resetOrigin">
              <span class="material-symbols-outlined">home</span>
              回零
            </button>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">手动 jog</div>
          <div class="jog-grid">
            <div
              v-for="axis in currentMachineInfo.axes"
              :key="axis"
              class="jog-row"
            >
              <span class="axis-label">{{ axis }}</span>
              <button
                class="jog-btn"
                :disabled="!powerOn"
                @mousedown="jogStart(axis, -1)"
                @mouseup="jogStop"
                @mouseleave="jogStop"
              >
                <span class="material-symbols-outlined">remove</span>
              </button>
              <span class="jog-value">{{
                axisPositions[axis]?.toFixed(3) || "0.000"
              }}</span>
              <button
                class="jog-btn"
                :disabled="!powerOn"
                @mousedown="jogStart(axis, 1)"
                @mouseup="jogStop"
                @mouseleave="jogStop"
              >
                <span class="material-symbols-outlined">add</span>
              </button>
            </div>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">进给倍率 {{ feedOverride }}%</div>
          <input
            type="range"
            class="feed-slider"
            min="0"
            max="200"
            v-model.number="feedOverride"
          />
        </div>

        <div class="panel-section">
          <div class="section-title">主轴转速 {{ spindleRPM }} RPM</div>
          <input
            type="range"
            class="feed-slider"
            min="0"
            max="8000"
            step="100"
            v-model.number="spindleRPM"
          />
        </div>

        <div class="panel-section">
          <div class="section-title">加载 G 代码</div>
          <textarea
            class="gcode-input"
            v-model="gcodeContent"
            placeholder="在此输入 G 代码...&#10;G90 G21 G17&#10;G0 X0 Y0 Z10&#10;G1 Z-5 F200&#10;M3 S3000&#10;G1 X50 F300&#10;M5&#10;G0 Z10"
            rows="10"
          />
          <div class="control-row" style="margin-top: 4px">
            <button class="ctrl-btn load-btn" @click="loadAndRun">
              <span class="material-symbols-outlined">play_circle</span>
              加载运行
            </button>
            <button class="ctrl-btn" @click="loadSampleGcode">
              <span class="material-symbols-outlined">description</span>
              示例
            </button>
          </div>
        </div>
      </div>

      <!-- 中间3D视图 -->
      <div class="viewport-container">
        <div class="viewport-header">
          <span>{{ currentMachineInfo.name }} — 3D 视图</span>
          <div class="viewport-controls">
            <button class="view-btn" @click="resetView" title="重置视图">
              <span class="material-symbols-outlined">center_focus_strong</span>
            </button>
            <button class="view-btn" @click="toggleWireframe" title="线框模式">
              <span class="material-symbols-outlined">grid_on</span>
            </button>
            <button class="view-btn" @click="toggleAnimation" title="暂停/继续">
              <span class="material-symbols-outlined">{{
                animating ? "pause" : "play_arrow"
              }}</span>
            </button>
          </div>
        </div>
        <div class="viewport-3d" ref="viewportContainer">
          <canvas ref="threeCanvas" />
        </div>
        <div class="path-info" v-if="pathPoints.length > 0">
          <span>路径点: {{ currentPathIdx }} / {{ pathPoints.length }}</span>
          <span v-if="spindleRunning" class="spindle-indicator">
            ⚙ {{ spindleRPM }} RPM
          </span>
          <button class="clear-btn" @click="clearPath">清除路径</button>
        </div>
      </div>

      <!-- 右侧状态面板 -->
      <div class="status-panel">
        <div class="panel-section">
          <div class="section-title">机床坐标 (MCS)</div>
          <div
            class="dro-row"
            v-for="axis in currentMachineInfo.axes"
            :key="axis"
          >
            <span class="dro-axis">{{ axis }}</span>
            <span class="dro-value mcs">{{
              (axisPositions[axis] || 0).toFixed(3)
            }}</span>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">工件坐标 (WCS)</div>
          <div
            class="dro-row"
            v-for="axis in currentMachineInfo.axes"
            :key="axis"
          >
            <span class="dro-axis">{{ axis }}</span>
            <span class="dro-value wcs">{{
              ((axisPositions[axis] || 0) - (workOffset[axis] || 0)).toFixed(3)
            }}</span>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">状态</div>
          <div class="status-row">
            <span class="status-label">模式</span>
            <span class="status-val">{{ cycleRunning ? "自动" : "手动" }}</span>
          </div>
          <div class="status-row">
            <span class="status-label">主轴</span>
            <span class="status-val" :class="{ 'spindle-on': spindleRunning }">
              {{ spindleRunning ? spindleRPM + " RPM" : "停止" }}
            </span>
          </div>
          <div class="status-row">
            <span class="status-label">进给</span>
            <span class="status-val">{{ currentFeed.toFixed(0) }} mm/min</span>
          </div>
          <div class="status-row">
            <span class="status-label">刀具</span>
            <span class="status-val">T{{ currentTool }}</span>
          </div>
          <div class="status-row">
            <span class="status-label">冷却</span>
            <span class="status-val">{{ coolantOn ? "开" : "关" }}</span>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">模态代码</div>
          <div class="gcode-modes">
            <span class="gmode" v-for="mode in activeModes" :key="mode">{{
              mode
            }}</span>
          </div>
        </div>

        <div class="panel-section">
          <div class="section-title">消息</div>
          <div class="message-area" ref="messageArea">
            <div
              v-for="(msg, i) in messages"
              :key="i"
              class="msg-item"
              :class="'msg-' + msg.level"
            >
              {{ msg.text }}
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, reactive, nextTick } from "vue";
import * as THREE from "three";
import { useKinematicsStore } from "@/stores/kinematics";

const emit = defineEmits<{
  connect: [];
  disconnect: [];
}>();

// ========== 机型定义 ==========
interface MachineDef {
  id: string;
  name: string;
  desc: string;
  axes: string[];
  kinematics: string;
  viewDistance: number;
  viewRotX: number;
  viewRotY: number;
  gridSize: number;
}

const machines: MachineDef[] = [
  {
    id: "vmc",
    name: "VMC 三轴铣床",
    desc: "立式加工中心 XYZ 3轴",
    axes: ["X", "Y", "Z"],
    kinematics: "identity",
    viewDistance: 250,
    viewRotX: -60,
    viewRotY: 45,
    gridSize: 200,
  },
  {
    id: "hmc",
    name: "HMC 卧式铣床",
    desc: "卧式加工中心 XYZ+B 4轴",
    axes: ["X", "Y", "Z", "B"],
    kinematics: "identity",
    viewDistance: 300,
    viewRotX: -40,
    viewRotY: 30,
    gridSize: 250,
  },
  {
    id: "lathe",
    name: "数控车床",
    desc: "两轴车床 X/Z + 主轴C",
    axes: ["X", "Z", "C"],
    kinematics: "identity",
    viewDistance: 250,
    viewRotX: -50,
    viewRotY: 45,
    gridSize: 200,
  },
  {
    id: "5axis_ac",
    name: "五轴铣床 (XYZAC)",
    desc: "A+C 转台型五轴 Hermle风格",
    axes: ["X", "Y", "Z", "A", "C"],
    kinematics: "identity",
    viewDistance: 500,
    viewRotX: -55,
    viewRotY: 25,
    gridSize: 400,
  },
  {
    id: "5axis_bc",
    name: "五轴铣床 (XYZBC)",
    desc: "B+C 转台型五轴",
    axes: ["X", "Y", "Z", "B", "C"],
    kinematics: "identity",
    viewDistance: 500,
    viewRotX: -55,
    viewRotY: 25,
    gridSize: 400,
  },
  {
    id: "5axis_bcw",
    name: "五轴龙门铣 (XYZBCW)",
    desc: "龙门铣 摆头+转台",
    axes: ["X", "Y", "Z", "B", "C"],
    kinematics: "5axis",
    viewDistance: 1200,
    viewRotX: -50,
    viewRotY: 35,
    gridSize: 1000,
  },
  {
    id: "5axis_ab",
    name: "五轴铣床 (XYZAB)",
    desc: "双旋转工作台五轴",
    axes: ["X", "Y", "Z", "A", "B"],
    kinematics: "identity",
    viewDistance: 500,
    viewRotX: -55,
    viewRotY: 0,
    gridSize: 400,
  },
  {
    id: "5axis_maxnc",
    name: "五轴铣床 (MaxNC)",
    desc: "MaxNC 5轴 摆头+转台",
    axes: ["X", "Y", "Z", "B", "C"],
    kinematics: "maxkins",
    viewDistance: 500,
    viewRotX: -45,
    viewRotY: 30,
    gridSize: 400,
  },
  {
    id: "scara",
    name: "SCARA 机器人",
    desc: "水平关节机器人 4轴",
    axes: ["J1", "J2", "J3", "J4"],
    kinematics: "scara",
    viewDistance: 500,
    viewRotX: -45,
    viewRotY: -60,
    gridSize: 400,
  },
  {
    id: "delta",
    name: "Delta 并联机器人",
    desc: "并联机器人 3轴",
    axes: ["J1", "J2", "J3"],
    kinematics: "delta",
    viewDistance: 500,
    viewRotX: -40,
    viewRotY: 25,
    gridSize: 400,
  },
  {
    id: "puma",
    name: "PUMA 六轴机器人",
    desc: "PUMA 560 六轴工业机器人",
    axes: ["J1", "J2", "J3", "J4", "J5", "J6"],
    kinematics: "puma",
    viewDistance: 150,
    viewRotX: -40,
    viewRotY: 50,
    gridSize: 150,
  },
  {
    id: "fanuc",
    name: "Fanuc 六轴机器人",
    desc: "Fanuc 200F 六轴工业机器人",
    axes: ["J1", "J2", "J3", "J4", "J5", "J6"],
    kinematics: "puma",
    viewDistance: 300,
    viewRotX: -40,
    viewRotY: 50,
    gridSize: 250,
  },
  {
    id: "router",
    name: "Router 开料机",
    desc: "ATC 龙门开料机 XYZ 3轴",
    axes: ["X", "Y", "Z"],
    kinematics: "identity",
    viewDistance: 1500,
    viewRotX: -30,
    viewRotY: 45,
    gridSize: 1200,
  },
];

const millMachines = machines.filter((m) =>
  ["vmc", "hmc", "lathe"].includes(m.id),
);
const fiveAxisMachines = machines.filter((m) => m.id.startsWith("5axis"));
const robotMachines = machines.filter((m) =>
  ["scara", "delta", "puma", "fanuc"].includes(m.id),
);
const otherMachines = machines.filter((m) => m.id === "router");

// 自定义运动学机型（来自运动学编辑器）
const kinStore = useKinematicsStore();
const customKinMachines = computed<MachineDef[]>(() =>
  kinStore.customKinConfigs.map((c) => ({
    id: `kin_${c.id}`,
    name: c.name,
    desc: `${c.type} — ${c.joints.length}轴 — 来自运动学编辑器`,
    axes: c.joints.map((_j, i) => `J${i + 1}`),
    kinematics: "custom_kin",
    viewDistance: 500,
    viewRotX: -45,
    viewRotY: 30,
    gridSize: 400,
  })),
);

// 所有机型（含自定义）
const allMachines = computed(() => [...machines, ...customKinMachines.value]);

const currentMachine = ref("vmc");
const currentMachineInfo = computed(() =>
  allMachines.value.find((m) => m.id === currentMachine.value)!,
);

// ========== 状态 ==========
const powerOn = ref(false);
const cycleRunning = ref(false);
const spindleRunning = ref(false);
const spindleRPM = ref(0);
const currentFeed = ref(0);
const feedOverride = ref(100);
const gcodeContent = ref("");
const activeModes = ref(["G21", "G90", "G17"]);
const currentTool = ref(1);
const coolantOn = ref(false);
const animating = ref(true);

const axisPositions = reactive<Record<string, number>>({});
const workOffset = reactive<Record<string, number>>({});
const pathPoints = ref<{ x: number; y: number; z: number }[]>([]);
const currentPathIdx = ref(0);
const messages = ref<Array<{ level: string; text: string }>>([
  { level: "info", text: "仿真器已就绪" },
]);
const messageArea = ref<HTMLDivElement | null>(null);

// Three.js
const viewportContainer = ref<HTMLDivElement | null>(null);
const threeCanvas = ref<HTMLCanvasElement | null>(null);
let scene: THREE.Scene | null = null;
let camera: THREE.PerspectiveCamera | null = null;
let renderer: THREE.WebGLRenderer | null = null;
let animationId: number | null = null;

// 机床部件
let machineGroup: THREE.Group | null = null;
let spindleHead: THREE.Mesh | null = null;
let toolMesh: THREE.Mesh | null = null;
let toolPathLine: THREE.Line | null = null;
let executedPathLine: THREE.Line | null = null;
let pathAnimTimer: ReturnType<typeof setInterval> | null = null;

// 鼠标控制
let isMouseDown = false;
let lastMouseX = 0;
let lastMouseY = 0;
let cameraAngleX = 0;
let cameraAngleY = 0;
let cameraDistance = 250;

// ========== 初始化 ==========
onMounted(() => {
  initPositions();
  initThreeJS();
  addMessage("info", `${currentMachineInfo.value.name} 3D 视图已初始化`);
  emit("connect");
});

onUnmounted(() => {
  if (animationId !== null) cancelAnimationFrame(animationId);
  if (pathAnimTimer) clearInterval(pathAnimTimer);
  renderer?.dispose();
});

function initPositions() {
  const axes = currentMachineInfo.value.axes;
  for (const a of axes) {
    axisPositions[a] = 0;
    workOffset[a] = 0;
  }
}

function onMachineChange() {
  initPositions();
  clearPath();
  rebuildMachine();
  addMessage("info", `切换机型: ${currentMachineInfo.value.name}`);
}

// ========== Three.js ==========
function initThreeJS() {
  if (!threeCanvas.value || !viewportContainer.value) return;
  const container = viewportContainer.value;
  const width = container.clientWidth;
  const height = container.clientHeight;

  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x0f172a);
  scene.fog = new THREE.Fog(0x0f172a, 500, 2000);

  camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 5000);

  renderer = new THREE.WebGLRenderer({
    canvas: threeCanvas.value,
    antialias: true,
  });
  renderer.setSize(width, height);
  renderer.setPixelRatio(window.devicePixelRatio);

  // 光照
  scene.add(new THREE.AmbientLight(0x404060, 0.6));
  const dirLight = new THREE.DirectionalLight(0xffffff, 1.0);
  dirLight.position.set(100, 200, 150);
  scene.add(dirLight);
  const pointLight = new THREE.PointLight(0x4d8eff, 0.5, 500);
  pointLight.position.set(-100, 200, -100);
  scene.add(pointLight);

  rebuildMachine();

  // 鼠标旋转/缩放
  const canvas = threeCanvas.value;
  canvas.addEventListener("mousedown", onMouseDown);
  canvas.addEventListener("mousemove", onMouseMove);
  canvas.addEventListener("mouseup", onMouseUp);
  canvas.addEventListener("mouseleave", onMouseUp);
  canvas.addEventListener("wheel", onWheel);

  updateCamera();
  animate();
}

function rebuildMachine() {
  if (scene) {
    if (machineGroup) scene.remove(machineGroup);
    if (toolPathLine) scene.remove(toolPathLine);
    if (executedPathLine) scene.remove(executedPathLine);
    toolPathLine = null;
    executedPathLine = null;
  }
  machineGroup = new THREE.Group();
  buildCurrentMachine();
  if (scene) {
    scene.add(machineGroup);
    // 网格
    const grid = currentMachineInfo.value.gridSize;
    const gridHelper = new THREE.GridHelper(
      grid * 2,
      Math.floor(grid / 5),
      0x1a2340,
      0x1a2340,
    );
    scene.add(gridHelper);
  }
  const mi = currentMachineInfo.value;
  cameraDistance = mi.viewDistance;
  cameraAngleX = THREE.MathUtils.degToRad(mi.viewRotX);
  cameraAngleY = THREE.MathUtils.degToRad(mi.viewRotY);
  updateCamera();
}

// ========== 按机型构建3D模型 ==========
function buildCurrentMachine() {
  if (!machineGroup) return;
  const id = currentMachine.value;
  switch (id) {
    case "vmc":
      buildVMC();
      break;
    case "hmc":
      buildHMC();
      break;
    case "lathe":
      buildLathe();
      break;
    case "5axis_ac":
    case "5axis_bc":
    case "5axis_ab":
    case "5axis_maxnc":
      build5Axis();
      break;
    case "5axis_bcw":
      buildGantry();
      break;
    case "scara":
      buildSCARA();
      break;
    case "delta":
      buildDelta();
      break;
    case "puma":
    case "fanuc":
      buildRobot6();
      break;
    case "router":
      buildRouter();
      break;
    default:
      if (id.startsWith("kin_")) {
        buildGenericRobot();
      } else {
        buildVMC();
      }
  }
}

function mat(color: number, wireframe = false) {
  return new THREE.MeshPhongMaterial({ color, wireframe });
}

function box(w: number, h: number, d: number, color: number): THREE.Mesh {
  return new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat(color));
}

function cyl(rTop: number, rBot: number, h: number, color: number): THREE.Mesh {
  return new THREE.Mesh(
    new THREE.CylinderGeometry(rTop, rBot, h, 20),
    mat(color),
  );
}

function buildVMC() {
  // 床身
  const bed = box(120, 10, 80, 0x2d3449);
  bed.position.set(0, -5, 0);
  machineGroup!.add(bed);
  // 工作台
  const table = box(80, 6, 60, 0x424754);
  table.position.set(0, 3, 0);
  machineGroup!.add(table);
  // 立柱
  const column = box(16, 100, 16, 0x3d4459);
  column.position.set(-50, 50, 0);
  machineGroup!.add(column);
  // 主轴箱
  const headBox = box(24, 30, 24, 0x4d8eff);
  headBox.position.set(-50, 70, 0);
  machineGroup!.add(headBox);
  // 主轴
  spindleHead = cyl(4, 4, 35, 0xadc6ff);
  spindleHead.position.set(-50, 45, 0);
  machineGroup!.add(spindleHead);
  // 刀具
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 18, 8), mat(0xfbabff));
  toolMesh.position.set(-50, 22, 0);
  machineGroup!.add(toolMesh);
}

function buildHMC() {
  // 床身（更长）
  const bed = box(160, 12, 100, 0x2d3449);
  bed.position.set(0, -6, 0);
  machineGroup!.add(bed);
  // 工作台（带B轴旋转台）
  const table = box(90, 8, 80, 0x424754);
  table.position.set(20, 8, 0);
  machineGroup!.add(table);
  // B轴转台
  const bTable = cyl(30, 30, 10, 0x4d8eff);
  bTable.position.set(20, 17, 0);
  machineGroup!.add(bTable);
  // 立柱（右侧）
  const column = box(16, 110, 16, 0x3d4459);
  column.position.set(60, 55, 0);
  machineGroup!.add(column);
  // 主轴箱（卧式）
  const headBox = box(40, 20, 20, 0x4d8eff);
  headBox.position.set(40, 60, 0);
  machineGroup!.add(headBox);
  // 主轴（水平方向）
  spindleHead = cyl(4, 4, 40, 0xadc6ff);
  spindleHead.rotation.z = Math.PI / 2;
  spindleHead.position.set(10, 60, 0);
  machineGroup!.add(spindleHead);
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 15, 8), mat(0xfbabff));
  toolMesh.rotation.z = Math.PI / 2;
  toolMesh.position.set(-15, 60, 0);
  machineGroup!.add(toolMesh);
}

function buildLathe() {
  // 床身
  const bed = box(140, 10, 70, 0x2d3449);
  bed.position.set(0, -5, 0);
  machineGroup!.add(bed);
  // 主轴箱（左侧）
  const headstock = box(30, 40, 50, 0x3d4459);
  headstock.position.set(-50, 20, 0);
  machineGroup!.add(headstock);
  // 卡盘
  spindleHead = cyl(18, 18, 8, 0x4d8eff);
  spindleHead.rotation.z = Math.PI / 2;
  spindleHead.position.set(-30, 20, 0);
  machineGroup!.add(spindleHead);
  // 尾座（右侧）
  const tailstock = box(20, 30, 30, 0x3d4459);
  tailstock.position.set(50, 15, 0);
  machineGroup!.add(tailstock);
  // 刀架
  const toolPost = box(15, 15, 15, 0x424754);
  toolPost.position.set(0, 30, 25);
  machineGroup!.add(toolPost);
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 12, 8), mat(0xfbabff));
  toolMesh.rotation.z = Math.PI / 2;
  toolMesh.position.set(0, 30, 15);
  machineGroup!.add(toolMesh);
}

function build5Axis() {
  // 床身
  const bed = box(150, 12, 120, 0x2d3449);
  bed.position.set(0, -6, 0);
  machineGroup!.add(bed);
  // 立柱
  const column = box(20, 120, 20, 0x3d4459);
  column.position.set(-60, 60, 0);
  machineGroup!.add(column);
  // 主轴箱
  const headBox = box(28, 35, 28, 0x4d8eff);
  headBox.position.set(-60, 75, 0);
  machineGroup!.add(headBox);
  // 主轴
  spindleHead = cyl(5, 5, 40, 0xadc6ff);
  spindleHead.position.set(-60, 45, 0);
  machineGroup!.add(spindleHead);
  // 刀具
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 16, 8), mat(0xfbabff));
  toolMesh.position.set(-60, 20, 0);
  machineGroup!.add(toolMesh);
  // 鞍座
  const saddle = box(80, 8, 80, 0x424754);
  saddle.position.set(0, 6, 0);
  machineGroup!.add(saddle);
  // 转台 (A/C)
  const rotaryTable = cyl(35, 35, 12, 0x4d8eff);
  rotaryTable.position.set(0, 16, 0);
  machineGroup!.add(rotaryTable);
  // 工件
  const workpiece = box(30, 25, 30, 0x5a7a5a);
  workpiece.position.set(0, 35, 0);
  machineGroup!.add(workpiece);
}

function buildGantry() {
  // 底座
  const base = box(200, 15, 160, 0x2d3449);
  base.position.set(0, -7, 0);
  machineGroup!.add(base);
  // 工作台
  const table = box(140, 8, 120, 0x424754);
  table.position.set(0, 11, 0);
  machineGroup!.add(table);
  // 左立柱
  const colL = box(16, 130, 16, 0x3d4459);
  colL.position.set(-80, 65, 0);
  machineGroup!.add(colL);
  // 右立柱
  const colR = box(16, 130, 16, 0x3d4459);
  colR.position.set(80, 65, 0);
  machineGroup!.add(colR);
  // 横梁
  const beam = box(180, 14, 16, 0x3d4459);
  beam.position.set(0, 110, 0);
  machineGroup!.add(beam);
  // 滑座
  const slider = box(30, 10, 24, 0x4d8eff);
  slider.position.set(0, 100, 0);
  machineGroup!.add(slider);
  // 滑枕 Z
  const ram = box(16, 50, 16, 0x4d8eff);
  ram.position.set(0, 70, 0);
  machineGroup!.add(ram);
  // 主轴
  spindleHead = cyl(5, 5, 30, 0xadc6ff);
  spindleHead.position.set(0, 40, 0);
  machineGroup!.add(spindleHead);
  // 刀具
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 16, 8), mat(0xfbabff));
  toolMesh.position.set(0, 18, 0);
  machineGroup!.add(toolMesh);
}

function buildSCARA() {
  // 底座
  const base = cyl(25, 30, 15, 0x2d3449);
  base.position.set(0, -7, 0);
  machineGroup!.add(base);
  // J1 大臂
  const arm1 = box(100, 12, 20, 0x3d4459);
  arm1.position.set(50, 10, 0);
  machineGroup!.add(arm1);
  // J1 关节
  const j1 = cyl(12, 12, 16, 0x4d8eff);
  j1.position.set(0, 10, 0);
  machineGroup!.add(j1);
  // J2 小臂
  const arm2 = box(80, 10, 16, 0x424754);
  arm2.position.set(90, 10, 0);
  machineGroup!.add(arm2);
  // J2 关节
  const j2 = cyl(10, 10, 14, 0x4d8eff);
  j2.position.set(100, 10, 0);
  machineGroup!.add(j2);
  // 末端执行器
  toolMesh = cyl(4, 4, 20, 0xfbabff);
  toolMesh.position.set(130, 5, 0);
  machineGroup!.add(toolMesh);
}

function buildDelta() {
  // 顶部平台
  const topPlate = cyl(40, 40, 8, 0x2d3449);
  topPlate.position.set(0, 80, 0);
  machineGroup!.add(topPlate);
  // 底部动平台
  const bottomPlate = cyl(20, 20, 6, 0x4d8eff);
  bottomPlate.position.set(0, 20, 0);
  machineGroup!.add(bottomPlate);
  // 三根导轨柱
  for (let i = 0; i < 3; i++) {
    const angle = (i * 120 * Math.PI) / 180;
    const rx = Math.cos(angle) * 35;
    const rz = Math.sin(angle) * 35;
    const rail = cyl(3, 3, 80, 0x3d4459);
    rail.position.set(rx, 40, rz);
    machineGroup!.add(rail);
    // 连杆
    const rod = box(4, 50, 4, 0x424754);
    rod.position.set(rx * 0.5, 45, rz * 0.5);
    machineGroup!.add(rod);
  }
  // 刀具
  toolMesh = cyl(2, 2, 15, 0xfbabff);
  toolMesh.position.set(0, 10, 0);
  machineGroup!.add(toolMesh);
}

function buildRobot6() {
  // 底座
  const base = cyl(20, 25, 12, 0x2d3449);
  base.position.set(0, -6, 0);
  machineGroup!.add(base);
  // J1 旋转台
  const j1 = cyl(16, 16, 15, 0x3d4459);
  j1.position.set(0, 12, 0);
  machineGroup!.add(j1);
  // 肩部
  const shoulder = box(14, 30, 14, 0x4d8eff);
  shoulder.position.set(0, 35, 0);
  machineGroup!.add(shoulder);
  // 大臂
  const upperArm = box(10, 50, 10, 0x3d4459);
  upperArm.position.set(0, 70, 0);
  machineGroup!.add(upperArm);
  // 肘关节
  const elbow = cyl(8, 8, 12, 0x4d8eff);
  elbow.position.set(0, 95, 0);
  machineGroup!.add(elbow);
  // 小臂
  const forearm = box(8, 40, 8, 0x424754);
  forearm.position.set(0, 120, 0);
  machineGroup!.add(forearm);
  // 手腕
  const wrist = cyl(6, 6, 10, 0x4d8eff);
  wrist.position.set(0, 142, 0);
  machineGroup!.add(wrist);
  // 末端
  toolMesh = cyl(2, 2, 14, 0xfbabff);
  toolMesh.position.set(0, 154, 0);
  machineGroup!.add(toolMesh);
}

function buildRouter() {
  // 床身（很大）
  const bed = box(300, 10, 200, 0x2d3449);
  bed.position.set(0, -5, 0);
  machineGroup!.add(bed);
  // 工作台面
  const tableSurface = box(280, 4, 180, 0x424754);
  tableSurface.position.set(0, 2, 0);
  machineGroup!.add(tableSurface);
  // 左立柱
  const colL = box(12, 100, 12, 0x3d4459);
  colL.position.set(-130, 50, 0);
  machineGroup!.add(colL);
  // 右立柱
  const colR = box(12, 100, 12, 0x3d4459);
  colR.position.set(130, 50, 0);
  machineGroup!.add(colR);
  // 横梁
  const beam = box(280, 10, 12, 0x3d4459);
  beam.position.set(0, 90, 0);
  machineGroup!.add(beam);
  // 主轴头
  spindleHead = box(20, 30, 16, 0x4d8eff);
  spindleHead.position.set(0, 75, 0);
  machineGroup!.add(spindleHead);
  // 主轴
  const spindle = cyl(4, 4, 25, 0xadc6ff);
  spindle.position.set(0, 55, 0);
  machineGroup!.add(spindle);
  // 刀具
  toolMesh = new THREE.Mesh(new THREE.ConeGeometry(2, 14, 8), mat(0xfbabff));
  toolMesh.position.set(0, 38, 0);
  machineGroup!.add(toolMesh);
}

// 通用运动学机器人构建器（来自运动学编辑器的自定义配置）
function buildGenericRobot() {
  const kinId = currentMachine.value.replace("kin_", "");
  const kinConfig = kinStore.getKinConfig(kinId);
  const jointCount = kinConfig?.joints.length || 6;

  // 底座
  const base = cyl(25, 30, 15, 0x2d3449);
  base.position.set(0, -7, 0);
  machineGroup!.add(base);

  // 根据关节数量生成连杆
  const linkHeight = Math.max(20, 120 / jointCount);
  let yPos = 0;
  const linkWidth = 10;
  const linkDepth = 10;

  for (let i = 0; i < jointCount; i++) {
    // 关节球
    const jointRadius = Math.max(5, 10 - i * 0.5);
    const joint = cyl(jointRadius, jointRadius, linkDepth, 0x4d8eff);
    joint.position.set(0, yPos, 0);
    machineGroup!.add(joint);

    // 连杆
    const armLen = linkHeight * (1 - i * 0.15);
    const arm = box(
      linkWidth,
      armLen,
      linkDepth,
      i % 2 === 0 ? 0x3d4459 : 0x424754,
    );
    arm.position.set(0, yPos + armLen / 2 + jointRadius / 2, 0);
    machineGroup!.add(arm);

    yPos += armLen + jointRadius;

    // 最后一个关节上安装末端执行器
    if (i === jointCount - 1) {
      const endJoint = cyl(5, 5, 10, 0x4d8eff);
      endJoint.position.set(0, yPos, 0);
      machineGroup!.add(endJoint);

      toolMesh = cyl(2, 2, 18, 0xfbabff);
      toolMesh.position.set(0, yPos + 14, 0);
      machineGroup!.add(toolMesh);
    }
  }
}

// ========== 摄像机控制 ==========
function updateCamera() {
  if (!camera) return;
  const x = cameraDistance * Math.cos(cameraAngleX) * Math.cos(cameraAngleY);
  const y = cameraDistance * Math.sin(cameraAngleX);
  const z = cameraDistance * Math.cos(cameraAngleX) * Math.sin(cameraAngleY);
  camera.position.set(x, Math.abs(y) + 50, z);
  camera.lookAt(0, 20, 0);
}

function onMouseDown(e: MouseEvent) {
  isMouseDown = true;
  lastMouseX = e.clientX;
  lastMouseY = e.clientY;
}
function onMouseMove(e: MouseEvent) {
  if (!isMouseDown) return;
  const dx = e.clientX - lastMouseX;
  const dy = e.clientY - lastMouseY;
  cameraAngleY += dx * 0.005;
  cameraAngleX = Math.max(-1.4, Math.min(0.1, cameraAngleX + dy * 0.005));
  lastMouseX = e.clientX;
  lastMouseY = e.clientY;
  updateCamera();
}
function onMouseUp() {
  isMouseDown = false;
}
function onWheel(e: WheelEvent) {
  cameraDistance = Math.max(
    50,
    Math.min(3000, cameraDistance + e.deltaY * 0.5),
  );
  updateCamera();
}

function animate() {
  animationId = requestAnimationFrame(animate);
  if (renderer && scene && camera && animating.value) {
    renderer.render(scene, camera);
  }
}

function resetView() {
  const mi = currentMachineInfo.value;
  cameraDistance = mi.viewDistance;
  cameraAngleX = THREE.MathUtils.degToRad(mi.viewRotX);
  cameraAngleY = THREE.MathUtils.degToRad(mi.viewRotY);
  updateCamera();
}

function toggleWireframe() {
  if (!machineGroup) return;
  machineGroup.traverse((child) => {
    if (child instanceof THREE.Mesh) {
      (child.material as THREE.MeshPhongMaterial).wireframe = !(
        child.material as THREE.MeshPhongMaterial
      ).wireframe;
    }
  });
}

function toggleAnimation() {
  animating.value = !animating.value;
}

// ========== 操作 ==========
function togglePower() {
  powerOn.value = !powerOn.value;
  addMessage("info", powerOn.value ? "电源已接通" : "电源已断开");
}

function emergencyStop() {
  powerOn.value = false;
  cycleRunning.value = false;
  spindleRunning.value = false;
  spindleRPM.value = 0;
  currentFeed.value = 0;
  coolantOn.value = false;
  if (pathAnimTimer) {
    clearInterval(pathAnimTimer);
    pathAnimTimer = null;
  }
  addMessage("error", "紧急停止!");
}

function startCycle() {
  cycleRunning.value = true;
  spindleRunning.value = true;
  addMessage("info", "循环启动");
}

function stopCycle() {
  cycleRunning.value = false;
  currentFeed.value = 0;
  if (pathAnimTimer) {
    clearInterval(pathAnimTimer);
    pathAnimTimer = null;
  }
  addMessage("info", "循环停止");
}

function resetOrigin() {
  initPositions();
  updateMachinePosition();
  addMessage("info", "各轴已回零");
}

function jogStart(axis: string, dir: number) {
  if (!powerOn.value) return;
  const step =
    currentMachine.value.includes("5axis") || currentMachine.value === "router"
      ? 1
      : 0.5;
  axisPositions[axis] =
    Math.round((axisPositions[axis] + dir * step) * 1000) / 1000;
  updateMachinePosition();
}

function jogStop() {}

// ========== G 代码解释器 ==========
function parseAndRun(gcode: string) {
  const lines = gcode.split("\n");
  const points: {
    x: number;
    y: number;
    z: number;
    feed: number;
    spindle: boolean;
  }[] = [];

  // 模态状态
  let modalG = "G01"; // G00/G01/G02/G03
  let feedRate = 100;
  let absolute = true; // G90/G91
  let currentX = axisPositions.X || 0;
  let currentY = axisPositions.Y || 0;
  let currentZ = axisPositions.Z || 0;

  for (const rawLine of lines) {
    const line = rawLine.split(";")[0].split("//")[0].trim();
    if (!line) continue;

    const upper = line.toUpperCase();
    const cmd: Record<string, number> = {};

    // 提取字母+数值
    const matches = upper.matchAll(/([A-Z])([-+]?\d*\.?\d+)/g);
    for (const m of matches) {
      cmd[m[1]] = parseFloat(m[2]);
    }

    // 处理 G 代码
    const gCodes = upper.match(/G(\d+)/g);
    if (gCodes) {
      for (const gc of gCodes) {
        const gn = parseInt(gc.substring(1));
        switch (gn) {
          case 0:
            modalG = "G00";
            break;
          case 1:
            modalG = "G01";
            break;
          case 2:
            modalG = "G02";
            break;
          case 3:
            modalG = "G03";
            break;
          case 17:
            activeModes.value.push("G17");
            break;
          case 18:
            activeModes.value.push("G18");
            break;
          case 19:
            activeModes.value.push("G19");
            break;
          case 20:
            addMessage("info", "英制模式");
            break;
          case 21:
            addMessage("info", "公制模式 (mm)");
            break;
          case 28:
            currentX = 0;
            currentY = 0;
            currentZ = 0;
            break;
          case 90:
            absolute = true;
            break;
          case 91:
            absolute = false;
            break;
          case 92:
            if (cmd.X !== undefined) workOffset.X = currentX - cmd.X;
            if (cmd.Y !== undefined) workOffset.Y = currentY - cmd.Y;
            if (cmd.Z !== undefined) workOffset.Z = currentZ - cmd.Z;
            break;
        }
      }
    }

    // 处理 M 代码
    const mCodes = upper.match(/M(\d+)/g);
    if (mCodes) {
      for (const mc of mCodes) {
        const mn = parseInt(mc.substring(1));
        switch (mn) {
          case 0:
            addMessage("info", "M0 程序暂停");
            break;
          case 1:
            addMessage("info", "M1 选择性暂停");
            break;
          case 2:
            cycleRunning.value = false;
            addMessage("info", "M2 程序结束");
            break;
          case 30:
            spindleRunning.value = false;
            cycleRunning.value = false;
            addMessage("info", "M30 程序结束并回零");
            break;
          case 3:
            spindleRunning.value = true;
            addMessage("info", "主轴正转");
            break;
          case 4:
            spindleRunning.value = true;
            addMessage("info", "主轴反转");
            break;
          case 5:
            spindleRunning.value = false;
            addMessage("info", "主轴停止");
            break;
          case 6:
            if (cmd.T !== undefined) currentTool.value = Math.round(cmd.T);
            addMessage("info", `换刀 T${currentTool.value}`);
            break;
          case 7:
            coolantOn.value = true;
            addMessage("info", "M7 切削液开 (雾)");
            break;
          case 8:
            coolantOn.value = true;
            addMessage("info", "M8 切削液开");
            break;
          case 9:
            coolantOn.value = false;
            addMessage("info", "M9 切削液关");
            break;
          case 98:
            addMessage("info", "M98 子程序调用");
            break;
          case 99:
            addMessage("info", "M99 子程序返回");
            break;
        }
      }
    }

    // 处理 S 代码 (主轴转速)
    if (cmd.S !== undefined) {
      spindleRPM.value = Math.round(cmd.S);
    }

    // 处理 T 代码 (刀具)
    if (cmd.T !== undefined) {
      currentTool.value = Math.round(cmd.T);
    }

    // 处理 F 代码 (进给)
    if (cmd.F !== undefined) {
      feedRate = cmd.F;
    }

    // 去掉 G/M/S/T/F 字母，剩下的是坐标
    let nx =
      cmd.X !== undefined ? (absolute ? cmd.X : currentX + cmd.X) : currentX;
    let ny =
      cmd.Y !== undefined ? (absolute ? cmd.Y : currentY + cmd.Y) : currentY;
    let nz =
      cmd.Z !== undefined ? (absolute ? cmd.Z : currentZ + cmd.Z) : currentZ;

    if (
      cmd.X !== undefined ||
      cmd.Y !== undefined ||
      cmd.Z !== undefined ||
      modalG === "G02" ||
      modalG === "G03"
    ) {
      if (modalG === "G02" || modalG === "G03") {
        // 圆弧插补: 简化为中间点插值
        const r = cmd.R || 10;
        const steps = 20;
        const dir = modalG === "G02" ? 1 : -1;
        for (let i = 1; i <= steps; i++) {
          const t = (i / steps) * Math.PI * 0.5 * dir;
          const ix =
            currentX + (nx - currentX) * (i / steps) + Math.sin(t) * r * 0.3;
          const iy =
            currentY + (ny - currentY) * (i / steps) + Math.cos(t) * r * 0.3;
          points.push({
            x: ix,
            y: iy,
            z: currentZ + (nz - currentZ) * (i / steps),
            feed: feedRate,
            spindle: spindleRunning.value,
          });
        }
      } else {
        points.push({
          x: nx,
          y: ny,
          z: nz,
          feed: feedRate,
          spindle: spindleRunning.value,
        });
      }
      currentX = nx;
      currentY = ny;
      currentZ = nz;
    }
  }

  pathPoints.value = points.map((p) => ({ x: p.x, y: p.y, z: p.z }));
  addMessage(
    "info",
    `G 代码解析完成，${points.length} 个路径点，开始动画执行...`,
  );

  // 动画执行
  animatePath(points);
}

function animatePath(
  points: { x: number; y: number; z: number; feed: number; spindle: boolean }[],
) {
  if (pathAnimTimer) clearInterval(pathAnimTimer);
  currentPathIdx.value = 0;

  const executedPoints: THREE.Vector3[] = [];

  pathAnimTimer = setInterval(() => {
    if (currentPathIdx.value >= points.length) {
      clearInterval(pathAnimTimer!);
      pathAnimTimer = null;
      cycleRunning.value = false;
      currentFeed.value = 0;
      addMessage("info", "仿真执行完成");
      return;
    }

    const pt = points[currentPathIdx.value];
    axisPositions.X = pt.x;
    axisPositions.Y = pt.y;
    axisPositions.Z = pt.z;
    spindleRunning.value = pt.spindle;
    currentFeed.value = pt.feed * (feedOverride.value / 100);
    currentPathIdx.value++;

    updateMachinePosition();

    // 绘制已执行路径
    executedPoints.push(new THREE.Vector3(pt.x, pt.y, pt.z));
    updateExecutedPath(executedPoints);
  }, 50);
}

function updateExecutedPath(points: THREE.Vector3[]) {
  if (!scene) return;
  if (executedPathLine) scene.remove(executedPathLine);
  if (points.length < 2) return;
  const geometry = new THREE.BufferGeometry().setFromPoints(points);
  const material = new THREE.LineBasicMaterial({
    color: 0x00ff88,
    linewidth: 2,
  });
  executedPathLine = new THREE.Line(geometry, material);
  scene.add(executedPathLine);
}

function updateMachinePosition() {
  if (!machineGroup) return;
  const axes = currentMachineInfo.value.axes;
  const x = axisPositions.X || 0;
  const y = axisPositions.Y || 0;
  const z = axisPositions.Z || 0;

  const id = currentMachine.value;
  if (id === "vmc" || id === "hmc") {
    if (toolMesh) {
      toolMesh.position.x = (id === "vmc" ? -50 : 0) + x;
      toolMesh.position.y = 22 + y;
      toolMesh.position.z = z;
    }
    if (spindleHead) {
      spindleHead.position.x = (id === "vmc" ? -50 : -15) + x;
      spindleHead.position.y = 45 + y;
      spindleHead.position.z = z;
    }
  } else if (id === "lathe") {
    if (toolMesh) {
      toolMesh.position.x = x;
      toolMesh.position.z = 15 + z;
    }
  } else if (id.startsWith("5axis")) {
    if (toolMesh) {
      toolMesh.position.x = -60 + x;
      toolMesh.position.y = 20 + y;
      toolMesh.position.z = z;
    }
  } else if (id === "5axis_bcw") {
    if (toolMesh) {
      toolMesh.position.x = x;
      toolMesh.position.y = 18 + z;
      toolMesh.position.z = y;
    }
  } else if (id === "router") {
    if (toolMesh) {
      toolMesh.position.x = x;
      toolMesh.position.y = 38 + z;
      toolMesh.position.z = y;
    }
  } else if (id.startsWith("kin_")) {
    // 自定义运动学机器人：用 J1 控制旋转，末端做简单位移
    if (toolMesh) {
      const j1Rad = ((axisPositions.J1 || 0) * Math.PI) / 180;
      toolMesh.position.x = Math.sin(j1Rad) * 80;
      toolMesh.position.z = Math.cos(j1Rad) * 80;
      toolMesh.position.y = 150 + (axisPositions.J2 || 0) * 0.5;
    }
  }
}

function loadAndRun() {
  if (!gcodeContent.value.trim()) {
    addMessage("warning", "请输入 G 代码");
    return;
  }
  clearPath();
  parseAndRun(gcodeContent.value);
}

function loadSampleGcode() {
  const id = currentMachine.value;
  if (
    id === "scara" ||
    id === "delta" ||
    id === "puma" ||
    id === "fanuc" ||
    id.startsWith("kin_")
  ) {
    gcodeContent.value = `; 示例关节运动
G90 G21
G0 J1=0 J2=0 J3=0 J4=0
G1 J1=45 F200
G1 J2=30 F150
G1 J1=0 J2=0 F300
M30`;
  } else if (id === "lathe") {
    gcodeContent.value = `; 示例车削
G90 G21 G18
G0 X20 Z0
G1 X0 Z-30 F100
G1 X20 Z-60 F150
G1 X25 Z-60
G0 X30 Z0
M30`;
  } else {
    gcodeContent.value = `; 示例铣削路径
G90 G21 G17
G0 X0 Y0 Z10
G1 Z-2 F100
M3 S3000
G1 X50 F300
G1 Y40
G1 X0
G1 Y0
G2 X50 Y40 R30 F200
M5
G0 Z10
G0 X0 Y0
M30`;
  }
}

function clearPath() {
  pathPoints.value = [];
  currentPathIdx.value = 0;
  if (pathAnimTimer) {
    clearInterval(pathAnimTimer);
    pathAnimTimer = null;
  }
  if (toolPathLine && scene) {
    scene.remove(toolPathLine);
    toolPathLine = null;
  }
  if (executedPathLine && scene) {
    scene.remove(executedPathLine);
    executedPathLine = null;
  }
}

function addMessage(level: string, text: string) {
  messages.value.push({ level, text });
  if (messages.value.length > 100) messages.value.shift();
  nextTick(() => {
    if (messageArea.value)
      messageArea.value.scrollTop = messageArea.value.scrollHeight;
  });
}
</script>

<style scoped>
.cncsim-panel {
  width: 100%;
  height: 100%;
}
.cncsim-layout {
  width: 100%;
  height: 100%;
  display: flex;
}
.control-panel {
  width: 260px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  overflow-y: auto;
  padding: 8px;
}
.panel-section {
  margin-bottom: 10px;
}
.section-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
  margin-bottom: 4px;
  padding: 3px 0;
}
.machine-select {
  width: 100%;
  padding: 4px 6px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 11px;
  font-family: "Inter", sans-serif;
  outline: none;
}
.machine-select:focus {
  border-color: var(--primary);
}
.machine-desc {
  font-size: 10px;
  color: var(--on-surface-variant);
  margin-top: 4px;
}
.machine-axes {
  font-size: 10px;
  color: var(--on-surface-variant);
  margin-top: 2px;
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
}
.axis-tag {
  padding: 1px 5px;
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-radius: 3px;
  font-size: 9px;
  font-weight: 700;
  font-family: "JetBrains Mono", monospace;
}
.control-row {
  display: flex;
  gap: 4px;
}
.ctrl-btn {
  display: flex;
  align-items: center;
  gap: 3px;
  padding: 5px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 10px;
  font-family: "Inter", sans-serif;
  cursor: pointer;
  transition: all 0.15s;
  flex: 1;
  justify-content: center;
}
.ctrl-btn:hover:not(:disabled) {
  background: var(--surface-container-high);
}
.ctrl-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.ctrl-btn.active {
  background: var(--secondary-container);
  color: var(--on-secondary-container);
  border-color: var(--secondary);
}
.ctrl-btn.estop {
  background: var(--error-container);
  color: var(--on-error-container);
  border-color: var(--error);
}
.ctrl-btn .material-symbols-outlined {
  font-size: 14px;
}
.jog-grid {
  display: flex;
  flex-direction: column;
  gap: 3px;
}
.jog-row {
  display: flex;
  align-items: center;
  gap: 3px;
}
.axis-label {
  width: 22px;
  font-size: 10px;
  font-weight: 700;
  color: var(--primary);
  font-family: "JetBrains Mono", monospace;
}
.jog-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  cursor: pointer;
  font-size: 12px;
}
.jog-btn:hover:not(:disabled) {
  background: var(--surface-container-high);
}
.jog-btn:disabled {
  opacity: 0.4;
}
.jog-value {
  flex: 1;
  text-align: center;
  font-size: 10px;
  font-family: "JetBrains Mono", monospace;
  color: var(--secondary);
}
.feed-slider {
  width: 100%;
  margin: 3px 0;
}
.gcode-input {
  width: 100%;
  padding: 5px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 10px;
  font-family: "JetBrains Mono", monospace;
  resize: vertical;
  outline: none;
}
.gcode-input:focus {
  border-color: var(--primary);
}
.load-btn {
  flex: 2;
}
/* 3D 视口 */
.viewport-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}
.viewport-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 12px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  font-size: 11px;
  color: var(--on-surface-variant);
}
.viewport-controls {
  display: flex;
  gap: 4px;
}
.view-btn {
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
}
.view-btn:hover {
  background: var(--surface-variant);
}
.view-btn .material-symbols-outlined {
  font-size: 16px;
}
.viewport-3d {
  flex: 1;
  overflow: hidden;
  position: relative;
}
.viewport-3d canvas {
  display: block;
  width: 100%;
  height: 100%;
}
.path-info {
  position: absolute;
  bottom: 8px;
  left: 8px;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 10px;
  background: rgba(0, 0, 0, 0.7);
  border-radius: var(--radius);
  font-size: 11px;
  color: var(--on-surface);
}
.spindle-indicator {
  color: var(--secondary);
  font-family: "JetBrains Mono", monospace;
}
.clear-btn {
  padding: 2px 8px;
  background: var(--error-container);
  border: none;
  border-radius: var(--radius);
  color: var(--on-error-container);
  font-size: 10px;
  cursor: pointer;
}
/* 状态面板 */
.status-panel {
  width: 210px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-left: 1px solid var(--outline-variant);
  overflow-y: auto;
  padding: 8px;
}
.dro-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 3px 6px;
  background: var(--surface-dim);
  border-radius: var(--radius);
  margin-bottom: 3px;
}
.dro-axis {
  font-size: 11px;
  font-weight: 700;
  color: var(--on-surface-variant);
  font-family: "JetBrains Mono", monospace;
}
.dro-value {
  font-size: 12px;
  font-family: "JetBrains Mono", monospace;
}
.dro-value.mcs {
  color: var(--secondary);
}
.dro-value.wcs {
  color: var(--primary);
}
.status-row {
  display: flex;
  justify-content: space-between;
  padding: 2px 0;
  font-size: 10px;
}
.status-label {
  color: var(--on-surface-variant);
}
.status-val {
  color: var(--on-surface);
  font-family: "JetBrains Mono", monospace;
}
.spindle-on {
  color: var(--secondary);
}
.gcode-modes {
  display: flex;
  flex-wrap: wrap;
  gap: 3px;
}
.gmode {
  padding: 2px 5px;
  background: var(--surface-variant);
  border-radius: var(--radius);
  font-size: 9px;
  font-family: "JetBrains Mono", monospace;
  color: var(--tertiary);
}
.message-area {
  max-height: 120px;
  overflow-y: auto;
}
.msg-item {
  font-size: 9px;
  padding: 1px 0;
  font-family: "JetBrains Mono", monospace;
}
.msg-info {
  color: var(--on-surface-variant);
}
.msg-warning {
  color: #fbabff;
}
.msg-error {
  color: var(--error);
}
</style>
