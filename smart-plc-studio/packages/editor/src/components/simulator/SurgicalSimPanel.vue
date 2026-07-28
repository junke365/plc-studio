<template>
  <div class="surgical-panel">
    <!-- 左侧控制面板 -->
    <div class="left-panel">
      <!-- WebSocket 连接栏 -->
      <div class="ws-bar">
        <input v-model="wsUrl" placeholder="ws://localhost:54001" class="ws-input" />
        <button @click="wsConnected ? disconnectWS() : connectWS()"
          :class="wsConnected ? 'ws-btn connected' : 'ws-btn'">
          {{ wsConnected ? '已连接' : '连接' }}
        </button>
        <span class="ws-dot" :class="{ online: wsConnected }"></span>
      </div>
      <div class="panel-section">
        <h3>机器人选择</h3>
        <select v-model="robotType" class="sel">
          <option value="MTM">da Vinci MTM (主手 7-DOF)</option>
          <option value="PSM">da Vinci PSM (从手 7-DOF)</option>
        </select>
      </div>

      <div class="panel-section">
        <h3>关节控制</h3>
        <div v-for="(_, i) in jointAngles" :key="i" class="joint-row">
          <label>J{{ i }}</label>
          <input type="range" v-model.number="jointAngles[i]" :min="-180" :max="180" class="jog" />
          <span class="val">{{ jointAngles[i].toFixed(1) }}°</span>
        </div>
      </div>

      <div class="panel-section">
        <h3>操作模式</h3>
        <div class="btn-row">
          <button :class="{ active: mode === 'manual' }" @click="mode = 'manual'">手动</button>
          <button :class="{ active: mode === 'ik' }" @click="mode = 'ik'">逆解 IK</button>
          <button :class="{ active: mode === 'auto' }" @click="mode = 'auto'">自动</button>
        </div>
      </div>

      <div class="panel-section" v-if="mode === 'ik'">
        <h3>目标 TCP 位置</h3>
        <div class="xyz-row">
          <label>X</label><input type="range" v-model.number="ikTarget.x" :min="-500" :max="500" class="jog" />
          <span class="val">{{ ikTarget.x.toFixed(0) }}</span>
        </div>
        <div class="xyz-row">
          <label>Y</label><input type="range" v-model.number="ikTarget.y" :min="-500" :max="500" class="jog" />
          <span class="val">{{ ikTarget.y.toFixed(0) }}</span>
        </div>
        <div class="xyz-row">
          <label>Z</label><input type="range" v-model.number="ikTarget.z" :min="-500" :max="500" class="jog" />
          <span class="val">{{ ikTarget.z.toFixed(0) }}</span>
        </div>
      </div>

      <div class="panel-section">
        <h3>信息</h3>
        <div class="info-row"><span>TCP X:</span><span>{{ tcpPos.x.toFixed(2) }}</span></div>
        <div class="info-row"><span>TCP Y:</span><span>{{ tcpPos.y.toFixed(2) }}</span></div>
        <div class="info-row"><span>TCP Z:</span><span>{{ tcpPos.z.toFixed(2) }}</span></div>
        <div class="info-row"><span>奇异值:</span><span :class="{ danger: singularityVal < 0.01 }">{{ singularityVal.toFixed(6) }}</span></div>
        <div class="info-row"><span>工作空间:</span><span :class="{ ok: inWorkspace }">{{ inWorkspace ? '有效' : '无效' }}</span></div>
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
        <div class="info-row"><span>步长:</span><span>{{ (dt * 1000).toFixed(1) }} ms</span></div>
        <div class="info-row"><span>仿真时间:</span><span>{{ simTime.toFixed(2) }} s</span></div>
        <div class="info-row"><span>步数:</span><span>{{ stepCount }}</span></div>
      </div>

      <div class="panel-section">
        <h3>关节限位</h3>
        <div v-for="(v, i) in jointAngles" :key="'l'+i" class="limit-row" :class="{ violated: v < -150 || v > 150 }">
          <span>J{{ i }}: {{ v.toFixed(1) }}°</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, watch } from "vue";

const emit = defineEmits<{ connect: []; disconnect: [] }>();

// WebSocket 连接
const wsUrl = ref("ws://localhost:54001")
const wsConnected = ref(false)
let ws: WebSocket | null = null

function connectWS() {
  ws = new WebSocket(wsUrl.value)
  ws.onopen = () => { wsConnected.value = true; console.log("WS 已连接") }
  ws.onclose = () => { wsConnected.value = false; console.log("WS 已断开") }
  ws.onerror = () => { wsConnected.value = false }
  ws.onmessage = (ev) => {
    const buf = ev.data as ArrayBuffer
    const view = new DataView(buf)
    const cmd = view.getUint32(0, true)
    const len = view.getUint32(4, true)
    if (cmd === 0x01 && len > 0) {
      const floats = new Float32Array(buf, 8, Math.floor(len / 4))
      applyState(floats)
    }
  }
}

function disconnectWS() {
  ws?.close()
  ws = null
  wsConnected.value = false
}

function sendCommand(cmd: number, data?: Float32Array) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return
  const len = data ? data.byteLength : 0
  const buf = new ArrayBuffer(8 + len)
  const view = new DataView(buf)
  view.setUint32(0, cmd, true)
  view.setUint32(4, len, true)
  if (data) new Uint8Array(buf, 8).set(new Uint8Array(data.buffer))
  ws.send(buf)
}

function applyState(floats: Float32Array) {
  if (floats.length >= 7) {
    for (let i = 0; i < 7; i++) jointAngles[i] = floats[i] * 180 / Math.PI
    updateScene()
  }
}

function fallbackIK(target: { x: number; y: number; z: number }) {
  const t = new THREE.Vector3(target.x / 1000, target.y / 1000, target.z / 1000)
  const qDeg = jacobianTransposeIK(t)
  for (let i = 0; i < 7; i++) jointAngles[i] = qDeg[i]
  updateScene()
}

async function solveIK(target: { x: number; y: number; z: number }) {
  if (wsConnected.value) {
    const data = new Float32Array([target.x, target.y, target.z])
    sendCommand(0x04, data)
  } else {
    fallbackIK(target)
  }
}

/* ======= Three.js 导入 ======= */
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";

const viewportRef = ref<HTMLDivElement>();
let scene: THREE.Scene, cam: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer;
let controls: OrbitControls;
let animId = 0;
let robotGroup: THREE.Group;

/* ======= 状态 ======= */
const robotType = ref("MTM");
const mode = ref("manual");
const simRunning = ref(false);
const dt = 0.005;
const simTime = ref(0);
const stepCount = ref(0);
const singularityVal = ref(1);
const inWorkspace = ref(true);
const tcpPos = reactive({ x: 0, y: 0, z: 0 });
const jointAngles = reactive([0, 0, 0, 0, 0, 0, 0]);
const ikTarget = reactive({ x: 200, y: 0, z: 100 });

/* ======= DH 参数 (MTM) ======= */
const mtmDH = {
  a: [0, 0, 0, 0, 0, 0, 0],
  alpha: [-Math.PI / 2, Math.PI / 2, -Math.PI / 2, Math.PI / 2, -Math.PI / 2, Math.PI / 2, 0],
  d: [0, 0, 0.3, 0, 0.25, 0, 0.15],
  theta: [0, -Math.PI / 2, 0, 0, 0, 0, 0],
  jointLimits: [[-144, 144], [-90, 90], [-160, 160], [-125, 125], [-160, 160], [-90, 90], [-160, 160]],
};

const psmDH = {
  a: [0, 0, 0, 0, 0, 0, 0],
  alpha: [-Math.PI / 2, Math.PI / 2, 0, -Math.PI / 2, Math.PI / 2, -Math.PI / 2, 0],
  d: [0, 0, 0.4, 0, 0, 0, 0.02],
  theta: [0, -Math.PI / 2, 0, 0, 0, 0, 0],
  jointLimits: [[-90, 90], [-72, 72], [0, 0.4], [-160, 160], [-90, 90], [-90, 90], [0, 0.02]],
};

function getDH() {
  return robotType.value === "MTM" ? mtmDH : psmDH;
}

/* ======= DH 正运动学 ======= */
function dhTransform(a: number, alpha: number, d: number, theta: number): THREE.Matrix4 {
  const ca = Math.cos(alpha), sa = Math.sin(alpha);
  const ct = Math.cos(theta), st = Math.sin(theta);
  const m = new THREE.Matrix4();
  m.set(
    ct, -st * ca, st * sa, a * ct,
    st, ct * ca, -ct * sa, a * st,
    0, sa, ca, d,
    0, 0, 0, 1
  );
  return m;
}

function forwardKinematics(joints: number[]): { frames: THREE.Matrix4[]; pos: THREE.Vector3 } {
  const dh = getDH();
  const frames: THREE.Matrix4[] = [];
  let T = new THREE.Matrix4().identity();
  for (let i = 0; i < 7; i++) {
    const theta = dh.theta[i] + joints[i] * Math.PI / 180;
    const Ai = dhTransform(dh.a[i], dh.alpha[i], dh.d[i], theta);
    T = T.clone().multiply(Ai);
    frames.push(T.clone());
  }
  const pos = new THREE.Vector3().setFromMatrixPosition(T);
  return { frames, pos };
}

/* ======= 构建机器人 3D 模型 ======= */
function buildRobot(joints: number[]): THREE.Group {
  const group = new THREE.Group();
  const { frames, pos } = forwardKinematics(joints);

  // 基座
  const baseGeo = new THREE.CylinderGeometry(0.06, 0.08, 0.05, 16);
  const baseMat = new THREE.MeshStandardMaterial({ color: 0x444466, metalness: 0.7, roughness: 0.3 });
  const base = new THREE.Mesh(baseGeo, baseMat);
  base.position.y = -0.025;
  group.add(base);

  // 每个连杆: 在关节 i-1 和 i 之间绘制圆柱
  for (let i = 0; i < frames.length; i++) {
    const prevPos = i === 0
      ? new THREE.Vector3(0, 0, 0)
      : new THREE.Vector3().setFromMatrixPosition(frames[i - 1]);
    const jointPos = new THREE.Vector3().setFromMatrixPosition(frames[i]);

    const dir = new THREE.Vector3().subVectors(jointPos, prevPos);
    const len = dir.length();
    if (len < 0.001) continue;

    const mid = new THREE.Vector3().addVectors(prevPos, jointPos).multiplyScalar(0.5);
    const hue = 0.6 + i * 0.04;
    const linkMat = new THREE.MeshStandardMaterial({
      color: new THREE.Color().setHSL(hue, 0.6, 0.5),
      metalness: 0.5,
      roughness: 0.4,
    });
    const link = new THREE.Mesh(new THREE.CylinderGeometry(0.015, 0.015, len, 8), linkMat);
    link.position.copy(mid);

    // 对齐到方向
    const up = new THREE.Vector3(0, 1, 0);
    const q = new THREE.Quaternion().setFromUnitVectors(up, dir.clone().normalize());
    link.quaternion.copy(q);
    group.add(link);

    // 关节球
    const jointMat = new THREE.MeshStandardMaterial({ color: 0x888899, metalness: 0.8, roughness: 0.2 });
    const sphere = new THREE.Mesh(new THREE.SphereGeometry(0.02, 12, 12), jointMat);
    sphere.position.copy(jointPos);
    group.add(sphere);
  }

  // TCP 标记
  const tcpMat = new THREE.MeshStandardMaterial({ color: 0xff4444, emissive: 0xff2222, emissiveIntensity: 0.3 });
  const tcp = new THREE.Mesh(new THREE.SphereGeometry(0.025, 12, 12), tcpMat);
  tcp.position.copy(pos);
  group.add(tcp);

  tcpPos.x = pos.x * 1000;
  tcpPos.y = pos.y * 1000;
  tcpPos.z = pos.z * 1000;

  return group;
}

/* ======= 更新场景 ======= */
function updateScene() {
  if (robotGroup) {
    scene.remove(robotGroup);
    robotGroup.traverse((child) => {
      if (child instanceof THREE.Mesh) {
        child.geometry.dispose();
        if (Array.isArray(child.material)) child.material.forEach(m => m.dispose());
        else child.material.dispose();
      }
    });
  }
  robotGroup = buildRobot([...jointAngles]);
  scene.add(robotGroup);
}

/* ======= 逆运动学 (阻尼最小二乘法) ======= */
function jacobianTransposeIK(target: THREE.Vector3): number[] {
  const dh = getDH();
  let q = [...jointAngles].map(v => v * Math.PI / 180);
  const maxIter = 100;
  let lambda = 0.5;

  for (let iter = 0; iter < maxIter; iter++) {
    const { frames, pos } = forwardKinematics(q.map(v => v * 180 / Math.PI));
    const err = new THREE.Vector3().copy(target).sub(pos);
    if (err.length() < 0.0001) break;

    // 计算雅可比
    const J: number[][] = [];
    for (let i = 0; i < 7; i++) {
      const z = new THREE.Vector3().setFromMatrixColumn(frames[i], 2);
      const p = new THREE.Vector3().setFromMatrixPosition(frames[i]);
      const diff = new THREE.Vector3().copy(pos).sub(p);
      const v = new THREE.Vector3().crossVectors(z, diff);
      J.push([v.x, v.y, v.z, z.x, z.y, z.z]);
    }

    // DLS: J^T * (J*J^T + lambda^2*I)^{-1} * err
    const JJt: number[][] = Array.from({ length: 6 }, () => Array(6).fill(0));
    for (let r = 0; r < 6; r++)
      for (let c = 0; c < 6; c++)
        for (let k = 0; k < 7; k++)
          JJt[r][c] += J[k][r] * J[k][c];

    for (let i = 0; i < 6; i++) JJt[i][i] += lambda * lambda;

    // 高斯消元解 JJt * x = [err.xyz, 0,0,0]
    const b = [err.x, err.y, err.z, 0, 0, 0];
    const x = solve6(JJt, b);
    if (!x) { lambda *= 2; continue; }

    const dq = Array(7).fill(0);
    for (let k = 0; k < 7; k++)
      for (let r = 0; r < 6; r++)
        dq[k] += J[k][r] * x[r];

    let dqNorm = Math.sqrt(dq.reduce((s, v) => s + v * v, 0));
    if (dqNorm > 0.3) {
      const scale = 0.3 / dqNorm;
      for (let k = 0; k < 7; k++) dq[k] *= scale;
    }

    for (let k = 0; k < 7; k++) q[k] += dq[k];
    lambda *= 0.95;
  }

  return q.map(v => v * 180 / Math.PI);
}

function solve6(A: number[][], b: number[]): number[] | null {
  const n = 6;
  const a = A.map(row => [...row]);
  const bb = [...b];
  for (let col = 0; col < n; col++) {
    let pivot = col;
    let maxVal = Math.abs(a[col][col]);
    for (let row = col + 1; row < n; row++) {
      const v = Math.abs(a[row][col]);
      if (v > maxVal) { maxVal = v; pivot = row; }
    }
    if (maxVal < 1e-12) return null;
    if (pivot !== col) {
      [a[col], a[pivot]] = [a[pivot], a[col]];
      [bb[col], bb[pivot]] = [bb[pivot], bb[col]];
    }
    const invPivot = 1 / a[col][col];
    for (let c = col; c < n; c++) a[col][c] *= invPivot;
    bb[col] *= invPivot;
    for (let row = col + 1; row < n; row++) {
      const factor = a[row][col];
      if (factor !== 0) {
        for (let c = col; c < n; c++) a[row][c] -= factor * a[col][c];
        bb[row] -= factor * bb[col];
      }
    }
  }
  const x = Array(n).fill(0);
  for (let i = n - 1; i >= 0; i--) {
    x[i] = bb[i];
    for (let j = i + 1; j < n; j++) x[i] -= a[i][j] * x[j];
  }
  return x;
}

/* ======= Three.js 初始化 ======= */
function initThree() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x1a1a2e);

  const container = viewportRef.value!;
  const w = container.clientWidth;
  const h = container.clientHeight;

  cam = new THREE.PerspectiveCamera(45, w / h, 0.01, 100);
  cam.position.set(0.6, 0.4, 0.8);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  container.appendChild(renderer.domElement);

  controls = new OrbitControls(cam, renderer.domElement);
  controls.target.set(0, 0.15, 0);
  controls.update();

  // 灯光
  const amb = new THREE.AmbientLight(0x404060, 0.6);
  scene.add(amb);
  const dir = new THREE.DirectionalLight(0xffffff, 1.2);
  dir.position.set(1, 2, 1);
  dir.castShadow = true;
  scene.add(dir);
  const fill = new THREE.DirectionalLight(0x8888ff, 0.4);
  fill.position.set(-1, 0.5, -1);
  scene.add(fill);

  // 地面网格
  const grid = new THREE.GridHelper(1.5, 20, 0x444466, 0x333355);
  scene.add(grid);

  // 参考轴
  const axes = new THREE.AxesHelper(0.3);
  scene.add(axes);

  updateScene();
  animate();
}

function animate() {
  animId = requestAnimationFrame(animate);
  controls.update();
  renderer.render(scene, cam);
}

/* ======= 控制 ======= */
function resetSim() {
  for (let i = 0; i < 7; i++) jointAngles[i] = 0;
  simTime.value = 0;
  stepCount.value = 0;
  updateScene();
}

function toggleSim() {
  simRunning.value = !simRunning.value;
  if (simRunning.value) emit("connect");
  else emit("disconnect");
}

/* ======= 自动模式 ======= */
let autoTimer = 0;
function autoStep() {
  if (!simRunning.value) return;
  autoTimer += dt;
  const t = autoTimer;

  // 正弦轨迹
  const r = 0.15;
  const cx = 0.25, cy = 0.15, cz = 0.1;
  const target = new THREE.Vector3(
    cx + r * Math.sin(t * 0.5),
    cy + r * Math.cos(t * 0.7),
    cz + r * Math.sin(t * 0.3 + 1)
  );

  // 自动目标位置显示
  ikTarget.x = target.x * 1000;
  ikTarget.y = target.y * 1000;
  ikTarget.z = target.z * 1000;

  if (mode.value === "ik" || mode.value === "auto") {
    const qDeg = jacobianTransposeIK(target);
    for (let i = 0; i < 7; i++) jointAngles[i] = qDeg[i];
    updateScene();
  }

  simTime.value += dt;
  stepCount.value++;
}

/* ======= 监听 ======= */
watch(robotType, () => {
  for (let i = 0; i < 7; i++) jointAngles[i] = 0;
  updateScene();
});

watch(jointAngles, () => { if (mode.value === "manual") updateScene(); }, { deep: true });

watch(mode, (val) => {
  if (val === "ik") {
    solveIK({ x: ikTarget.x, y: ikTarget.y, z: ikTarget.z })
  }
});

watch([() => ikTarget.x, () => ikTarget.y, () => ikTarget.z], () => {
  if (mode.value === "ik") {
    solveIK({ x: ikTarget.x, y: ikTarget.y, z: ikTarget.z })
  }
});

/* ======= 生命周期 ======= */
onMounted(() => {
  initThree();
  setInterval(autoStep, dt * 1000);
});

onUnmounted(() => {
  cancelAnimationFrame(animId);
  if (renderer && viewportRef.value) {
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

.joint-row label, .xyz-row label {
  width: 24px;
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
  width: 52px;
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
