<template>
  <div class="ros2-panel">
    <!-- 左侧控制面板 -->
    <div class="left-panel">
      <!-- WebSocket 连接栏 -->
      <div class="ws-bar">
        <input v-model="wsUrl" placeholder="ws://localhost:9090/ws (rosbridge_suite)" class="ws-input" />
        <button @click="wsConnected ? disconnectWS() : connectWS()"
          :class="wsConnected ? 'ws-btn connected' : 'ws-btn'">
          {{ wsConnected ? '已连接' : '连接' }}
        </button>
        <span class="ws-dot" :class="{ online: wsConnected }"></span>
      </div>
      <div class="hint" v-if="wsConnected">订阅 /joint_states 实时驱动模型</div>

      <div class="panel-section">
        <h3>模型来源</h3>
        <div class="btn-row">
          <button :class="{ active: modelSource === 'urdf' }" @click="switchSource('urdf')">URDF 机器人</button>
          <button :class="{ active: modelSource === 'gazebo' }" @click="switchSource('gazebo')">Gazebo PX4</button>
        </div>
      </div>

      <div class="panel-section">
        <h3>机器人选择</h3>
        <select v-if="modelSource === 'urdf'" v-model="robotKey" class="sel" :disabled="loading" @change="onRobotChange">
          <optgroup label="内置机器人">
            <option v-for="(cfg, key) in BUILTIN" :key="'b' + key" :value="'b:' + key">{{ cfg.name }}</option>
          </optgroup>
          <optgroup v-for="repo in catalogGroups" :key="repo" :label="'MoveIt 库 · ' + repo">
            <option v-for="item in catalogByRepo[repo]" :key="item.url" :value="'m:' + item.url">
              {{ item.repo }}/{{ item.pkg }}/{{ item.name }}
            </option>
          </optgroup>
        </select>
        <select v-else v-model="gazeboKey" class="sel" :disabled="loading" @change="onGazeboChange">
          <optgroup v-for="(group, gk) in gazeboGroups" :key="gk" :label="group.label">
            <option v-for="m in group.items" :key="m.id" :value="m.id">{{ m.name }}</option>
          </optgroup>
        </select>
        <div class="btn-row small">
          <button @click="modelSource === 'urdf' ? refreshCatalog() : refreshGazeboCatalog()" class="secondary">
            <span class="material-symbols-outlined">refresh</span>刷新{{ modelSource === 'urdf' ? ' MoveIt 库' : ' Gazebo 库' }}
          </button>
        </div>
        <div class="hint" v-if="loading">正在加载模型网格 ({{ robotKey.startsWith('b:loong') ? 'AzureLoong STL 约 32MB' : '' }})...</div>
        <div class="hint" :class="{ error: loadError }" v-else-if="loadError">{{ loadError }}</div>
        <div class="hint" v-else>{{ robotMeta }}</div>
      </div>

      <div class="panel-section" v-if="modelSource === 'gazebo'">
        <h3>环境 (world)</h3>
        <select v-model="worldKey" class="sel" @change="applyWorld">
          <option value="">无 (默认网格)</option>
          <option v-for="w in gazeboWorlds" :key="w.id" :value="w.id">{{ w.name }}</option>
        </select>
        <div class="hint" v-if="worldKey && activeWorld">环境着色: {{ activeWorld.desc }}</div>
      </div>

      <div class="panel-section">
        <h3>关节控制 ({{ modelJoints.length }})</h3>
        <div v-if="!modelJoints.length" class="hint">无可动关节或模型未加载</div>
        <div v-for="j in modelJoints" :key="j.name" class="joint-row">
          <label :title="j.name">{{ shortName(j.name) }}</label>
          <input type="range" v-model.number="j.value" :min="j.min" :max="j.max" class="jog"
            :disabled="!simRunning && mode === 'auto'" />
          <span class="val">{{ j.isPrismatic ? j.value.toFixed(1) + 'mm' : j.value.toFixed(1) + '°' }}</span>
        </div>
      </div>

      <div class="panel-section">
        <h3>操作模式</h3>
        <div class="btn-row">
          <button :class="{ active: mode === 'manual' }" @click="mode = 'manual'">手动</button>
          <button :class="{ active: mode === 'auto' }" @click="mode = 'auto'">自动动画</button>
        </div>
        <div class="hint" v-if="mode === 'auto'">{{ animDesc }}</div>
      </div>

      <div class="panel-section">
        <h3>基座高度 (m)</h3>
        <div class="joint-row">
          <input type="range" v-model.number="baseHeight" min="-1" max="2" step="0.01" class="jog" @input="applyBaseHeight" />
          <span class="val">{{ baseHeight.toFixed(2) }}</span>
        </div>
        <button class="micro-btn" @click="resetBaseHeight">恢复默认高度</button>
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
        <div class="info-row"><span>连杆:</span><span>{{ linkCount }} 个</span></div>
        <div class="info-row"><span>关节:</span><span>{{ jointCount }} 个</span></div>
        <div class="info-row"><span>仿真时间:</span><span>{{ simTime.toFixed(2) }} s</span></div>
      </div>

      <div class="panel-section">
        <h3>关节当前值</h3>
        <div v-for="j in modelJoints" :key="'c' + j.name" class="limit-row">
          <span>{{ shortName(j.name) }}:</span>
          <span>{{ j.isPrismatic ? j.value.toFixed(1) + 'mm' : j.value.toFixed(1) + '°' }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, watch, computed } from "vue";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { UrdfRobot } from "./UrdfRobot";
import { GazeboModel } from "./GazeboModel";

const emit = defineEmits<{ connect: []; disconnect: [] }>();

/* ======= 内置机器人注册表 ======= */
const BUILTIN: Record<string, { name: string; urdf: string; meshBase: string; baseHeight: number; kind: string }> = {
  dog: {
    name: "Unitree Go1 机械狗",
    urdf: "/models/ros2/unitree_go1.urdf",
    meshBase: "/models/ros2",
    baseHeight: 0.375,
    kind: "dog",
  },
  loong: {
    name: "AzureLoong 人形机器人",
    urdf: "/models/ros2/azureloong.urdf",
    meshBase: "/models/ros2",
    baseHeight: 1.14,
    kind: "humanoid",
  },
};

interface CatalogItem {
  url: string;
  name: string;
  pkg: string;
  repo: string;
  pkgDir: string;
}

interface GazeboModelItem {
  id: string;
  name: string;
  cat: string;
  desc: string;
}

interface GazeboWorldItem {
  id: string;
  dir: string;
  name: string;
  desc: string;
  sky: string;
  ground: string;
}

const CAT_LABELS: Record<string, string> = {
  multirotor: "无人机 · 多旋翼",
  fixedwing: "无人机 · 固定翼",
  ground: "无人车",
  water: "水面 / 水下",
};

const gazeboCatalog = ref<{ models: GazeboModelItem[]; worlds: GazeboWorldItem[] }>({ models: [], worlds: [] });
const gazeboKey = ref("x500");
const worldKey = ref("");

const gazeboGroups = computed(() => {
  const order = ["multirotor", "fixedwing", "ground", "water"];
  const groups: { key: string; label: string; items: GazeboModelItem[] }[] = [];
  for (const cat of order) {
    const items = gazeboCatalog.value.models.filter((m) => m.cat === cat);
    if (items.length) groups.push({ key: cat, label: CAT_LABELS[cat] ?? cat, items });
  }
  return groups;
});

const gazeboWorlds = computed(() => gazeboCatalog.value.worlds);
const activeWorld = computed(() => gazeboCatalog.value.worlds.find((w) => w.id === worldKey.value));

const catalog = ref<CatalogItem[]>([]);
const catalogGroups = computed(() => [...new Set(catalog.value.map((i) => i.repo))].sort());
const catalogByRepo = computed(() => {
  const m: Record<string, CatalogItem[]> = {};
  for (const item of catalog.value) (m[item.repo] ||= []).push(item);
  return m;
});

/* ======= Three.js ======= */
const viewportRef = ref<HTMLDivElement>();
let scene: THREE.Scene, cam: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer;
let controls: OrbitControls;
let animId = 0;
let robot: UrdfRobot | null = null;
let gz: GazeboModel | null = null;
const robotsCache = new Map<string, UrdfRobot>();
const meshCount = ref(0);
const linkCount = ref(0);
const jointCount = ref(0);
const loadError = ref("");

/* ======= 状态 ======= */
const modelSource = ref<"urdf" | "gazebo">("gazebo");
const robotKey = ref("b:dog");
const robotKind = ref("dog");
const mode = ref("manual");
const simRunning = ref(false);
const dt = 0.03;
const simTime = ref(0);
const baseHeight = ref(0);
let baseHeightDefault = 0;
const loading = ref(false);
const robotMeta = ref("");

interface ModelJoint {
  name: string;
  value: number;
  min: number;
  max: number;
  isPrismatic: boolean;
}

const modelJoints = reactive<ModelJoint[]>([]);

function shortName(name: string) {
  return name.replace(/^J_/, "").replace(/^([A-Z]+)_/, "$1.");
}

function toSlider(j: { name: string; value: number; limit?: { lower: number; upper: number } }): ModelJoint {
  const isPrismatic = /prismatic|insertion|slide|translational/i.test(j.name);
  const clamp = (v: number, d: number) => (Number.isFinite(v) && Math.abs(v) < 1e6 ? v : d);
  const min = j.limit ? clamp(j.limit.lower, isPrismatic ? -1000 : -180) : (isPrismatic ? -1000 : -180);
  const max = j.limit ? clamp(j.limit.upper, isPrismatic ? 1000 : 180) : (isPrismatic ? 1000 : 180);
  const value = isPrismatic ? j.value * 1000 : j.value * 180 / Math.PI;
  const v = Math.max(min, Math.min(max, value));
  return { name: j.name, value: v, min, max, isPrismatic };
}

function buildJointSliders() {
  modelJoints.splice(0, modelJoints.length);
  if (robot) {
    for (const j of robot.getJointValues()) modelJoints.push(toSlider(j));
  } else if (gz) {
    for (const j of gz.getJointValues()) modelJoints.push(toSlider(j));
  }
}

function applyJointState() {
  const values: Record<string, number> = {};
  for (const j of modelJoints) {
    values[j.name] = j.isPrismatic ? j.value / 1000 : j.value * Math.PI / 180;
  }
  robot?.setJointValues(values);
  gz?.setJointValues(values);
}

function applyBaseHeight() {
  if (robot) robot.root.position.y = baseHeight.value;
  if (gz) gz.root.position.y = baseHeight.value;
}

function resetBaseHeight() {
  baseHeight.value = baseHeightDefault;
  applyBaseHeight();
}

function countStats() {
  let m = 0;
  robot?.root.traverse((child) => { if ((child as THREE.Mesh).isMesh) m++; });
  gz?.root.traverse((child) => { if ((child as THREE.Mesh).isMesh) m++; });
  meshCount.value = m;
  linkCount.value = robot?.links.size ?? gz?.links.size ?? 0;
  jointCount.value = robot?.joints.size ?? gz?.joints.size ?? 0;
}

/* ======= MoveIt 库 ======= */
async function refreshCatalog() {
  try {
    const res = await fetch("/ros-catalog.json?refresh=1");
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    catalog.value = (await res.json()) as CatalogItem[];
  } catch (e) {
    console.warn("刷新 MoveIt 库失败:", e);
  }
}

/* ======= Gazebo 库 ======= */
async function refreshGazeboCatalog() {
  try {
    const res = await fetch("/models/px4/gazebo-catalog.json?t=" + Date.now());
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    gazeboCatalog.value = (await res.json()) as { models: GazeboModelItem[]; worlds: GazeboWorldItem[] };
    const valid = gazeboCatalog.value.models.some((m) => m.id === gazeboKey.value);
    if (!valid) gazeboKey.value = gazeboCatalog.value.models[0]?.id ?? "";
  } catch (e) {
    console.warn("刷新 Gazebo 库失败:", e);
  }
}

function fitCameraToModel(box: THREE.Box3) {
  const size = new THREE.Vector3();
  box.getSize(size);
  const radius = Math.max(size.x, size.y, size.z) * 0.5;
  const center = box.getCenter(new THREE.Vector3());
  controls.target.copy(center);
  const dir = cam.position.clone().sub(controls.target).normalize();
  if (dir.lengthSq() < 0.01) dir.set(1, 0.5, 1);
  cam.position.copy(center).add(dir.multiplyScalar(radius * 2.4 + 0.6));
  cam.near = Math.max(0.01, radius * 0.01);
  cam.far = Math.max(200, radius * 20);
  cam.updateProjectionMatrix();
  controls.update();
}

/* ======= Gazebo 模型装载 ======= */
async function loadGazebo(id: string) {
  loadError.value = "";
  loading.value = true;
  robotMeta.value = "";
  const item = gazeboCatalog.value.models.find((m) => m.id === id);
  if (!item) {
    loadError.value = "目录中找不到该模型";
    loading.value = false;
    return;
  }
  const base = `/models/px4/${item.id}`;
  const g = new GazeboModel(`${base}/model.sdf`, base, item.id);
  // 在加载前设置网格补偿（Y-up 网格在 root 旋转 -PI/2 后需要补偿 +PI/2）
  const cfg = GAZEBO_CAT_CONFIG[item.cat] ?? GAZEBO_CAT_CONFIG.ground;
  if (cfg.meshCompensate) g.meshCompensationAngle = Math.PI / 2;
  try {
    await g.load();
  } catch (e) {
    console.error("加载 Gazebo 模型失败:", e);
    loadError.value = `加载失败: ${(e as Error).message}`;
    loading.value = false;
    return;
  }
  activateGazebo(g, item.cat);
  loading.value = false;
}

// 不同类别的模型朝向策略
// multirotor/fixedwing: mesh 为 Y-up (Blender), root 旋转 -PI/2 后网格也被转，需补偿
// ground/water:       mesh 为 Z-up, root 旋转 -PI/2 后网格正确
const GAZEBO_CAT_CONFIG: Record<string, { rootRotX: number; meshCompensate: boolean }> = {
  multirotor: { rootRotX: -Math.PI / 2, meshCompensate: true },
  fixedwing:  { rootRotX: -Math.PI / 2, meshCompensate: true },
  ground:     { rootRotX: -Math.PI / 2, meshCompensate: false },
  water:      { rootRotX: -Math.PI / 2, meshCompensate: false },
};

function activateGazebo(g: GazeboModel, cat: string = "ground") {
  if (gz) scene.remove(gz.root);
  gz = g;
  scene.add(g.root);
  const cfg = GAZEBO_CAT_CONFIG[cat] ?? GAZEBO_CAT_CONFIG.ground;
  g.root.rotation.x = cfg.rootRotX;
  g.root.position.y = 0;
  buildJointSliders();
  g.applyCurrentState();
  const b = new THREE.Box3();
  g.root.updateMatrixWorld(true);
  b.setFromObject(g.root);
  baseHeightDefault = -b.min.y;
  baseHeight.value = baseHeightDefault;
  g.root.position.y = baseHeight.value;
  g.root.updateMatrixWorld(true);
  b.setFromObject(g.root);
  countStats();
  fitCameraToModel(b);
  robotMeta.value = `Gazebo · ${gazeboCatalog.value.models.find((m) => m.id === g.modelName)?.name ?? g.modelName} · links=${g.links.size} joints=${g.joints.size} · 接地高度=${baseHeight.value.toFixed(2)}m`;
}

function onGazeboChange() {
  loadGazebo(gazeboKey.value);
}

function switchSource(src: "urdf" | "gazebo") {
  if (modelSource.value === src) return;
  modelSource.value = src;
  if (robot) scene.remove(robot.root);
  if (gz) scene.remove(gz.root);
  robot = null;
  gz = null;
  loadError.value = "";
  if (src === "urdf") loadRobot(robotKey.value);
  else loadGazebo(gazeboKey.value);
}

/* ======= 环境 (world) 预设 ======= */
let groundMesh: THREE.Mesh | null = null;
let gridHelper: THREE.GridHelper | null = null;

function applyWorld() {
  const w = activeWorld.value;
  scene.background = new THREE.Color(w ? w.sky : 0x16181d);
  if (!groundMesh) return;
  const groundMat = new THREE.MeshStandardMaterial({
    color: w ? w.ground : 0x1c1f26,
    roughness: 1,
    transparent: true,
    opacity: w ? 1 : 0.6,
  });
  const oldMat = groundMesh.material as THREE.Material;
  oldMat.dispose();
  groundMesh.material = groundMat;
  if (gridHelper) {
    const gridMat = gridHelper.material as THREE.LineBasicMaterial;
    gridMat.color = new THREE.Color(w ? 0x445566 : 0x334455);
    gridMat.opacity = w ? 0.6 : 1;
  }
}

/* ======= 机器人装载 ======= */
async function loadRobot(key: string) {
  loadError.value = "";
  loading.value = true;
  robotMeta.value = "";

  const cached = robotsCache.get(key);
  if (cached) {
    activateRobot(key, cached);
    loading.value = false;
    return;
  }

  let rb: UrdfRobot;
  try {
    if (key.startsWith("b:")) {
      const id = key.slice(2);
      const cfg = BUILTIN[id];
      rb = new UrdfRobot(cfg.urdf, cfg.meshBase);
      robotKind.value = cfg.kind;
      baseHeightDefault = cfg.baseHeight;
    } else {
      const url = key.slice(2);
      const item = catalog.value.find((i) => i.url === url);
      if (!item) throw new Error("目录中找不到该机器人");
      rb = new UrdfRobot(item.url, item.pkgDir, "/ros/moveit_robots");
      robotKind.value = "moveit";
      baseHeightDefault = 0;
    }
    await rb.load();
  } catch (e) {
    console.error("加载机器人失败:", e);
    loadError.value = `加载失败: ${(e as Error).message}`;
    loading.value = false;
    return;
  }

  if (robotsCache.size >= 2) {
    const oldest = robotsCache.keys().next().value as string;
    const old = robotsCache.get(oldest);
    robotsCache.delete(oldest);
    scene.remove(old!.root);
    old!.dispose();
  }
  robotsCache.set(key, rb);
  activateRobot(key, rb);
  loading.value = false;
}

function activateRobot(key: string, rb: UrdfRobot) {
  if (robot) scene.remove(robot.root);
  robot = rb;
  scene.add(rb.root);
  rb.root.rotation.x = -Math.PI / 2;
  rb.root.position.y = 0;
  buildJointSliders();
  if (key.startsWith("b:dog")) setDogStanding();
  else rb.applyCurrentState();
  const b = new THREE.Box3();
  rb.root.updateMatrixWorld(true);
  b.setFromObject(rb.root);
  baseHeightDefault = -b.min.y;
  baseHeight.value = baseHeightDefault;
  rb.root.position.y = baseHeight.value;
  countStats();
  robotMeta.value = `${key.startsWith("b:") ? "内置" : "MoveIt"} · ${rb.urdfUrl.split("/").pop()} · links=${rb.links.size} joints=${rb.joints.size} · 接地高度=${baseHeight.value.toFixed(2)}m`;
}

function onRobotChange() {
  const key = robotKey.value;
  loadRobot(key);
}

/* ======= 动画 ======= */
const animDesc = computed(() => {
  if (modelSource.value === "gazebo") {
    const hasSpinner = modelJoints.some((j) => /rotor|prop|wheel|motor|screw/i.test(j.name));
    return hasSpinner ? "桨叶/车轮/螺旋桨高速旋转, 舵面低频摆动" : "全关节低频正弦摆动";
  }
  if (robotKind.value === "dog") return "四足 trot 小跑步态 (对角支撑)";
  if (robotKind.value === "humanoid") return "人形双臂摆动步态";
  return "全关节低频正弦摆动";
});

let animT = 0;

function setDogStanding() {
  if (!robot) return;
  for (const j of modelJoints) {
    if (j.name.endsWith("_thigh_joint")) j.value = -8;
    else if (j.name.endsWith("_calf_joint")) j.value = 20;
    else j.value = 0;
  }
  applyJointState();
}

function animateRobot() {
  const t = animT;
  if (robotKind.value === "dog") {
    const legs = ["FL", "FR", "RL", "RR"];
    const phase: Record<string, number> = { FL: 0, RR: 0, FR: Math.PI, RL: Math.PI };
    for (const leg of legs) {
      const ph = phase[leg];
      for (const j of modelJoints) {
        if (j.name === `${leg}_thigh_joint`) j.value = 0.6 * Math.sin(t * 2.5 + ph) * 180 / Math.PI;
        else if (j.name === `${leg}_calf_joint`) j.value = (-1.0 * Math.sin(t * 2.5 + ph) + 0.2) * 180 / Math.PI;
        else if (j.name === `${leg}_hip_joint`) j.value = 0.12 * Math.cos(t * 2.5 + ph) * 180 / Math.PI;
      }
    }
  } else if (robotKind.value === "humanoid") {
    const sides = ["l", "r"];
    for (const s of sides) {
      const ph = s === "l" ? 0 : Math.PI;
      const phOpp = s === "l" ? Math.PI : 0;
      for (const j of modelJoints) {
        const n = j.name;
        if (n === `J_hip_${s}_pitch`) j.value = 0.45 * Math.sin(t * 2.0 + ph) * 180 / Math.PI;
        else if (n === `J_knee_${s}_pitch`) j.value = (-0.6 * Math.sin(t * 2.0 + ph) + 0.05) * 180 / Math.PI;
        else if (n === `J_ankle_${s}_pitch`) j.value = 0.08 * Math.sin(t * 2.0 + ph) * 180 / Math.PI;
        else if (n === `J_hip_${s}_roll`) j.value = 0.05 * Math.cos(t * 2.0 + ph) * 180 / Math.PI;
        else if (n === `J_arm_${s}_01` || n === `J_arm_${s}_02`) j.value = 0.3 * Math.sin(t * 2.0 + phOpp) * 180 / Math.PI;
        else if (n === `J_arm_${s}_03` || n === `J_arm_${s}_04`) j.value = 0.15 * Math.sin(t * 2.0 + phOpp) * 180 / Math.PI;
      }
    }
  } else {
    let i = 0;
    for (const j of modelJoints) {
      i++;
      const amp = Math.min(45, Math.abs(j.max - j.min) * 0.15);
      j.value = amp * Math.sin(t * 1.3 + i * 0.7) + (j.min + j.max) / 2;
    }
  }
  applyJointState();
  simTime.value += dt;
}

function animateGazebo() {
  const t = animT;
  let i = 0;
  for (const j of modelJoints) {
    i++;
    const isSpinner = /rotor|prop|wheel|motor|screw/i.test(j.name);
    if (isSpinner) {
      const dir = j.name.includes("_cw") || j.name.includes("left") ? -1 : 1;
      j.value = (((dir * t * 3600) % 360) + 360) % 360;
    } else {
      const amp = Math.min(45, Math.abs(j.max - j.min) * 0.15 || 20);
      j.value = amp * Math.sin(t * 1.3 + i * 0.7) + (j.min + j.max) / 2;
    }
  }
  applyJointState();
  simTime.value += dt;
}

function autoStep() {
  if (!simRunning.value || mode.value !== "auto") return;
  if (!robot && !gz) return;
  animT += dt;
  if (modelSource.value === "gazebo") animateGazebo();
  else animateRobot();
}

/* ======= 控制 ======= */
function resetSim() {
  robot?.reset();
  gz?.reset();
  for (const j of modelJoints) j.value = 0;
  simTime.value = 0;
  animT = 0;
}

function toggleSim() {
  simRunning.value = !simRunning.value;
  if (simRunning.value) emit("connect");
  else emit("disconnect");
}

/* ======= ROS2 桥接 (joint_states 订阅) ======= */
const wsUrl = ref("ws://localhost:9090/ws");
const wsConnected = ref(false);
let ws: WebSocket | null = null;
let wsReady = false;

function connectWS() {
  if (!wsUrl.value) return;
  ws = new WebSocket(wsUrl.value);
  ws.onopen = () => {
    wsConnected.value = true;
    wsReady = true;
    ws!.send(JSON.stringify({ type: "subscribe", topic: "/joint_states" }));
    console.log("ROS2 桥接已连接, 订阅 /joint_states");
  };
  ws.onclose = () => { wsConnected.value = false; wsReady = false; };
  ws.onerror = () => { wsConnected.value = false; };
  ws.onmessage = (ev) => {
    let data: any;
    try { data = JSON.parse(ev.data); } catch { return; }
    if (data.event !== "message") return;
    const payload = data.data?.payload;
    const names: string[] = payload?.name || [];
    const positions: number[] = payload?.position || [];
    if (!names.length || (!robot && !gz)) return;
    for (let i = 0; i < names.length; i++) {
      const slider = modelJoints.find((j) => j.name === names[i]);
      if (!slider) continue;
      const rad = positions[i] ?? 0;
      slider.value = slider.isPrismatic ? rad * 1000 : rad * 180 / Math.PI;
    }
    applyJointState();
  };
}

function disconnectWS() {
  if (wsReady) ws!.send(JSON.stringify({ type: "unsubscribe", topic: "/joint_states" }));
  ws?.close();
  ws = null;
  wsConnected.value = false;
  wsReady = false;
}

/* ======= 场景 ======= */
function initThree() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x16181d);

  const container = viewportRef.value!;
  const w = container.clientWidth;
  const h = container.clientHeight;

  cam = new THREE.PerspectiveCamera(45, w / h, 0.01, 500);
  cam.position.set(2.5, -3.5, 2.2);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  container.appendChild(renderer.domElement);

  controls = new OrbitControls(cam, renderer.domElement);
  controls.target.set(0, 0.6, 0);
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

  gridHelper = new THREE.GridHelper(10, 20, 0x334455, 0x223344);
  gridHelper.position.y = 0;
  scene.add(gridHelper);

  groundMesh = new THREE.Mesh(
    new THREE.PlaneGeometry(10, 10),
    new THREE.MeshStandardMaterial({ color: 0x1c1f26, roughness: 1, transparent: true, opacity: 0.6 })
  );
  groundMesh.rotation.x = -Math.PI / 2;
  groundMesh.position.y = -0.001;
  groundMesh.receiveShadow = true;
  scene.add(groundMesh);

  const axes = new THREE.AxesHelper(0.5);
  scene.add(axes);

  applyWorld();
  refreshGazeboCatalog().then(() => {
    if (modelSource.value === "gazebo") loadGazebo(gazeboKey.value);
  });
  refreshCatalog().then(() => {
    if (modelSource.value === "urdf") loadRobot(robotKey.value);
  });
  animate();
}

function animate() {
  animId = requestAnimationFrame(animate);
  controls.update();
  renderer.render(scene, cam);
}

/* ======= 监听 ======= */
watch(modelJoints, () => { if (mode.value === "manual") applyJointState(); }, { deep: true });

watch(mode, (m) => {
  if (m === "manual") applyJointState();
});

/* ======= 生命周期 ======= */
onMounted(() => {
  initThree();
  setInterval(autoStep, dt * 1000);
});

onUnmounted(() => {
  cancelAnimationFrame(animId);
  disconnectWS();
  if (renderer && viewportRef.value) {
    viewportRef.value.removeChild(renderer.domElement);
    renderer.dispose();
  }
  for (const rb of robotsCache.values()) rb.dispose();
  gz?.dispose();
});
</script>

<style scoped>
.ros2-panel {
  display: flex;
  height: 100%;
  width: 100%;
  background: var(--background);
}

.left-panel, .right-panel {
  width: 280px;
  min-width: 280px;
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

.hint.error { color: var(--error); font-weight: 600; }

.sel {
  width: 100%;
  padding: 6px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  margin-bottom: 6px;
}

.joint-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}

.joint-row label {
  width: 96px;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  flex-shrink: 0;
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
  flex-shrink: 0;
}

.btn-row {
  display: flex;
  gap: 4px;
}

.btn-row.vert { flex-direction: column; }
.btn-row.small button { font-size: 11px; padding: 4px 6px; }

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

.micro-btn {
  width: 100%;
  margin-top: 4px;
  padding: 4px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 11px;
  cursor: pointer;
}

.info-row, .limit-row {
  display: flex;
  justify-content: space-between;
  font-size: 12px;
  padding: 2px 0;
  font-family: monospace;
}

.ws-bar { display: flex; align-items: center; gap: 6px; padding: 6px; background: var(--surface-dim); border-radius: var(--radius); margin-bottom: 8px; }
.ws-input { flex:1; padding:4px 8px; background:var(--surface-container); border:1px solid var(--outline-variant); border-radius:4px; color:var(--on-surface); font-size:11px; }
.ws-btn { padding:4px 10px; background:var(--primary-container); color:var(--on-primary-container); border:none; border-radius:4px; font-size:10px; font-weight:700; cursor:pointer; }
.ws-btn.connected { background:var(--tertiary-container); color:var(--on-tertiary-container); }
.ws-dot { width:8px; height:8px; border-radius:50%; background:#666; }
.ws-dot.online { background:#4caf50; }
</style>
