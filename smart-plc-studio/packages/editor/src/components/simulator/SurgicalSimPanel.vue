<template>
  <div class="surgical-panel">
    <!-- 左侧控制面板 -->
    <div class="left-panel">
      <!-- WebSocket 连接栏 -->
      <div class="ws-bar">
        <input v-model="wsUrl" placeholder="ws://localhost:9090/ws (dVRK ROS2 桥接)" class="ws-input" />
        <button @click="wsConnected ? disconnectWS() : connectWS()"
          :class="wsConnected ? 'ws-btn connected' : 'ws-btn'">
          {{ wsConnected ? '已连接' : '连接' }}
        </button>
        <span class="ws-dot" :class="{ online: wsConnected }"></span>
      </div>
      <div class="hint" v-if="wsConnected">订阅 {{ jsTopic }} 实时驱动真实 dVRK 模型</div>

      <div class="panel-section">
        <h3>机器人选择</h3>
        <select v-model="robotType" class="sel" :disabled="loading">
          <option v-for="(cfg, key) in MODELS" :key="key" :value="key">{{ cfg.name }}</option>
        </select>
        <div class="hint" v-if="loading">正在加载真实模型网格...</div>
        <div class="hint" v-else>SUJ 推车 + MTML/MTMR + PSM1-3 + ECM 全部组装显示</div>
      </div>

      <div class="panel-section">
        <h3>器官手术 (DejaVu)</h3>
        <div class="param-row">
          <label>器官</label>
          <select v-model="organType" class="sel" :disabled="organLoading">
            <option value="none">无</option>
            <option v-for="(cfg, key) in ORGAN_CONFIGS" :key="key" :value="key">{{ cfg.name }}</option>
          </select>
        </div>
        <div class="hint" v-if="organLoading">正在加载器官网格...</div>
        <div class="hint" v-else-if="organType !== 'none'">SOFA 风格软体网格 · 抓取/牵拉/切割实时形变</div>
        <div class="param-row" v-if="organType !== 'none'">
          <label>显示部件</label>
          <span class="switch-row">
            <label class="switch">
              <input type="checkbox" v-model="showParts" />
              <span class="slider"></span>
            </label>
            <span class="switch-label">血管/肿瘤</span>
          </span>
        </div>
        <div class="param-row" v-if="organType !== 'none'">
          <label>工具模式</label>
          <div class="mode-row">
            <button :class="{ active: toolMode === 'view' }" @click="toolMode = 'view'">查看</button>
            <button :class="{ active: toolMode === 'grab' }" @click="toolMode = 'grab'">牵拉</button>
            <button :class="{ active: toolMode === 'cut' }" @click="toolMode = 'cut'">切割</button>
          </div>
        </div>
        <div class="param-row" v-if="organType !== 'none'">
          <label>物理后端</label>
          <div class="mode-row">
            <button :class="{ active: physicsBackend === 'mini' }" @click="physicsBackend = 'mini'">JS 内置</button>
            <button :class="{ active: physicsBackend === 'sofa' }" @click="physicsBackend = 'sofa'">SOFA 桥接</button>
          </div>
        </div>
        <div class="hint" v-if="physicsBackend === 'sofa' && organType !== 'none'">
          {{ bridgeStatus === 'online' ? 'SOFA 侧车在线 · 真实 FEM 物理' : bridgeStatus === 'connecting' ? '正在连接 SOFA 侧车...' : bridgeStatus === 'error' ? 'SOFA 侧车连接失败' : 'SOFA 侧车未连接 (ws://localhost:9090/ws)' }}
          <span v-if="bridgeStatus === 'online' && bridgeFps > 0"> · {{ bridgeFps.toFixed(0) }} fps{{ bridgeBytes > 0 ? ' · ' + (bridgeBytes / 1024).toFixed(0) + 'KB/帧' : '' }}</span>
        </div>
        <div class="param-row" v-if="organType !== 'none'">
          <label>内窥镜</label>
          <span class="switch-row">
            <label class="switch">
              <input type="checkbox" v-model="endoscope" />
              <span class="slider"></span>
            </label>
            <span class="switch-label">ECM 视角画中画</span>
          </span>
        </div>
        <div class="hint" v-if="toolMode === 'grab'">按住左键拖拽镊子 → 抓住并牵拉组织</div>
        <div class="hint" v-if="toolMode === 'cut'">按住左键划过 → 沿刀路切开组织</div>
        <div class="hint" v-if="toolMode === 'grab' || toolMode === 'cut'">滚轮控制工具深浅</div>
      </div>

      <div class="panel-section">
        <h3>ECM 手动微调</h3>
        <div class="ecm-row"><label>X (m)</label><button class="step" @click="nudge('pos.x', -0.05)">−</button><input type="number" step="0.05" v-model.number="ecmManual.pos.x" class="num" /><button class="step" @click="nudge('pos.x', 0.05)">+</button></div>
        <div class="ecm-row"><label>Y (m)</label><button class="step" @click="nudge('pos.y', -0.05)">−</button><input type="number" step="0.05" v-model.number="ecmManual.pos.y" class="num" /><button class="step" @click="nudge('pos.y', 0.05)">+</button></div>
        <div class="ecm-row"><label>Z (m)</label><button class="step" @click="nudge('pos.z', -0.05)">−</button><input type="number" step="0.05" v-model.number="ecmManual.pos.z" class="num" /><button class="step" @click="nudge('pos.z', 0.05)">+</button></div>
        <div class="ecm-row"><label>Roll (°)</label><button class="step" @click="nudge('rpy.roll', -1)">−</button><input type="number" step="1" v-model.number="ecmManual.rpy.roll" class="num" /><button class="step" @click="nudge('rpy.roll', 1)">+</button></div>
        <div class="ecm-row"><label>Pitch (°)</label><button class="step" @click="nudge('rpy.pitch', -1)">−</button><input type="number" step="1" v-model.number="ecmManual.rpy.pitch" class="num" /><button class="step" @click="nudge('rpy.pitch', 1)">+</button></div>
        <div class="ecm-row"><label>Yaw (°)</label><button class="step" @click="nudge('rpy.yaw', -1)">−</button><input type="number" step="1" v-model.number="ecmManual.rpy.yaw" class="num" /><button class="step" @click="nudge('rpy.yaw', 1)">+</button></div>
      </div>

      <div class="panel-section">
        <h3>关节控制 ({{ modelJoints.length }})</h3>
        <div v-for="j in modelJoints" :key="j.name" class="joint-row">
          <label>{{ shortName(j.name) }}</label>
          <input type="range" v-model.number="j.value" :min="j.min" :max="j.max" class="jog"
            :disabled="!simRunning && mode === 'auto'" />
          <span class="val">{{ j.isPrismatic ? j.value.toFixed(1) + 'mm' : j.value.toFixed(1) + '°' }}</span>
        </div>
      </div>

      <div class="panel-section">
        <h3>操作模式</h3>
        <div class="btn-row">
          <button :class="{ active: mode === 'manual' }" @click="mode = 'manual'">手动</button>
          <button :class="{ active: mode === 'auto' }" @click="mode = 'auto'">自动轨迹</button>
        </div>
      </div>

      <div class="panel-section">
        <h3>信息</h3>
        <div class="info-row"><span>TCP X:</span><span>{{ tcpPos.x.toFixed(3) }}</span></div>
        <div class="info-row"><span>TCP Y:</span><span>{{ tcpPos.y.toFixed(3) }}</span></div>
        <div class="info-row"><span>TCP Z:</span><span>{{ tcpPos.z.toFixed(3) }}</span></div>
        <div class="info-row"><span>模型:</span><span>{{ modelName }} (关节控制)</span></div>
      </div>
    </div>

    <!-- 中央 3D 视口 -->
    <div ref="viewportRef" class="viewport"></div>

    <!-- 右侧状态面板 -->
    <div class="right-panel">
      <div class="panel-section">
        <h3>仿真控制</h3>
        <div class="btn-row vert">
          <button @click="resetSim" class="secondary"><span class="material-symbols-outlined">restart_alt</span>重置</button>
          <button @click="toggleSim" :class="simRunning ? 'danger' : 'primary'">
            <span class="material-symbols-outlined">{{ simRunning ? 'stop' : 'play_arrow' }}</span>
            {{ simRunning ? '停止' : '运行' }}
          </button>
        </div>
      </div>

      <div class="panel-section">
        <h3>物理参数</h3>
        <div class="info-row"><span>网格:</span><span>{{ meshCount }} 个</span></div>
        <div class="info-row"><span>关节:</span><span>{{ modelJoints.length }} 个</span></div>
        <div class="info-row"><span>仿真时间:</span><span>{{ simTime.toFixed(2) }} s</span></div>
      </div>

      <div class="panel-section">
        <h3>关节当前值</h3>
        <div v-for="j in modelJoints" :key="'c'+j.name" class="limit-row">
          <span>{{ shortName(j.name) }}:</span>
          <span>{{ j.isPrismatic ? j.value.toFixed(1) + 'mm' : j.value.toFixed(1) + '°' }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, watch } from "vue";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { UrdfRobot } from "./UrdfRobot";
import { ORGAN_CONFIGS, loadOrgan, type OrganHandle } from "./surgery/organScene";
import { SurgeryTool } from "./surgery/SurgeryTool";
import { MiniSofaPhysics } from "./surgery/MiniSofaPhysics";
import { SofaBridgePhysics, type SofaBridgeStatus } from "./surgery/SofaBridgePhysics";
import type { OrganPhysicsDriver } from "./surgery/OrganPhysics";

const emit = defineEmits<{ connect: []; disconnect: [] }>();

/* ======= 模型注册表 (官方 dvrk_model 真实模型) ======= */
const MODELS = {
  SUJ: {
    name: "SUJ 推车底座",
    urdf: "/models/dvrk/SUJ.urdf",
    tip: "",
    prefix: "",
    jsTopic: "",
  },
  MTML: {
    name: "MTML (左主手)",
    urdf: "/models/dvrk/MTML.urdf",
    tip: "MTML_wrist_roll_link",
    prefix: "MTML_",
    jsTopic: "/MTML/measured_js",
  },
  MTMR: {
    name: "MTMR (右主手)",
    urdf: "/models/dvrk/MTMR.urdf",
    tip: "MTMR_wrist_roll_link",
    prefix: "MTMR_",
    jsTopic: "/MTMR/measured_js",
  },
  PSM1: {
    name: "PSM1 (从手 SCA)",
    urdf: "/models/dvrk/PSM1.urdf",
    tip: "PSM1_tool_tip_link",
    prefix: "PSM1_",
    jsTopic: "/PSM1/measured_js",
  },
  PSM2: {
    name: "PSM2 (从手 SCA)",
    urdf: "/models/dvrk/PSM2.urdf",
    tip: "PSM2_tool_tip_link",
    prefix: "PSM2_",
    jsTopic: "/PSM2/measured_js",
  },
  PSM3: {
    name: "PSM3 (从手 SCA)",
    urdf: "/models/dvrk/PSM3.urdf",
    tip: "PSM3_tool_tip_link",
    prefix: "PSM3_",
    jsTopic: "/PSM3/measured_js",
  },
  ECM: {
    name: "ECM (内窥镜)",
    urdf: "/models/dvrk/ECM.urdf",
    tip: "ECM_tool_link",
    prefix: "ECM_",
    jsTopic: "/ECM/measured_js",
  },
};

const modelName = ref("");
const jsTopic = ref("");
const loading = ref(false);

/* ======= Three.js ======= */
const viewportRef = ref<HTMLDivElement>();
let scene: THREE.Scene, cam: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer;
let controls: OrbitControls;
let animId = 0;
let robot: UrdfRobot | null = null;
const robots = new Map<string, UrdfRobot>();
let meshCount = ref(0);

/* ======= ECM 手动微调 ======= */
const ecmManual = reactive({
  pos: { x: 0, y: 0, z: 0 },          // 平移补偿 (米)
  rpy: { roll: 0, pitch: 0, yaw: 0 }, // 旋转补偿 (度)
});
let ecmBasePos: THREE.Vector3 | null = null;
let ecmBaseQuat: THREE.Quaternion | null = null;

function applyEcmManual() {
  const rb = robots.get("ECM");
  if (!rb || !ecmBasePos || !ecmBaseQuat) return;
  rb.root.position.set(
    ecmBasePos.x + ecmManual.pos.x,
    ecmBasePos.y + ecmManual.pos.y,
    ecmBasePos.z + ecmManual.pos.z
  );
  const qManual = new THREE.Quaternion().setFromEuler(
    new THREE.Euler(
      THREE.MathUtils.degToRad(ecmManual.rpy.roll),
      THREE.MathUtils.degToRad(ecmManual.rpy.pitch),
      THREE.MathUtils.degToRad(ecmManual.rpy.yaw),
      "ZYX"
    )
  );
  rb.root.quaternion.copy(ecmBaseQuat).premultiply(qManual);
}

function nudge(path: "pos.x" | "pos.y" | "pos.z" | "rpy.roll" | "rpy.pitch" | "rpy.yaw", delta: number) {
  const [group, key] = path.split(".");
  const target = group === "pos" ? ecmManual.pos : ecmManual.rpy;
  (target as unknown as Record<string, number>)[key] =
    Math.round((((target as unknown as Record<string, number>)[key] + delta) + Number.EPSILON) * 1000) / 1000;
}

/* ======= 状态 ======= */
const robotType = ref("PSM1");
const mode = ref("manual");
const simRunning = ref(false);
const dt = 0.005;
const simTime = ref(0);
const tcpPos = reactive({ x: 0, y: 0, z: 0 });

interface ModelJoint {
  name: string;
  value: number;
  min: number;
  max: number;
  isPrismatic: boolean;
}

const modelJoints = reactive<ModelJoint[]>([]);

function shortName(name: string) {
  return name.replace(/^PSM1_|^MTML_/, "");
}

function toSlider(j: { name: string; value: number; limit?: { lower: number; upper: number } }): ModelJoint {
  const isPrismatic = /insertion/.test(j.name) && !/pitch/.test(j.name) || j.name.endsWith("_insertion");
  const min = j.limit ? (isPrismatic ? j.limit.lower * 1000 : j.limit.lower * 180 / Math.PI) : -90;
  const max = j.limit ? (isPrismatic ? j.limit.upper * 1000 : j.limit.upper * 180 / Math.PI) : 90;
  const value = isPrismatic ? j.value * 1000 : j.value * 180 / Math.PI;
  return { name: j.name, value, min, max, isPrismatic };
}

function buildJointSliders() {
  modelJoints.splice(0, modelJoints.length);
  if (!robot) return;
  for (const j of robot.getJointValues()) {
    if (j.name.includes("mimic")) continue;
    modelJoints.push(toSlider(j));
  }
}

function applyJointState() {
  if (!robot) return;
  const values: Record<string, number> = {};
  for (const j of modelJoints) {
    values[j.name] = j.isPrismatic ? j.value / 1000 : j.value * Math.PI / 180;
  }
  robot.setJointValues(values);
  updateTcp();
}

function updateTcp() {
  if (!robot) return;
  const cfg = MODELS[robotType.value as keyof typeof MODELS];
  const pose = robot.getLinkPose(cfg.tip);
  if (pose) {
    tcpPos.x = pose.position.x;
    tcpPos.y = pose.position.y;
    tcpPos.z = pose.position.z;
  }
}

/* ======= ROS2 桥接 (dVRK measured_js 订阅) ======= */
const wsUrl = ref("ws://localhost:9090/ws")
const wsConnected = ref(false)
let ws: WebSocket | null = null
let wsReady = false

function connectWS() {
  if (!wsUrl.value) return
  ws = new WebSocket(wsUrl.value)
  ws.onopen = () => {
    wsConnected.value = true
    wsReady = true
    const cfg = MODELS[robotType.value as keyof typeof MODELS]
    jsTopic.value = cfg.jsTopic
    ws!.send(JSON.stringify({ type: "subscribe", topic: cfg.jsTopic }))
    console.log("ROS2 桥接已连接, 订阅", cfg.jsTopic)
  }
  ws.onclose = () => { wsConnected.value = false; wsReady = false; console.log("ROS2 桥接已断开") }
  ws.onerror = () => { wsConnected.value = false }
  ws.onmessage = (ev) => {
    let data: any
    try { data = JSON.parse(ev.data) } catch { return }
    if (data.event !== "message") return
    const payload = data.data?.payload
    const names: string[] = payload?.name || []
    const positions: number[] = payload?.position || []
    if (!names.length || !robot) return
    const cfg = MODELS[robotType.value as keyof typeof MODELS]
    for (let i = 0; i < names.length; i++) {
      const base = names[i].replace(new RegExp("^" + cfg.prefix), "")
      const slider = modelJoints.find((j) => j.name === base)
      if (!slider) continue
      const rad = positions[i] ?? 0
      slider.value = slider.isPrismatic ? rad * 1000 : rad * 180 / Math.PI
    }
    applyJointState()
  }
}

function disconnectWS() {
  if (wsReady && robot) {
    const cfg = MODELS[robotType.value as keyof typeof MODELS]
    ws!.send(JSON.stringify({ type: "unsubscribe", topic: cfg.jsTopic }))
  }
  ws?.close()
  ws = null
  wsConnected.value = false
  wsReady = false
}

/* ======= 器官手术 (DejaVu + MiniSofa / SOFA 桥接) ======= */
const organType = ref<keyof typeof ORGAN_CONFIGS | "none">("none");
const organLoading = ref(false);
const showParts = ref(true);
const toolMode = ref<"view" | "grab" | "cut">("view");
const physicsBackend = ref<"mini" | "sofa">("mini");
const bridgeUrl = ref("ws://localhost:9090/ws");
const bridgeStatus = ref<SofaBridgeStatus>("idle");
const bridgeFps = ref(0);
const bridgeBytes = ref(0);
const endoscope = ref(false);

let organ: OrganHandle | null = null;
let organDriver: OrganPhysicsDriver | null = null;
let tool: SurgeryTool | null = null;
let toolDepth = 0.05;
let grabbing = false;
let lastCutPoint: THREE.Vector3 | null = null;
const tipWorld = new THREE.Vector3();

const ORGAN_ORIGIN = new THREE.Vector3(0.28, 0.02, 0.66);
const ORGAN_TIP_DIST = 0.14;

async function loadOrganAsync(type: keyof typeof ORGAN_CONFIGS | string) {
  const cfg = ORGAN_CONFIGS[type as keyof typeof ORGAN_CONFIGS];
  if (!cfg) return;
  organLoading.value = true;
  try {
    if (organ) disposeOrgan();
    organ = await loadOrgan(cfg);
    organ.group.position.copy(ORGAN_ORIGIN);
    if (physicsBackend.value === "sofa") {
      organDriver = new SofaBridgePhysics(organ.mainMesh.geometry, {
        onStatus: (s) => (bridgeStatus.value = s),
        onStats: (st) => {
          bridgeFps.value = st.fps;
          bridgeBytes.value = st.bytesPerFrame;
        },
      });
      (organDriver as SofaBridgePhysics).connect(bridgeUrl.value);
    } else {
      organDriver = new MiniSofaPhysics(organ.mainMesh.geometry, {
        grabRadius: cfg.grabRadius,
        cutRadius: cfg.cutRadius,
      });
    }
    organ.mainMesh.userData["driver"] = organDriver;
    applyPartsVisibility();
    scene.add(organ.group);

    tool = new SurgeryTool(toolMode.value === "cut" ? "scalpel" : "forceps");
    tool.scale.setScalar(1);
    scene.add(tool);
    updateToolPosition();

    setPartsOpacity();
  } catch (e) {
    console.error("加载器官失败:", e);
  } finally {
    organLoading.value = false;
  }
}

function disposeOrgan() {
  if (organDriver) {
    organDriver.dispose();
    organDriver = null;
  }
  if (organ) {
    scene.remove(organ.group);
    organ.dispose();
    organ = null;
  }
  if (tool) {
    scene.remove(tool);
    tool = null;
  }
  bridgeStatus.value = "idle";
  bridgeFps.value = 0;
  grabbing = false;
  lastCutPoint = null;
}

function applyPartsVisibility() {
  if (!organ) return;
  for (const p of organ.parts) {
    p.group.visible = showParts.value;
  }
}

function setPartsOpacity() {
  if (!organ) return;
  const base = organ.config.partsOpacity ?? 1;
  for (const p of organ.parts) {
    const m = p.mesh.material as THREE.MeshStandardMaterial;
    if (m && m.transparent !== undefined) m.opacity = base;
  }
}

function updateToolPosition() {
  if (!tool) return;
  const ndc = new THREE.Vector2(toolPointer.x, toolPointer.y);
  const ray = new THREE.Raycaster();
  ray.setFromCamera(ndc, cam);

  let target: THREE.Vector3;
  if (organ) {
    const hits = ray.intersectObject(organ.mainMesh, false);
    if (hits.length) {
      target = hits[0].point.clone();
    } else {
      target = ray.ray.origin.clone().addScaledVector(ray.ray.direction, ORGAN_TIP_DIST + toolDepth);
    }
  } else {
    target = ray.ray.origin.clone().addScaledVector(ray.ray.direction, ORGAN_TIP_DIST);
  }
  tipWorld.copy(target);

  tool.position.copy(target);
  tool.orient(target, cam.position);
  tool.visible = toolMode.value !== "view";
}

const toolPointer = reactive({ x: 0, y: 0, down: false });

function onPointerDown(e: PointerEvent) {
  if (!organ || !tool) return;
  if (toolMode.value === "view") return;
  const el = renderer.domElement;
  const rect = el.getBoundingClientRect();
  toolPointer.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
  toolPointer.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
  toolPointer.down = true;
  controls.enabled = false;
  updateToolPosition();
  if (toolMode.value === "grab") {
    const local = toOrganLocal(tipWorld);
    organDriver?.grab(local.x, local.y, local.z);
    grabbing = true;
    tool?.setJaw(0.4);
  } else if (toolMode.value === "cut") {
    lastCutPoint = toOrganLocal(tipWorld);
  }
}

function onPointerMove(e: PointerEvent) {
  if (!organ || !tool) return;
  const el = renderer.domElement;
  const rect = el.getBoundingClientRect();
  toolPointer.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
  toolPointer.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
  updateToolPosition();
  if (!toolPointer.down) return;
  if (toolMode.value === "grab" && grabbing) {
    const local = toOrganLocal(tipWorld);
    organDriver?.setGrabTarget(local.x, local.y, local.z);
  } else if (toolMode.value === "cut") {
    const cur = toOrganLocal(tipWorld);
    if (lastCutPoint && cur.distanceTo(lastCutPoint) > 0.002) {
      organDriver?.cut(
        lastCutPoint.x, lastCutPoint.y, lastCutPoint.z,
        cur.x, cur.y, cur.z
      );
      lastCutPoint.copy(cur);
    }
  }
}

function onPointerUp() {
  toolPointer.down = false;
  controls.enabled = true;
  if (grabbing) {
    organDriver?.release();
    grabbing = false;
  }
  lastCutPoint = null;
  tool?.setJaw(0.75);
}

function onWheel(e: WheelEvent) {
  if (toolMode.value === "grab" || toolMode.value === "cut") {
    e.preventDefault();
    toolDepth = Math.min(0.2, Math.max(-0.05, toolDepth + e.deltaY * 0.0002));
    updateToolPosition();
  }
}

function toOrganLocal(world: THREE.Vector3) {
  if (!organ) return world.clone();
  const s = organ.group.scale.x;
  return new THREE.Vector3(
    (world.x - organ.group.position.x) / s,
    (world.y - organ.group.position.y) / s,
    (world.z - organ.group.position.z) / s
  );
}

function organStep() {
  if (!organ || !organDriver || !simRunning.value) return;
  organDriver.solve(dt);
}

function resetOrgan() {
  organDriver?.reset();
}

function endoscopeRender() {
  if (!endoscope.value || !organ) return;
  const ecRb = robots.get("ECM");
  const pose = ecRb?.getLinkPose("ECM_tool_link");
  if (!pose) return;
  const w = renderer.domElement.clientWidth;
  const h = renderer.domElement.clientHeight;
  const vw = Math.min(300, w * 0.32);
  const vh = vw * (9 / 16);
  const x = w - vw - 12;
  const y = h - vh - 12;

  scopeCam.position.copy(pose.position);
  scopeCam.quaternion.copy(pose.quaternion);
  scopeCam.rotateY(Math.PI);

  renderer.setViewport(x, y, vw, vh);
  renderer.setScissor(x, y, vw, vh);
  renderer.setScissorTest(true);
  renderer.render(scene, scopeCam);
  renderer.setScissorTest(false);
  renderer.setViewport(0, 0, w, h);
}

let scopeCam: THREE.PerspectiveCamera;
let organPointerBind = false;

/* ======= 场景 ======= */
function initThree() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x1a1a2e);

  const container = viewportRef.value!;
  const w = container.clientWidth;
  const h = container.clientHeight;

  cam = new THREE.PerspectiveCamera(45, w / h, 0.01, 100);
  cam.position.set(2.5, -3.5, 3.0);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  container.appendChild(renderer.domElement);

  controls = new OrbitControls(cam, renderer.domElement);
  controls.target.set(0, 0, 0.8);
  controls.update();

  const amb = new THREE.AmbientLight(0x8090b0, 0.9);
  scene.add(amb);
  const dir = new THREE.DirectionalLight(0xffffff, 1.4);
  dir.position.set(1.5, 2.5, 1.5);
  dir.castShadow = true;
  scene.add(dir);
  const fill = new THREE.DirectionalLight(0x8888ff, 0.5);
  fill.position.set(-1.5, 0.8, -1);
  scene.add(fill);

  const axes = new THREE.AxesHelper(0.5);
  scene.add(axes);

  scopeCam = new THREE.PerspectiveCamera(45, 1, 0.001, 10);

  const el = renderer.domElement;
  el.style.touchAction = "none";
  el.addEventListener("pointerdown", onPointerDown);
  el.addEventListener("pointermove", onPointerMove);
  el.addEventListener("pointerup", onPointerUp);
  el.addEventListener("pointercancel", onPointerUp);
  el.addEventListener("wheel", onWheel, { passive: false });
  organPointerBind = true;

  loadAllRobots();
  animate();
}

async function loadAllRobots() {
  loading.value = true;

  // standalone URDF 内部 world->base 的偏移 (xyz, rpy)
  const INTERNAL_OFFSETS: Record<string, { xyz: [number, number, number]; rpy: [number, number, number] }> = {
    PSM1: { xyz: [-0.25, 0, 0.5], rpy: [0, 0, Math.PI] },
    PSM2: { xyz: [-0.25, 0, 0.5], rpy: [0, 0, Math.PI] },
    PSM3: { xyz: [ 0.50, 0, 0.5], rpy: [0, 0, -Math.PI] },
    ECM:  { xyz: [-0.612599, 0, -0.101595], rpy: [0, 0, 0] },
    MTML: { xyz: [-0.25, 0, 1.0], rpy: [0, 0, 0] },
    MTMR: { xyz: [ 0.25, 0, 1.0], rpy: [0, 0, 0] },
  };

  // MTM 医生控制台位置 (base_link 应放置的世界坐标)
  const MTM_CONSOLE: Record<string, { pos: [number, number, number]; rpy: [number, number, number] }> = {
    MTML: { pos: [-0.4, -2.0, 0.75], rpy: [0, 0, 0] },
    MTMR: { pos: [ 0.4, -2.0, 0.75], rpy: [0, 0, 0] },
  };

  // ---- 第 1 步: 加载 SUJ 推车底座 ----
  const sujCfg = MODELS.SUJ;
  const sujRobot = new UrdfRobot(sujCfg.urdf, "/models/dvrk");
  const sujRb = await sujRobot.load();
  sujRb.root.position.set(0, 0, 0);
  // 设置 SUJ_ECM_J1 = 32 度，使 ECM 臂处于合理位置
  sujRobot.setJointValue("SUJ_ECM_J1", THREE.MathUtils.degToRad(32));
  sujRb.root.updateMatrixWorld(true);
  robots.set("SUJ", sujRb);
  scene.add(sujRb.root);

  // ---- 第 2 步: 从 SUJ 读取各安装点的世界坐标变换 ----
  const mountPoses: Record<string, { pos: THREE.Vector3; quat: THREE.Quaternion }> = {};
  const mountLinks: Record<string, string> = {
    PSM1: "PSM1_mounting_point",
    PSM2: "PSM2_mounting_point",
    PSM3: "PSM3_mounting_point",
    ECM:  "ECM_mounting_point",
  };
  for (const [type, linkName] of Object.entries(mountLinks)) {
    const pose = sujRobot.getLinkPose(linkName);
    if (pose) {
      mountPoses[type] = {
        pos: pose.position.clone(),
        quat: pose.quaternion.clone(),
      };
    } else {
      console.warn(`${type} mount point "${linkName}" not found in SUJ!`);
    }
  }

  // ---- 第 3 步: 加载 PSM/ECM 机械臂并定位到安装点 ----
  const armTypes: (keyof typeof MODELS)[] = ["PSM1", "PSM2", "PSM3", "ECM"];
  for (const type of armTypes) {
    const cfg = MODELS[type];
    if (robots.has(type)) continue;
    try {
      const r = new UrdfRobot(cfg.urdf, "/models/dvrk");
      const rb = await r.load();
      const mount = mountPoses[type];
      const offset = INTERNAL_OFFSETS[type];
      if (mount && offset) {
        const qMount = mount.quat;
        const qOffset = new THREE.Quaternion().setFromEuler(
          new THREE.Euler(offset.rpy[0], offset.rpy[1], offset.rpy[2], "ZYX")
        );
        const qWorld = qMount.clone().multiply(qOffset.invert());
        const vOffset = new THREE.Vector3(offset.xyz[0], offset.xyz[1], offset.xyz[2]);
        const vRotated = vOffset.clone().applyQuaternion(qWorld);
        rb.root.position.set(
          mount.pos.x - vRotated.x,
          mount.pos.y - vRotated.y,
          mount.pos.z - vRotated.z
        );
        rb.root.quaternion.copy(qWorld);
        // ECM 手动微调 (由界面实时调节, 见 ecmManual)
        if (type === "ECM") {
          ecmBasePos = rb.root.position.clone();
          ecmBaseQuat = rb.root.quaternion.clone();
          applyEcmManual();
        }
      }
      robots.set(type, rb);
      scene.add(rb.root);
    } catch (e) {
      console.error(`加载 ${cfg.name} 失败:`, e);
    }
  }

  // ---- 第 4 步: 加载 MTM 主手 ----
  const mtmTypes: (keyof typeof MODELS)[] = ["MTML", "MTMR"];
  for (const type of mtmTypes) {
    const cfg = MODELS[type];
    if (robots.has(type)) continue;
    try {
      const r = new UrdfRobot(cfg.urdf, "/models/dvrk");
      const rb = await r.load();
      const consolePos = MTM_CONSOLE[type];
      const offset = INTERNAL_OFFSETS[type];
      const qConsole = new THREE.Quaternion().setFromEuler(
        new THREE.Euler(consolePos.rpy[0], consolePos.rpy[1], consolePos.rpy[2], "ZYX")
      );
      const qOffset = new THREE.Quaternion().setFromEuler(
        new THREE.Euler(offset.rpy[0], offset.rpy[1], offset.rpy[2], "ZYX")
      );
      const qWorld = qConsole.clone().multiply(qOffset.invert());
      const vOffset = new THREE.Vector3(offset.xyz[0], offset.xyz[1], offset.xyz[2]);
      const vRotated = vOffset.clone().applyQuaternion(qWorld);
      rb.root.position.set(
        consolePos.pos[0] - vRotated.x,
        consolePos.pos[1] - vRotated.y,
        consolePos.pos[2] - vRotated.z
      );
      rb.root.quaternion.copy(qWorld);
      robots.set(type, rb);
      scene.add(rb.root);
    } catch (e) {
      console.error(`加载 ${cfg.name} 失败:`, e);
    }
  }

  setActiveRobot(robotType.value);
  loading.value = false;
}

function setActiveRobot(type: string) {
  const rb = robots.get(type);
  robot = rb ?? null;
  modelName.value = MODELS[type as keyof typeof MODELS].name;
  jsTopic.value = MODELS[type as keyof typeof MODELS].jsTopic;
  buildJointSliders();
  let count = 0;
  rb?.root.traverse((child) => { if ((child as THREE.Mesh).isMesh) count++; });
  meshCount.value = count;
  applyJointState();
  updateTcp();
}

function animate() {
  animId = requestAnimationFrame(animate);
  controls.update();
  organStep();
  if (tool && organ) {
    tool.setColor(grabbing ? 0xff5252 : 0x4caf50);
  }
  endoscopeRender();
  renderer.render(scene, cam);
}

/* ======= 控制 ======= */
function resetSim() {
  for (const rb of robots.values()) rb.reset();
  for (const j of modelJoints) j.value = 0;
  simTime.value = 0;
  resetOrgan();
  updateTcp();
}

function toggleSim() {
  simRunning.value = !simRunning.value;
  if (simRunning.value) emit("connect");
  else emit("disconnect");
}

/* ======= 自动轨迹 (真实关节正弦轨迹) ======= */
let autoTimer = 0;
function autoStep() {
  if (!simRunning.value || mode.value !== "auto" || !robot) return;
  autoTimer += dt;
  const t = autoTimer;

  const map: Record<string, number> = {};
  if (robotType.value.startsWith("PSM") || robotType.value === "ECM") {
    map["yaw"] = 0.3 * Math.sin(t * 0.6);
    map["pitch"] = 0.5 * Math.sin(t * 0.8);
    map["insertion"] = 0.05 + 0.04 * Math.sin(t * 0.5);
    map["roll"] = 0.6 * Math.sin(t * 0.9);
    map["wrist_pitch"] = 0.5 * Math.sin(t * 1.1);
    map["wrist_yaw"] = 0.4 * Math.sin(t * 0.7);
    if (robotType.value.startsWith("PSM"))
      map["jaw"] = 0.4 + 0.3 * Math.sin(t * 1.3);
  } else {
    map["outer_yaw"] = 0.4 * Math.sin(t * 0.6);
    map["shoulder_pitch"] = 0.4 * Math.sin(t * 0.8);
    map["elbow_pitch"] = 0.5 * Math.sin(t * 0.9);
    map["wrist_platform"] = 0.5 * Math.sin(t * 0.5);
    map["wrist_pitch"] = 0.4 * Math.sin(t * 1.1);
    map["wrist_yaw"] = 0.3 * Math.sin(t * 0.7);
    map["wrist_roll"] = 0.6 * Math.sin(t * 1.0);
  }
  for (const j of modelJoints) {
    const rad = map[j.name];
    if (rad === undefined) continue;
    j.value = j.isPrismatic ? rad * 1000 : rad * 180 / Math.PI;
  }
  applyJointState();
  simTime.value += dt;
}

/* ======= 监听 ======= */
watch(modelJoints, () => { if (mode.value === "manual") applyJointState(); }, { deep: true });

watch(ecmManual, () => applyEcmManual(), { deep: true });

watch(robotType, (val, oldVal) => {
  if (wsReady && ws) {
    const oldCfg = MODELS[oldVal as keyof typeof MODELS];
    if (oldCfg) ws.send(JSON.stringify({ type: "unsubscribe", topic: oldCfg.jsTopic }));
  }
  setActiveRobot(val);
  if (wsReady && ws) {
    jsTopic.value = MODELS[val as keyof typeof MODELS].jsTopic;
    ws.send(JSON.stringify({ type: "subscribe", topic: jsTopic.value }))
  }
});

watch(organType, (val) => {
  if (val === "none") {
    disposeOrgan();
    return;
  }
  loadOrganAsync(val);
});

watch(showParts, () => applyPartsVisibility());

watch(toolMode, (val) => {
  if (!organ) return;
  if (grabbing) {
    organDriver?.release();
    grabbing = false;
  }
  if (tool) {
    tool.kind = val === "cut" ? "scalpel" : "forceps";
    scene.remove(tool);
    tool = new SurgeryTool(tool.kind);
    scene.add(tool);
    updateToolPosition();
  }
  toolDepth = 0.05;
});

watch(physicsBackend, () => {
  if (organType.value === "none") return;
  loadOrganAsync(organType.value);
});

watch(endoscope, (val) => {
  if (!val || !organ) return;
  const ecRb = robots.get("ECM");
  if (ecRb) {
    const pose = ecRb.getLinkPose("ECM_tool_link");
    if (pose) {
      scopeCam.position.copy(pose.position);
      scopeCam.quaternion.copy(pose.quaternion);
    }
  }
});

/* ======= 生命周期 ======= */
onMounted(() => {
  initThree();
  setInterval(autoStep, dt * 1000);
});

onUnmounted(() => {
  cancelAnimationFrame(animId);
  disconnectWS();
  disposeOrgan();
  if (renderer && viewportRef.value) {
    if (organPointerBind) {
      const el = renderer.domElement;
      el.removeEventListener("pointerdown", onPointerDown);
      el.removeEventListener("pointermove", onPointerMove);
      el.removeEventListener("pointerup", onPointerUp);
      el.removeEventListener("pointercancel", onPointerUp);
      el.removeEventListener("wheel", onWheel);
    }
    viewportRef.value.removeChild(renderer.domElement);
    renderer.dispose();
  }
});
</script>

<style scoped>
.surgical-panel {
  display: flex;
  height: 100%;
  width: 100%;
  background: var(--background);
}

.left-panel, .right-panel {
  width: 260px;
  min-width: 260px;
  padding: 12px;
  overflow-y: auto;
  background: var(--surface-container-low);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.right-panel {
  border-right: none;
  border-left: 1px solid var(--outline-variant);
}

.viewport {
  flex: 1;
  overflow: hidden;
  position: relative;
}

.panel-section {
  background: var(--surface-container);
  border-radius: var(--radius);
  padding: 10px;
}

.panel-section h3 {
  font-size: 11px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: var(--on-surface-variant);
  margin: 0 0 8px 0;
}

.hint {
  font-size: 11px;
  color: var(--on-surface-variant);
  padding: 4px 2px;
}

.sel {
  width: 100%;
  padding: 6px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 13px;
}

.joint-row, .xyz-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}

.ecm-row {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-bottom: 4px;
}

.ecm-row label {
  width: 62px;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
  flex-shrink: 0;
}

.ecm-row .step {
  flex-shrink: 0;
  width: 30px;
  height: 30px;
  border: 1px solid var(--outline-variant);
  border-radius: 6px;
  background: var(--surface-variant);
  color: var(--on-surface);
  font-size: 18px;
  font-weight: 700;
  line-height: 1;
  cursor: pointer;
  transition: all var(--transition-fast);
  display: flex;
  align-items: center;
  justify-content: center;
}

.ecm-row .step:hover { background: var(--surface-hover); }
.ecm-row .step:active { background: var(--primary-container); color: var(--on-primary-container); }

.ecm-row .num {
  flex: 1;
  min-width: 0;
  padding: 5px 6px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 11px;
  font-family: monospace;
  text-align: right;
}

.ecm-row input[type="number"]::-webkit-inner-spin-button,
.ecm-row input[type="number"]::-webkit-outer-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.ecm-row input[type="number"] { -moz-appearance: textfield; }

.joint-row label, .xyz-row label {
  width: 78px;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
}

.jog {
  flex: 1;
  height: 4px;
  -webkit-appearance: none;
  appearance: none;
  background: var(--surface-variant);
  border-radius: 2px;
  outline: none;
}

.jog::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: var(--primary);
  cursor: pointer;
}

.val {
  width: 62px;
  text-align: right;
  font-size: 11px;
  font-family: monospace;
  color: var(--on-surface);
}

.btn-row {
  display: flex;
  gap: 4px;
}

.btn-row.vert {
  flex-direction: column;
}

.btn-row button {
  flex: 1;
  padding: 6px 8px;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  background: var(--surface-variant);
  color: var(--on-surface);
  cursor: pointer;
  font-size: 12px;
  transition: all var(--transition-fast);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
}

.btn-row button:hover { background: var(--surface-hover); }
.btn-row button.active { background: var(--primary-container); color: var(--on-primary-container); border-color: var(--primary); }
.btn-row button.primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-row button.danger { background: var(--error); color: var(--on-error); border-color: var(--error); }
.btn-row button.secondary { background: var(--surface-variant); color: var(--on-surface-variant); }

.info-row, .limit-row {
  display: flex;
  justify-content: space-between;
  font-size: 12px;
  padding: 2px 0;
  font-family: monospace;
}

.param-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
}

.param-row label {
  width: 60px;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
  flex-shrink: 0;
}

.param-row .sel { flex: 1; }

.mode-row {
  display: flex;
  gap: 4px;
  flex: 1;
}

.mode-row button {
  flex: 1;
  padding: 5px 4px;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  background: var(--surface-variant);
  color: var(--on-surface);
  cursor: pointer;
  font-size: 11px;
  transition: all var(--transition-fast);
}

.mode-row button:hover { background: var(--surface-hover); }
.mode-row button.active { background: var(--primary-container); color: var(--on-primary-container); border-color: var(--primary); }

.switch-row { display: flex; align-items: center; gap: 6px; flex: 1; }
.switch-label { font-size: 11px; color: var(--on-surface); }

.switch { position: relative; display: inline-block; width: 32px; height: 18px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.switch .slider {
  position: absolute; cursor: pointer; inset: 0;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: 9px;
  transition: all var(--transition-fast);
}
.switch .slider::before {
  content: ""; position: absolute; width: 12px; height: 12px;
  left: 2px; top: 2px; border-radius: 50%;
  background: var(--on-surface-variant);
  transition: all var(--transition-fast);
}
.switch input:checked + .slider { background: var(--primary-container); border-color: var(--primary); }
.switch input:checked + .slider::before { transform: translateX(14px); background: var(--primary); }

.limit-row.violated { color: var(--error); }
.danger { color: var(--error); font-weight: 600; }
.ok { color: var(--primary); }

.ws-bar { display: flex; align-items: center; gap: 6px; padding: 6px; background: var(--surface-dim); border-radius: var(--radius); margin-bottom: 8px; }
.ws-input { flex:1; padding:4px 8px; background:var(--surface-container); border:1px solid var(--outline-variant); border-radius:4px; color:var(--on-surface); font-size:11px; }
.ws-btn { padding:4px 10px; background:var(--primary-container); color:var(--on-primary-container); border:none; border-radius:4px; font-size:10px; font-weight:700; cursor:pointer; }
.ws-btn.connected { background:var(--tertiary-container); color:var(--on-tertiary-container); }
.ws-dot { width:8px; height:8px; border-radius:50%; background:#666; }
.ws-dot.online { background:#4caf50; }
</style>
