<template>
  <div class="winder-panel">
    <!-- 左侧控制面板 -->
    <div class="left-panel">
      <!-- WebSocket 连接栏 (winder scripts/ws.py) -->
      <div class="ws-bar">
        <input v-model="wsUrl" placeholder="ws://localhost:8765 (winder ws.py)" class="ws-input" />
        <button @click="wsConnected ? disconnectWS() : connectWS()"
          :class="wsConnected ? 'ws-btn connected' : 'ws-btn'">
          {{ wsConnected ? '已连接' : '连接' }}
        </button>
        <span class="ws-dot" :class="{ online: wsConnected }"></span>
      </div>
      <div class="hint" v-if="wsConnected">流式接收电机位置 (ws.py @60fps)</div>

      <!-- 绕线参数 -->
      <div class="panel-section">
        <h3>绕线参数 (winding)</h3>
        <div class="param-row">
          <label>绕组方案</label>
          <select v-model="presetKey" class="sel small" @change="applyPreset">
            <option v-for="(p, k) in WINDING_PRESETS" :key="k" :value="k">{{ p.label }}</option>
            <option value="">自定义</option>
          </select>
        </div>
        <div class="param-row">
          <label>绕组配置</label>
          <input v-model="params.winding.winding_config" type="text" class="num" />
        </div>
        <div class="param-row">
          <label>匝数 turns</label>
          <input v-model.number="params.winding.turns" type="number" min="1" class="num" />
        </div>
        <div class="param-row">
          <label>起始齿 starts_at</label>
          <input v-model.number="params.winding.starts_at" type="number" min="0" class="num" />
        </div>
        <div class="param-row">
          <label>固定 M3 张紧</label>
          <input v-model="params.winding.dont_move_m3" type="checkbox" class="cb" />
        </div>
        <div class="hint" :class="{ error: configError }">
          槽数 {{ teethCount }}{{ configError ? ' · ' + configError : '' }}
        </div>
      </div>

      <div class="panel-section" v-for="mi in 4" :key="'m' + (mi - 1)">
        <div class="motor-title">
          <span class="motor-badge">M{{ mi - 1 }}</span>
          {{ motorNames[mi - 1] }}
          <label class="dir"><input type="checkbox" v-model="(params.motors as any)[mi - 1].direction" /> 反向</label>
        </div>
        <div v-for="f in motorFields[mi - 1]" :key="f.key" class="param-row">
          <label>{{ f.label }}</label>
          <input type="number" v-model.number="(params.motors as any)[mi - 1][f.key]" :step="f.step ?? 0.5" class="num" />
        </div>
        <div class="param-row">
          <label>速度 velocity</label>
          <input type="number" v-model.number="(params.motors as any)[mi - 1].velocity" step="0.5" class="num" />
        </div>
      </div>

      <!-- 操作 -->
      <div class="panel-section">
        <h3>仿真控制</h3>
        <div class="param-row">
          <label>速度倍率</label>
          <input type="range" v-model.number="simSpeed" min="1" max="60" step="1" class="jog" />
          <span class="val">{{ simSpeed }}x</span>
        </div>
        <div class="btn-row">
          <button @click="engine.initPosition()" class="secondary" :disabled="engine.running">初始化</button>
          <button @click="toggleRun" :class="engine.running ? 'danger' : 'primary'">
            {{ engine.running ? '暂停' : '开始绕线' }}
          </button>
        </div>
        <div class="btn-row">
          <button @click="engine.estop()" class="danger-outline">急停</button>
          <button @click="resetAll" class="secondary">复位</button>
        </div>
        <div class="hint" :class="{ ok: engine.phase === 'done' }">{{ engine.message }}</div>
      </div>
    </div>

    <!-- 中央 3D 视口 -->
    <div ref="viewportRef" class="viewport"></div>

    <!-- 右侧状态面板 -->
    <div class="right-panel">
      <div class="panel-section">
        <h3>运行状态</h3>
        <div class="info-row"><span>阶段:</span><span>{{ phaseText }}</span></div>
        <div class="info-row"><span>当前线:</span><span>{{ engine.currentWire >= 0 ? 'ABC'[engine.currentWire] : '—' }}</span></div>
        <div class="info-row"><span>齿序号:</span><span>{{ engine.currentTeethIdx }} / {{ teethCount - 1 }}</span></div>
        <div class="info-row"><span>当前槽:</span><span>#{{ currentTeeth }}</span></div>
        <div class="info-row"><span>已完成匝数:</span><span>{{ engine.turnsDone }} / {{ params.winding.turns }}</span></div>
        <div class="info-row"><span>绕线时间:</span><span>{{ engine.elapsed.toFixed(1) }} s</span></div>
      </div>

      <div class="panel-section">
        <h3>电机位置</h3>
        <div class="info-row"><span>M0 滑动:</span><span>{{ engine.positions[0].toFixed(2) }}</span></div>
        <div class="info-row"><span>M1 定子:</span><span>{{ (engine.positions[1] * 180 / Math.PI).toFixed(1) }}°</span></div>
        <div class="info-row"><span>M2 绕线:</span><span>{{ (engine.positions[2] * 180 / Math.PI).toFixed(1) }}°</span></div>
        <div class="info-row"><span>M3 张紧:</span><span>{{ engine.positions[3].toFixed(3) }}</span></div>
      </div>

      <div class="panel-section">
        <h3>线包进度</h3>
        <div v-for="i in 3" :key="i" class="limit-row">
          <span class="wire-dot" :style="{ background: WIRE_COLORS[i - 1] }"></span>
          <span>{{ 'ABC'[i - 1] }} 相:</span>
          <span>{{ wirePointCount(i - 1) }} 点</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch, onMounted, onUnmounted } from "vue";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader";
import { WinderEngine, DEFAULT_PARAMS, WINDING_PRESETS, type WinderParams } from "./winderEngine";

const emit = defineEmits<{ connect: []; disconnect: [] }>();

const WIRE_COLORS = [0xff8787, 0x69db7c, 0x74c0fc];
const SCALE = 0.02; // godot 单位 -> 米
const ZSCALE = 0.02; // M0 电机单位 -> 米
const STATOR_Z_BASE = -0.3;
const ARM_Y = 1.8;

/* ======= 参数 ======= */
const params = reactive<WinderParams>(JSON.parse(JSON.stringify(DEFAULT_PARAMS)));
const presetKey = ref("24n22p");
const configError = computed(() => {
  const c = params.winding.winding_config.trim();
  if (!c.length) return "配置为空";
  if (c.length % 3 !== 0) return "长度需为 3 的倍数";
  return "";
});
const teethCount = computed(() => params.winding.winding_config.trim().length);

const motorNames = ["定子滑动 M0", "定子旋转 M1", "绕线臂 M2", "张紧 M3"];
const motorFields: { key: string; label: string; step?: number }[][] = [
  [
    { key: "wind_range_start", label: "绕线起点" },
    { key: "wind_range_end", label: "绕线终点" },
    { key: "end_to_zero", label: "回零余量" },
  ],
  [
    { key: "zero", label: "零点 zero" },
    { key: "end_to_rotating_position", label: "旋转位余量" },
  ],
  [
    { key: "zero", label: "零点 zero" },
    { key: "angle_to_prevent_collision", label: "防碰角" },
  ],
  [
    { key: "pull_wire_torque", label: "拉线扭矩" },
    { key: "wind_torque", label: "绕线扭矩" },
  ],
];

function applyPreset() {
  const p = WINDING_PRESETS[presetKey.value];
  if (p) params.winding.winding_config = p.config;
}

const engine = new WinderEngine(DEFAULT_PARAMS);
const simSpeed = ref(20);

function applyParams() {
  engine.setParams(params as unknown as WinderParams);
}

watch(params, applyParams, { deep: true });
watch(presetKey, applyPreset);

/* ======= Three.js ======= */
const viewportRef = ref<HTMLDivElement>();
let scene: THREE.Scene, cam: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer;
let controls: OrbitControls;
let animId = 0;
let modelsReady = false;

let statorGroup = new THREE.Group();
let armGroup = new THREE.Group();
let toothMarker: THREE.Mesh;
const wireMeshes: THREE.Line[] = [];

const phaseText = computed(() => {
  const m: Record<string, string> = {
    idle: "待机", init: "初始化位置", wind: "绕线中", around: "换轴", done: "完成", error: "错误",
  };
  return m[engine.phase] ?? engine.phase;
});

const currentTeeth = computed(() => (modelsReady ? engine.getCurrentTeeth() : 0));

/* ======= 线包可视化 ======= */
let currentPts: THREE.Vector3[] = [];
let lastWire = -1;
let prevM2: number | null = null;
const wireBuffers: THREE.Vector3[][] = [[], [], []];

function wirePointCount(w: number) {
  return wireBuffers[w].length;
}

function commitWire(w: number) {
  prevM2 = null;
  if (w < 0 || currentPts.length < 2) {
    currentPts = [];
    return;
  }
  wireBuffers[w].push(...currentPts);
  currentPts = [];
  if (wireBuffers[w].length > 60000) {
    wireBuffers[w].splice(0, wireBuffers[w].length - 60000);
  }
  if (wireMeshes[w]) {
    const geo = wireMeshes[w].geometry as THREE.BufferGeometry;
    const arr = new Float32Array(wireBuffers[w].length * 3);
    for (let i = 0; i < wireBuffers[w].length; i++) {
      arr[i * 3] = wireBuffers[w][i].x;
      arr[i * 3 + 1] = wireBuffers[w][i].y;
      arr[i * 3 + 2] = wireBuffers[w][i].z;
    }
    geo.setAttribute("position", new THREE.BufferAttribute(arr, 3));
    geo.computeBoundingSphere();
    geo.attributes.position.needsUpdate = true;
  }
}

const WIRE_R = 0.45;
const WIRE_STEP = 0.4;
function accumulateWire() {
  const wire = engine.currentWire;
  if (wire !== lastWire) {
    commitWire(lastWire);
    lastWire = wire;
  }
  if (wire < 0) return;
  const M2 = engine.positions[2];
  const z = statorGroup.position.z;
  if (prevM2 !== null && Math.abs(M2 - prevM2) > 1e-6) {
    const a0 = prevM2;
    const a1 = M2;
    const dir = a1 >= a0 ? 1 : -1;
    for (let a = a0; (dir > 0 ? a < a1 : a > a1); a += dir * WIRE_STEP) {
      currentPts.push(new THREE.Vector3(Math.sin(a) * WIRE_R, ARM_Y + Math.cos(a) * WIRE_R, z));
    }
  }
  currentPts.push(new THREE.Vector3(Math.sin(M2) * WIRE_R, ARM_Y + Math.cos(M2) * WIRE_R, z));
  prevM2 = M2;
}

/* ======= 场景 ======= */
function buildScene() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x16181d);

  const container = viewportRef.value!;
  const w = container.clientWidth;
  const h = container.clientHeight;

  cam = new THREE.PerspectiveCamera(45, w / h, 0.01, 200);
  cam.position.set(3.4, 3.6, 3.4);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  container.appendChild(renderer.domElement);

  controls = new OrbitControls(cam, renderer.domElement);
  controls.target.set(0, ARM_Y, 0);
  controls.update();

  const amb = new THREE.AmbientLight(0x8090b0, 1.0);
  scene.add(amb);
  const dir = new THREE.DirectionalLight(0xffffff, 1.6);
  dir.position.set(3, 5, 3);
  dir.castShadow = true;
  scene.add(dir);
  const fill = new THREE.DirectionalLight(0x88aaff, 0.5);
  fill.position.set(-2, 1.5, -2);
  scene.add(fill);

  const grid = new THREE.GridHelper(10, 20, 0x334455, 0x223344);
  scene.add(grid);

  const ground = new THREE.Mesh(
    new THREE.PlaneGeometry(12, 12),
    new THREE.MeshStandardMaterial({ color: 0x1c1f26, roughness: 1 })
  );
  ground.rotation.x = -Math.PI / 2;
  ground.position.y = 0;
  ground.receiveShadow = true;
  scene.add(ground);

  const axes = new THREE.AxesHelper(0.6);
  scene.add(axes);

  // 底座平台 + 立柱
  const plate = new THREE.Mesh(
    new THREE.BoxGeometry(2.2, 0.08, 1.6),
    new THREE.MeshStandardMaterial({ color: 0x2a3040, roughness: 0.6, metalness: 0.3 })
  );
  plate.position.y = 0.04;
  plate.castShadow = plate.receiveShadow = true;
  scene.add(plate);

  const column = new THREE.Mesh(
    new THREE.CylinderGeometry(0.05, 0.05, ARM_Y, 12),
    new THREE.MeshStandardMaterial({ color: 0x3a4a62, roughness: 0.5, metalness: 0.5 })
  );
  column.position.set(0, ARM_Y / 2, STATOR_Z_BASE);
  column.castShadow = true;
  scene.add(column);

  const hub = new THREE.Mesh(
    new THREE.CylinderGeometry(0.09, 0.09, 0.14, 16),
    new THREE.MeshStandardMaterial({ color: 0x596580, roughness: 0.4, metalness: 0.6 })
  );
  hub.rotation.z = Math.PI / 2;
  hub.position.set(0, ARM_Y, STATOR_Z_BASE);
  scene.add(hub);

  // 定子 (旋转 M1 + 滑动 M0)
  statorGroup.position.set(0, ARM_Y - 0.278, STATOR_Z_BASE);
  scene.add(statorGroup);

  // 齿槽高亮
  toothMarker = new THREE.Mesh(
    new THREE.BoxGeometry(0.12, 0.12, 0.12),
    new THREE.MeshStandardMaterial({ color: 0xffd43b, emissive: 0xff9800, emissiveIntensity: 0.8 })
  );
  toothMarker.visible = false;
  statorGroup.add(toothMarker);

  // 绕线臂 (旋转 M2)
  armGroup.position.set(0, ARM_Y, STATOR_Z_BASE);
  scene.add(armGroup);

  // 线包 (A/B/C)
  for (let i = 0; i < 3; i++) {
    const line = new THREE.Line(
      new THREE.BufferGeometry(),
      new THREE.LineBasicMaterial({ color: WIRE_COLORS[i], transparent: true, opacity: 0.95 })
    );
    line.frustumCulled = false;
    scene.add(line);
    wireMeshes.push(line);
  }

  loadModels();
  animate();
}

async function loadModels() {
  try {
    const loader = new GLTFLoader();
    const [armRes, statorRes] = await Promise.all([
      loader.loadAsync("/models/winder/arm.glb"),
      loader.loadAsync("/models/winder/stator.glb"),
    ]);
    armRes.scene.traverse((o) => {
      if ((o as THREE.Mesh).isMesh) { o.castShadow = true; o.receiveShadow = true; }
    });
    statorRes.scene.traverse((o) => {
      if ((o as THREE.Mesh).isMesh) { o.castShadow = true; o.receiveShadow = true; }
    });
    armRes.scene.scale.setScalar(SCALE);
    statorRes.scene.scale.setScalar(SCALE);
    armGroup.add(armRes.scene);
    statorGroup.add(statorRes.scene);
    modelsReady = true;
  } catch (e) {
    console.error("加载绕线机模型失败:", e);
  }
}

/* ======= 动画循环 ======= */
let last = performance.now();

function applyTransforms() {
  const M0 = engine.positions[0];
  const M1 = engine.positions[1];
  const M2 = engine.positions[2];
  statorGroup.position.z = STATOR_Z_BASE - M0 * ZSCALE;
  statorGroup.rotation.y = -M1 + Math.PI / 2;
  armGroup.rotation.z = M2;

  const n = teethCount.value || 1;
  const th = (Math.PI * 2) / n * engine.currentTeethIdx;
  const R = 0.45;
  toothMarker.position.set(Math.sin(th) * R, 0.278, Math.cos(th) * R);
  toothMarker.visible = engine.phase === "wind";
}

function animate() {
  animId = requestAnimationFrame(animate);
  const now = performance.now();
  const realDt = Math.min(0.1, (now - last) / 1000);
  last = now;

  if (engine.running) {
    engine.step(realDt * simSpeed.value);
    accumulateWire();
  } else if (engine.phase === "wind") {
    accumulateWire();
  }
  if (engine.phase !== "wind" && engine.phase !== "init") {
    commitWire(lastWire);
    lastWire = -1;
  }

  applyTransforms();
  controls.update();
  renderer.render(scene, cam);
}

/* ======= 控制 ======= */
function toggleRun() {
  if (engine.running) {
    engine.running = false;
    engine.message = "已暂停";
    emit("disconnect");
  } else {
    if (engine.phase === "done") engine.reset();
    if (engine.phase !== "wind") engine.startContinuous();
    engine.running = true;
    engine.message = "绕线中";
    emit("connect");
  }
}

function resetAll() {
  engine.reset();
  for (let i = 0; i < 3; i++) {
    wireBuffers[i] = [];
    const geo = wireMeshes[i].geometry as THREE.BufferGeometry;
    geo.setAttribute("position", new THREE.BufferAttribute(new Float32Array(0), 3));
  }
  currentPts = [];
  lastWire = -1;
  prevM2 = null;
}

/* ======= WS (winder scripts/ws.py) ======= */
const wsUrl = ref("ws://localhost:8765");
const wsConnected = ref(false);
let ws: WebSocket | null = null;

function connectWS() {
  if (!wsUrl.value) return;
  ws = new WebSocket(wsUrl.value);
  ws.onopen = () => { wsConnected.value = true; emit("connect"); };
  ws.onclose = () => { wsConnected.value = false; emit("disconnect"); };
  ws.onerror = () => { wsConnected.value = false; };
  ws.onmessage = (ev) => {
    if (engine.running) return;
    try {
      const d = JSON.parse(ev.data as string);
      const pos: [number, number, number, number] = [
        Number(d.M0) || 0, Number(d.M1) || 0, Number(d.M2) || 0, Number(d.M3) || 0,
      ];
      engine.syncPositions(pos);
    } catch { /* ignore */ }
  };
}

function disconnectWS() {
  ws?.close();
  ws = null;
  wsConnected.value = false;
}

/* ======= 生命周期 ======= */
function onResize() {
  if (!renderer || !viewportRef.value) return;
  const w = viewportRef.value.clientWidth;
  const h = viewportRef.value.clientHeight;
  cam.aspect = w / h;
  cam.updateProjectionMatrix();
  renderer.setSize(w, h);
}

onMounted(() => {
  buildScene();
  window.addEventListener("resize", onResize);
  (window as any).__winderEngine = engine;
  (window as any).__winderStats = () => ({
    statorZ: +statorGroup.position.z.toFixed(3),
    statorRotY: +statorGroup.rotation.y.toFixed(3),
    armRotZ: +armGroup.rotation.z.toFixed(3),
    wireCounts: wireBuffers.map((b) => b.length),
  });
});

onUnmounted(() => {
  window.removeEventListener("resize", onResize);
  cancelAnimationFrame(animId);
  disconnectWS();
  if (renderer && viewportRef.value) {
    viewportRef.value.removeChild(renderer.domElement);
    renderer.dispose();
  }
});
</script>

<style scoped>
.winder-panel {
  display: flex;
  height: 100%;
  width: 100%;
  background: var(--background);
}

.left-panel, .right-panel {
  width: 300px;
  min-width: 300px;
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

.motor-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  font-weight: 600;
  margin-bottom: 6px;
}

.motor-badge {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-radius: 4px;
  padding: 1px 6px;
  font-size: 11px;
  font-weight: 700;
}

.motor-title .dir {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  font-weight: 400;
  color: var(--on-surface-variant);
  cursor: pointer;
}

.param-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}

.param-row label {
  width: 110px;
  font-size: 11px;
  color: var(--on-surface-variant);
  flex-shrink: 0;
}

.param-row .num {
  flex: 1;
  min-width: 0;
  padding: 3px 6px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 11px;
  font-family: monospace;
}

.param-row .cb {
  width: 14px;
  height: 14px;
  accent-color: var(--primary);
}

.sel {
  padding: 4px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
}

.sel.small { width: 100%; }

.hint {
  font-size: 11px;
  color: var(--on-surface-variant);
  padding: 4px 2px;
}

.hint.error { color: var(--error); font-weight: 600; }
.hint.ok { color: var(--tertiary); font-weight: 600; }

.btn-row {
  display: flex;
  gap: 4px;
  margin-bottom: 4px;
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
}

.btn-row button:hover { background: var(--surface-hover); }
.btn-row button.primary { background: var(--primary); color: var(--on-primary); border-color: var(--primary); }
.btn-row button.danger { background: var(--error); color: var(--on-error); border-color: var(--error); }
.btn-row button.danger-outline { background: var(--surface-variant); color: var(--error); border-color: var(--error); }
.btn-row button.secondary { background: var(--surface-variant); color: var(--on-surface-variant); }
.btn-row button:disabled { opacity: 0.5; cursor: not-allowed; }

.info-row, .limit-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
  padding: 2px 0;
  font-family: monospace;
}

.limit-row { gap: 8px; }
.limit-row span:first-child { margin-right: auto; }

.wire-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
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
  width: 40px;
  text-align: right;
  font-size: 11px;
  font-family: monospace;
  color: var(--on-surface);
}

.ws-bar { display: flex; align-items: center; gap: 6px; padding: 6px; background: var(--surface-dim); border-radius: var(--radius); margin-bottom: 8px; }
.ws-input { flex:1; padding:4px 8px; background:var(--surface-container); border:1px solid var(--outline-variant); border-radius:4px; color:var(--on-surface); font-size:11px; }
.ws-btn { padding:4px 10px; background:var(--primary-container); color:var(--on-primary-container); border:none; border-radius:4px; font-size:10px; font-weight:700; cursor:pointer; }
.ws-btn.connected { background:var(--tertiary-container); color:var(--on-tertiary-container); }
.ws-dot { width:8px; height:8px; border-radius:50%; background:#666; }
.ws-dot.online { background:#4caf50; }
</style>
