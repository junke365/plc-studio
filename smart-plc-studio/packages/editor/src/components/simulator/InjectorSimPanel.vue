<template>
  <div class="injector-panel">
    <!-- 左侧控制面板 -->
    <div class="left-panel">
      <!-- 机型选择 -->
      <div class="panel-section">
        <h3>机型</h3>
        <div class="param-row">
          <select v-model="machineType" class="sel small">
            <option value="smart">Smart Injector (螺杆式)</option>
            <option value="mk4">Buster Beagle MK4 (气动式)</option>
          </select>
        </div>
      </div>

      <!-- Smart Injector 参数 -->
      <template v-if="machineType === 'smart'">
        <div class="panel-section">
          <h3>循环参数 (cycle)</h3>
          <div class="param-row">
            <label>注塑转数 rotationsFor</label>
            <input v-model.number="smartParams.cycle.rotationsFor" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>后退转数 rotationsBack</label>
            <input v-model.number="smartParams.cycle.rotationsBack" type="number" min="0" class="num" />
          </div>
          <div class="param-row">
            <label>注塑后延时 delay</label>
            <input v-model.number="smartParams.cycle.delayBetween" type="number" min="0" step="0.1" class="num" />
          </div>
          <div class="param-row">
            <label>开模前等待 wait</label>
            <input v-model.number="smartParams.cycle.waitBeforeOpen" type="number" min="0" step="0.1" class="num" />
          </div>
        </div>

        <div class="panel-section">
          <h3>挤出机 (extruder)</h3>
          <div class="param-row">
            <label>注塑转速 rpm</label>
            <input v-model.number="smartParams.extruder.speedRpm" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>后退转速 rpm</label>
            <input v-model.number="smartParams.extruder.suckbackRpm" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>减速比 gear</label>
            <input v-model.number="smartParams.extruder.gearRatio" type="number" min="1" step="0.5" class="num" />
          </div>
          <div class="param-row">
            <label>每转步数</label>
            <input v-model.number="smartParams.extruder.stepsPerRev" type="number" min="1" class="num" />
          </div>
        </div>

        <div class="panel-section">
          <h3>合模 (clamp)</h3>
          <div class="param-row">
            <label>合模过行程步</label>
            <input v-model.number="smartParams.clamp.closeOverSteps" type="number" min="0" class="num" />
          </div>
          <div class="param-row">
            <label>开模过行程步</label>
            <input v-model.number="smartParams.clamp.openOverSteps" type="number" min="0" class="num" />
          </div>
          <div class="param-row">
            <label>合模速度 steps/s</label>
            <input v-model.number="smartParams.clamp.stepsPerSec" type="number" min="100" class="num" />
          </div>
        </div>

        <div class="panel-section">
          <h3>加热 (heating)</h3>
          <div class="param-row">
            <label>目标温度 °C</label>
            <input v-model.number="smartParams.heating.targetTemp" type="number" min="30" class="num" />
          </div>
          <div class="param-row">
            <label>加热速率 °C/s</label>
            <input v-model.number="smartParams.heating.heatRate" type="number" min="0.5" step="0.5" class="num" />
          </div>
          <div class="param-row">
            <label>最低注塑温 °C</label>
            <input v-model.number="smartParams.heating.minInjectTemp" type="number" min="30" class="num" />
          </div>
        </div>
      </template>

      <!-- MK4 参数 -->
      <template v-else>
        <div class="panel-section">
          <h3>任务参数 (菜单)</h3>
          <div class="param-row">
            <label>注塑时间 injectTime</label>
            <input v-model.number="mk4Params.cycle.injectTime" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>合钳保持 viseHold</label>
            <input v-model.number="mk4Params.cycle.viseHoldTime" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>补料时间 shotSize</label>
            <input v-model.number="mk4Params.cycle.shotSize" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>目标件数 numOfParts</label>
            <input v-model.number="mk4Params.cycle.numOfParts" type="number" min="1" class="num" />
          </div>
          <div class="param-row">
            <label>件间暂停 cyclePause</label>
            <input v-model.number="mk4Params.cycle.cyclePause" type="number" min="0" class="num" />
          </div>
        </div>

        <div class="panel-section">
          <h3>加热 (PID)</h3>
          <div class="param-row">
            <label>目标温度 °C</label>
            <input v-model.number="mk4Params.heating.targetTemp" type="number" min="30" class="num" />
          </div>
          <div class="param-row">
            <label>加热速率 °C/s</label>
            <input v-model.number="mk4Params.heating.heatRate" type="number" min="0.5" step="0.5" class="num" />
          </div>
        </div>

        <div class="panel-section">
          <h3>时序 (timings)</h3>
          <div class="param-row">
            <label>合钳时长 s</label>
            <input v-model.number="mk4Params.timings.viseCloseSec" type="number" min="0.5" step="0.5" class="num" />
          </div>
          <div class="param-row">
            <label>注塑前等待 s</label>
            <input v-model.number="mk4Params.timings.injectWaitSec" type="number" min="0" step="0.5" class="num" />
          </div>
          <div class="param-row">
            <label>掉件检测延时 s</label>
            <input v-model.number="mk4Params.timings.ejectDelaySec" type="number" min="0.2" step="0.2" class="num" />
          </div>
        </div>
      </template>

      <!-- 仿真控制 -->
      <div class="panel-section">
        <h3>仿真控制</h3>
        <div class="param-row">
          <label>速度倍率</label>
          <input type="range" v-model.number="simSpeed" min="1" max="60" step="1" class="jog" />
          <span class="val">{{ simSpeed }}x</span>
        </div>
        <div class="btn-row">
          <button @click="startAction" :class="engineRunning ? 'danger' : 'primary'">
            {{ engineRunning ? '暂停' : (machineType === 'smart' ? '开始循环' : '开始任务') }}
          </button>
          <button @click="toggleHeating" :class="heatingOn ? 'danger-outline' : 'secondary'">
            {{ heatingOn ? '关闭加热' : '开启加热' }}
          </button>
        </div>
        <div class="btn-row">
          <template v-if="machineType === 'smart'">
            <button @click="smart.manualBackward()" class="secondary" :disabled="smart.running">手动后退</button>
          </template>
          <template v-else>
            <button @click="mk4.manualInject()" class="secondary" :disabled="mk4.running">手动注塑</button>
            <button @click="mk4.manualHopper()" class="secondary" :disabled="mk4.running">手动补料</button>
          </template>
          <button @click="resetAll" class="secondary">复位</button>
        </div>
        <div class="hint" :class="{ ok: enginePhase === 'done' }">{{ engine.message }}</div>
      </div>
    </div>

    <!-- 中央 3D 视口 -->
    <div ref="viewportRef" class="viewport"></div>

    <!-- 右侧状态面板 -->
    <div class="right-panel">
      <div class="panel-section">
        <h3>运行状态</h3>
        <div class="info-row"><span>阶段:</span><span>{{ phaseText }}</span></div>
        <div class="info-row"><span>消息:</span><span class="msg">{{ engine.message }}</span></div>
        <div class="info-row"><span>仿真时间:</span><span>{{ engine.elapsed.toFixed(1) }} s</span></div>
        <template v-if="machineType === 'smart'">
          <div class="info-row"><span>完成循环:</span><span>{{ smart.cycleCount }} 次</span></div>
          <div class="info-row"><span>模具温度:</span><span>{{ smart.mouldTemp.toFixed(0) }} °C</span></div>
          <div class="info-row"><span>螺杆温度:</span><span>{{ smart.extruderTemp.toFixed(0) }} °C</span></div>
        </template>
        <template v-else>
          <div class="info-row"><span>剩余件数:</span><span>{{ mk4.partsLeft }} / {{ mk4.totalParts }}</span></div>
          <div class="info-row"><span>阶段剩余:</span><span>{{ mk4.timeLeft.toFixed(1) }} s</span></div>
          <div class="info-row"><span>料筒温度:</span><span>{{ mk4.chamberTemp.toFixed(0) }} °C</span></div>
          <div class="info-row"><span>无检测计数:</span><span>{{ mk4.noDetectionCount }}</span></div>
        </template>
      </div>

      <template v-if="machineType === 'smart'">
        <div class="panel-section">
          <h3>机构位置</h3>
          <div class="info-row"><span>合模位置:</span><span>{{ (smart.clampPos * 100).toFixed(1) }} %</span></div>
          <div class="info-row"><span>合模步数:</span><span>{{ smart.clampSteps.toFixed(0) }} 步</span></div>
          <div class="info-row"><span>螺杆转数:</span><span>{{ smart.extRot.toFixed(1) }}</span></div>
          <div class="info-row"><span>螺杆转速:</span><span>{{ smart.screwRpm.toFixed(1) }} rpm</span></div>
          <div class="info-row"><span>限位1(合模):</span><span :class="{ ok: smart.endstop1 }">{{ smart.endstop1 ? 'ON' : 'OFF' }}</span></div>
          <div class="info-row"><span>限位2(开模):</span><span :class="{ ok: smart.endstop2 }">{{ smart.endstop2 ? 'ON' : 'OFF' }}</span></div>
        </div>
        <div class="panel-section">
          <h3>型腔填充</h3>
          <div class="bar"><div class="bar-fill" :style="{ width: (smart.materialLevel * 100) + '%' }"></div></div>
          <div class="info-row"><span>材料填充:</span><span>{{ (smart.materialLevel * 100).toFixed(0) }} %</span></div>
        </div>
      </template>

      <template v-else>
        <div class="panel-section">
          <h3>机构位置</h3>
          <div class="info-row"><span>钳口开度:</span><span>{{ (mk4.viseOpen * 100).toFixed(0) }} %</span></div>
          <div class="info-row"><span>注塑缸:</span><span>{{ (mk4.injectionRam * 100).toFixed(0) }} %</span></div>
          <div class="info-row"><span>料斗螺旋转速:</span><span>{{ mk4.augerRpm.toFixed(0) }} rpm</span></div>
          <div class="info-row"><span>顶出阀:</span><span :class="{ ok: mk4.pneumaticInjection }">{{ mk4.pneumaticInjection ? 'ON' : 'OFF' }}</span></div>
          <div class="info-row"><span>合钳霍尔:</span><span :class="{ ok: mk4.hallSensorVise }">{{ mk4.hallSensorVise ? '触发' : '—' }}</span></div>
          <div class="info-row"><span>掉件检测:</span><span :class="{ ok: mk4.partDropDetected }">{{ mk4.partDropDetected ? '已检测' : '等待' }}</span></div>
        </div>
        <div class="panel-section">
          <h3>任务进度</h3>
          <div class="bar"><div class="bar-fill" :style="{ width: mk4.totalParts ? (mk4.partsMade / mk4.totalParts * 100) + '%' : '0%' }"></div></div>
          <div class="info-row"><span>已完成:</span><span>{{ mk4.partsMade }} / {{ mk4.totalParts }}</span></div>
        </div>
      </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch, onMounted, onUnmounted } from "vue";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader";
import { InjectorEngine, DEFAULT_PARAMS as SMART_DEFAULT, type InjectorParams } from "./injectorEngine";
import { Mk4Engine, DEFAULT_PARAMS as MK4_DEFAULT, type Mk4Params } from "./mk4Engine";

const emit = defineEmits<{ connect: []; disconnect: [] }>();

const smart = new InjectorEngine(SMART_DEFAULT);
const mk4 = new Mk4Engine(MK4_DEFAULT);

const machineType = ref<"smart" | "mk4">("smart");
const smartParams = reactive<InjectorParams>(JSON.parse(JSON.stringify(SMART_DEFAULT)));
const mk4Params = reactive<Mk4Params>(JSON.parse(JSON.stringify(MK4_DEFAULT)));

watch(smartParams, () => smart.setParams(smartParams as InjectorParams), { deep: true });
watch(mk4Params, () => mk4.setParams(mk4Params as Mk4Params), { deep: true });

const engine = computed(() => (machineType.value === "smart" ? smart : mk4));
const engineRunning = computed(() => engine.value.running);
const enginePhase = computed(() => engine.value.phase);
const heatingOn = computed(() => (machineType.value === "smart" ? smart.heatingOn : mk4.heatingOn));

const simSpeed = ref(15);

function startAction() {
  const e = engine.value;
  if (e.running) {
    e.pause();
    emit("disconnect");
  } else {
    if (e.phase === "done") e.reset();
    if (machineType.value === "smart") smart.startCycle();
    else mk4.startJob();
    emit("connect");
  }
}

function toggleHeating() {
  engine.value.toggleHeating();
}

function resetAll() {
  engine.value.reset();
  wireResetSmart();
  wireResetMk4();
}

const phaseText = computed(() => {
  const m: Record<string, string> = {
    idle: "待机", closing: "合模中", injecting: "注塑中", holding: "保压冷却",
    opening: "开模", backing: "螺杆后退", waiting: "等待", done: "完成",
    menu: "菜单", closing_vise: "合钳", inject_wait: "待注塑", refilling: "补料",
    opening_vise: "开钳", ejecting: "顶出", pausing: "件间暂停",
  };
  return m[engine.value.phase] ?? engine.value.phase;
});

/* ======= Three.js 公共 ======= */
const viewportRef = ref<HTMLDivElement>();
let scene: THREE.Scene, cam: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer;
let controls: OrbitControls;
let animId = 0;
let machineRoot = new THREE.Group();

function mBox(parent: THREE.Object3D, w: number, h: number, d: number, color: number, x: number, y: number, z: number, opts?: { opacity?: number; emissive?: number; emissiveIntensity?: number }) {
  const mat = new THREE.MeshStandardMaterial({ color, roughness: 0.6, metalness: 0.25 });
  if (opts?.opacity != null) { mat.transparent = true; mat.opacity = opts.opacity; }
  if (opts?.emissive != null) mat.emissive = new THREE.Color(opts.emissive);
  mat.emissiveIntensity = opts?.emissiveIntensity ?? 0;
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
  mesh.position.set(x, y, z);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  parent.add(mesh);
  return mesh;
}

function mCyl(parent: THREE.Object3D, rt: number, rb: number, len: number, color: number, x: number, y: number, z: number, opts?: { emissive?: number; emissiveIntensity?: number; rx?: number; rz?: number; rx90?: boolean; opacity?: number }) {
  const mat = new THREE.MeshStandardMaterial({ color, roughness: 0.5, metalness: 0.4 });
  if (opts?.emissive != null) mat.emissive = new THREE.Color(opts.emissive);
  mat.emissiveIntensity = opts?.emissiveIntensity ?? 0;
  if (opts?.opacity != null) { mat.transparent = true; mat.opacity = opts.opacity; }
  const mesh = new THREE.Mesh(new THREE.CylinderGeometry(rt, rb, len, 20), mat);
  mesh.position.set(x, y, z);
  if (opts?.rx90) mesh.rotation.z = Math.PI / 2;
  else { mesh.rotation.x = opts?.rx ?? 0; mesh.rotation.z = opts?.rz ?? 0; }
  mesh.castShadow = true;
  parent.add(mesh);
  return mesh;
}

function mTorus(parent: THREE.Object3D, rt: number, tube: number, color: number, x: number, y: number, z: number) {
  const mat = new THREE.MeshStandardMaterial({ color, roughness: 0.6, metalness: 0.3, emissive: new THREE.Color(0xff6b35), emissiveIntensity: 0 });
  const mesh = new THREE.Mesh(new THREE.TorusGeometry(rt, tube, 8, 24), mat);
  mesh.position.set(x, y, z);
  mesh.rotation.x = Math.PI / 2;
  parent.add(mesh);
  return mesh;
}

function mCone(parent: THREE.Object3D, r: number, len: number, color: number, x: number, y: number, z: number, down = false) {
  const mesh = new THREE.Mesh(new THREE.ConeGeometry(r, len, 18), new THREE.MeshStandardMaterial({ color, roughness: 0.5, metalness: 0.4 }));
  mesh.position.set(x, y, z);
  mesh.rotation.x = down ? Math.PI : 0;
  mesh.castShadow = true;
  parent.add(mesh);
  return mesh;
}

function loadGLB(url: string, parent: THREE.Object3D, onDone?: () => void, opts: { center?: boolean; rx?: number; ry?: number; rz?: number; at?: [number, number, number]; scale?: number } = {}) {
  new GLTFLoader().load(url, (gltf) => {
    const obj = gltf.scene;
    if (opts.center) {
      const box = new THREE.Box3().setFromObject(obj);
      obj.position.sub(box.getCenter(new THREE.Vector3()));
    }
    if (opts.rx) obj.rotation.x = opts.rx;
    if (opts.ry) obj.rotation.y = opts.ry;
    if (opts.rz) obj.rotation.z = opts.rz;
    if (opts.at) obj.position.set(opts.at[0], opts.at[1], opts.at[2]);
    if (opts.scale) obj.scale.setScalar(opts.scale);
    parent.add(obj);
    onDone?.();
  });
}
/* ======= Smart Injector 真实 CAD 装配 ======= */
// 源自 Smart_Injector/03_CAD/*.step，OCP 转 STL → trimesh → GLB。
// 卧式注塑机：CAD 坐标系单位 mm，Z 为机器长度（水平），Y 为高度，X 为宽度。
//   frame x±150 y0..435 z55..1517 | extruder z19..887 | clamp z845..1422 | ventilation z28..1038 | panel z433..823
const SMART_GLB = [
  { name: "frame", url: "models/injector/smart_frame.glb" },
  { name: "extruder", url: "models/injector/smart_extruder.glb" },
  { name: "clamp", url: "models/injector/smart_clamp.glb" },
  { name: "ventilation", url: "models/injector/smart_ventilation.glb" },
  { name: "panel", url: "models/injector/smart_panel.glb" },
];

const SMART_CAD_SCALE = 0.001;
const MOULD_Y = 0.25;
const MOULD_Z = 0.98;

const smartRefs: {
  cadRoot: THREE.Group;
  part: THREE.Mesh;
  partMat: THREE.MeshStandardMaterial;
  stream: THREE.Mesh;
  glowMould: THREE.Mesh;
  glowBarrel: THREE.Mesh;
  loaded: boolean;
} = { cadRoot: new THREE.Group(), part: new THREE.Mesh(), partMat: new THREE.MeshStandardMaterial(), stream: new THREE.Mesh(), glowMould: new THREE.Mesh(), glowBarrel: new THREE.Mesh(), loaded: false };

let smartPartFallT = -1;
let smartPrevPhase = "";

function buildSmartScene() {
  const cad = smartRefs.cadRoot;
  // GLB 已是 Y-up，Z 为机器长度(水平)、Y 为高度，仅需 mm→m 缩放，无需旋转
  cad.rotation.set(0, 0, 0);
  cad.scale.setScalar(SMART_CAD_SCALE);
  scene.add(cad);

  smartRefs.partMat = new THREE.MeshStandardMaterial({ color: 0xffb454, roughness: 0.5, emissive: new THREE.Color(0xcc7722), emissiveIntensity: 0.15 });
  smartRefs.part = new THREE.Mesh(new THREE.BoxGeometry(0.12, 0.05, 0.1), smartRefs.partMat);
  smartRefs.part.position.set(0, MOULD_Y, MOULD_Z);
  smartRefs.part.visible = false;
  scene.add(smartRefs.part);

  smartRefs.stream = mCyl(scene, 0.012, 0.012, 0.08, 0xff8c42, 0, MOULD_Y - 0.05, MOULD_Z, { emissive: 0xff8c42, emissiveIntensity: 0.6 });
  smartRefs.stream.visible = false;

  // 温度发光标记（叠加在真实模型上的非侵入标注）
  smartRefs.glowMould = mTorus(scene, 0.16, 0.006, 0xff6b35, 0, MOULD_Y, MOULD_Z);
  smartRefs.glowBarrel = mTorus(scene, 0.1, 0.006, 0xff6b35, 0, 0.42, MOULD_Z);

  const loader = new GLTFLoader();
  let remaining = SMART_GLB.length;
  for (const p of SMART_GLB) {
    loader.load(p.url, (gltf) => {
      cad.add(gltf.scene);
      remaining -= 1;
      if (remaining === 0) smartRefs.loaded = true;
    });
  }
  smartPartFallT = -1;
}
function animateSmart(dt: number) {
  const e = smart;
  const r = smartRefs;
  if (e.phase === "closing" && smartPrevPhase !== "closing") {
    smartPartFallT = -1;
    r.part.visible = false;
    r.part.scale.setScalar(1);
    r.part.position.set(0, MOULD_Y, MOULD_Z);
  }
  smartPrevPhase = e.phase;

  r.stream.visible = e.phase === "injecting" || e.phase === "holding";

  if (e.materialLevel > 0 && smartPartFallT < 0) {
    r.part.visible = true;
    const s = 0.25 + 0.75 * e.materialLevel;
    r.part.scale.setScalar(s);
    r.part.position.y = MOULD_Y + (s - 1) * 0.01;
  }
  if (e.partEjected && smartPartFallT < 0) smartPartFallT = 0;
  if (smartPartFallT >= 0) {
    smartPartFallT += dt;
    const k = Math.min(1, smartPartFallT / 1.4);
    r.part.position.y = MOULD_Y - 0.34 * k;
    r.part.visible = k < 0.98;
  }

  const glow = Math.min(1, e.mouldTemp / e.params.heating.targetTemp) * 1.4;
  (r.glowMould.material as THREE.MeshStandardMaterial).emissiveIntensity = glow;
  const glowBarrel = Math.min(1, e.extruderTemp / e.params.heating.targetTemp) * 1.4;
  (r.glowBarrel.material as THREE.MeshStandardMaterial).emissiveIntensity = glowBarrel;
}

function wireResetSmart() {
  smartPartFallT = -1;
  if (smartRefs.part) smartRefs.part.visible = false;
  if (smartRefs.stream) smartRefs.stream.visible = false;
}

/* ======= MK4 场景 ======= */
const mk4Refs: {
  jaw: THREE.Group;
  pistonRod: THREE.Mesh;
  ram: THREE.Group;
  auger: THREE.Group;
  part: THREE.Mesh;
  partMat: THREE.MeshStandardMaterial;
  beam: THREE.Mesh;
  beamMat: THREE.MeshStandardMaterial;
  horn: THREE.Mesh;
  bands: THREE.Mesh[];
} = { jaw: new THREE.Group(), pistonRod: new THREE.Mesh(), ram: new THREE.Group(), auger: new THREE.Group(), part: new THREE.Mesh(), partMat: new THREE.MeshStandardMaterial(), beam: new THREE.Mesh(), beamMat: new THREE.MeshStandardMaterial(), horn: new THREE.Mesh(), bands: [] };

const JAW_OPEN_X = -0.3;
const JAW_CLOSED_X = -0.04;
let mk4PartFallT = -1;
let mk4PrevPhase = "";

function buildMk4Scene() {
  const s = scene;
  mk4Refs.jaw = new THREE.Group();
  mk4Refs.ram = new THREE.Group();
  mk4Refs.auger = new THREE.Group();
  mk4Refs.bands = [];

  mBox(s, 1.0, 0.08, 0.6, 0x2b2f36, 0, 0.04, 0);
  for (const sx of [-0.42, 0.42]) for (const sz of [-0.22, 0.22]) mBox(s, 0.05, 1.1, 0.05, 0x32363e, sx, 0.6, sz);
  mBox(s, 1.0, 0.06, 0.5, 0x2b2f36, 0, 1.18, 0);

  // 料筒 (加热腔)
  mCyl(s, 0.055, 0.055, 0.62, 0x8a9098, 0, 0.86, 0);
  mk4Refs.bands.push(mTorus(s, 0.062, 0.012, 0x6b2a1a, 0, 0.7, 0));
  mk4Refs.bands.push(mTorus(s, 0.062, 0.012, 0x6b2a1a, 0, 0.86, 0));
  mk4Refs.bands.push(mTorus(s, 0.062, 0.012, 0x6b2a1a, 0, 1.0, 0));
  mCone(s, 0.05, 0.12, 0x6c7280, 0, 0.5, 0, true);

  // 料斗 + 螺旋给料
  mCone(s, 0.15, 0.2, 0xd9a066, 0, 1.28, 0);
  for (let i = 0; i < 8; i++) {
    const a = (i / 8) * Math.PI * 2;
    mBox(s, 0.035, 0.035, 0.035, 0x8c6a46, Math.cos(a) * 0.09, 1.3, Math.sin(a) * 0.09);
  }
  mBox(s, 0.14, 0.12, 0.12, 0x3a4456, 0.42, 1.06, 0);
  mCyl(s, 0.036, 0.036, 0.38, 0x6c7280, 0.22, 1.06, 0, { rx90: true });
  mk4Refs.auger = new THREE.Group();
  mCyl(mk4Refs.auger, 0.026, 0.026, 0.36, 0xc7ccd4, 0.22, 1.06, 0, { rx90: true });
  mBox(mk4Refs.auger, 0.36, 0.014, 0.04, 0x9aa0a8, 0.22, 1.07, 0);
  s.add(mk4Refs.auger);

  // 气动注塑缸
  mCyl(s, 0.045, 0.045, 0.3, 0x4a5464, 0, 1.42, 0);
  mk4Refs.ram = new THREE.Group();
  mCyl(mk4Refs.ram, 0.02, 0.02, 0.26, 0xc7ccd4, 0, -0.13, 0);
  s.add(mk4Refs.ram);
  mBox(s, 0.18, 0.05, 0.18, 0x39414c, 0, 1.32, 0);

  // 气动虎钳
  mBox(s, 0.42, 0.06, 0.3, 0x3d4450, 0, 0.28, 0);
  mBox(s, 0.14, 0.16, 0.24, 0x4a515c, 0.18, 0.3, 0);
  mBox(s, 0.09, 0.13, 0.16, 0x6d7a90, 0.09, 0.3, 0);
  const jaw = mk4Refs.jaw;
  mBox(jaw, 0.14, 0.16, 0.24, 0x4a515c, 0, 0.02, 0);
  mBox(jaw, 0.09, 0.13, 0.16, 0x6d7a90, 0.065, 0.02, 0);
  s.add(jaw);
  // 虎钳气缸
  mCyl(s, 0.035, 0.035, 0.2, 0x4a5464, -0.5, 0.3, 0, { rx90: true });
  mk4Refs.pistonRod = mBox(s, 0.05, 0.025, 0.025, 0xc7ccd4, -0.4, 0.3, 0);
  for (const sy of [-0.18, 0.18]) mCyl(s, 0.008, 0.008, 0.62, 0x6c7280, 0, 0.34 + sy * 0.5, 0.16);

  // 产品
  mk4Refs.partMat = new THREE.MeshStandardMaterial({ color: 0xffb454, roughness: 0.5, emissive: new THREE.Color(0xcc7722), emissiveIntensity: 0.15 });
  mk4Refs.part = new THREE.Mesh(new THREE.BoxGeometry(0.07, 0.05, 0.08), mk4Refs.partMat);
  mk4Refs.part.position.set(0, 0.33, 0);
  mk4Refs.part.visible = false;
  s.add(mk4Refs.part);

  // 掉件滑道 + 光学传感器
  mBox(s, 0.3, 0.02, 0.26, 0x39414c, 0, 0.16, 0.2);
  mBox(s, 0.2, 0.1, 0.2, 0x3d4450, 0, 0.07, 0.32);
  mBox(s, 0.05, 0.06, 0.05, 0x555c66, -0.16, 0.2, 0.28);
  mBox(s, 0.05, 0.06, 0.05, 0x555c66, 0.16, 0.2, 0.28);
  mk4Refs.beamMat = new THREE.MeshStandardMaterial({ color: 0xff5b5b, emissive: new THREE.Color(0xff3030), emissiveIntensity: 1.4, transparent: true, opacity: 0.45 });
  mk4Refs.beam = new THREE.Mesh(new THREE.CylinderGeometry(0.005, 0.005, 0.3, 8), mk4Refs.beamMat);
  mk4Refs.beam.rotation.z = Math.PI / 2;
  mk4Refs.beam.position.set(0, 0.2, 0.28);
  s.add(mk4Refs.beam);

  // 顶出 servo
  mBox(s, 0.07, 0.07, 0.08, 0x3a4456, 0.28, 0.18, 0.12);
  mk4Refs.horn = mBox(s, 0.12, 0.02, 0.03, 0xc7ccd4, 0.33, 0.18, 0.1);
  s.add(mk4Refs.horn);

  // 控制面板 / PID / LCD
  mBox(s, 0.26, 0.16, 0.14, 0x39414c, -0.44, 0.1, 0.2, { emissive: 0x2f6f3f, emissiveIntensity: 1.2 });
  const lcd = mBox(s, 0.32, 0.16, 0.04, 0x39414c, 0.5, 0.24, 0.14, { emissive: 0x2f6f3f, emissiveIntensity: 1.2 });
  lcd.rotation.y = -0.6;
  mCyl(s, 0.03, 0.03, 0.06, 0x8a9098, 0.5, 0.14, 0.16);

  mk4PartFallT = -1;
}

function animateMk4(dt: number) {
  const e = mk4;
  const r = mk4Refs;
  if (e.phase === "closing_vise" && mk4PrevPhase !== "closing_vise") {
    mk4PartFallT = -1;
    r.part.visible = false;
    r.part.scale.setScalar(1);
    r.part.position.set(0, 0.33, 0);
    r.beamMat.opacity = 0.45;
  }
  mk4PrevPhase = e.phase;

  const jawX = JAW_OPEN_X + (JAW_CLOSED_X - JAW_OPEN_X) * e.viseOpen;
  r.jaw.position.x = jawX;
  const rodLen = jawX + 0.44;
  r.pistonRod.position.x = -0.42 + rodLen / 2;
  r.pistonRod.scale.x = Math.max(0.05, rodLen);

  r.ram.position.y = 1.27 - e.injectionRam * 0.2;
  r.auger.rotation.x = e.augerRpm * 0.15;

  r.beamMat.opacity = e.partDropDetected ? 0.1 : 0.45;
  r.horn.rotation.z = (-e.servoAngle / 180) * Math.PI;

  if (e.partFill > 0 && mk4PartFallT < 0) {
    r.part.visible = true;
    const sc = 0.25 + 0.75 * e.partFill;
    r.part.scale.setScalar(sc);
  }
  if (e.partDropped && mk4PartFallT < 0) mk4PartFallT = 0;
  if (mk4PartFallT >= 0) {
    mk4PartFallT += dt;
    const k = Math.min(1, mk4PartFallT / 1.1);
    r.part.position.y = 0.33 - 0.22 * k;
    r.part.position.z = 0.2 + 0.1 * k;
    r.part.visible = k < 0.98;
  }

  const glow = Math.min(1, e.chamberTemp / e.params.heating.targetTemp) * 1.4;
  for (const b of r.bands) (b.material as THREE.MeshStandardMaterial).emissiveIntensity = glow;
}

function wireResetMk4() {
  mk4PartFallT = -1;
  if (mk4Refs.part) mk4Refs.part.visible = false;
  if (mk4Refs.beamMat) mk4Refs.beamMat.opacity = 0.45;
}

/* ======= 场景搭建 ======= */
function buildScene() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x16181d);

  const container = viewportRef.value!;
  const w = container.clientWidth;
  const h = container.clientHeight;

  cam = new THREE.PerspectiveCamera(40, w / h, 0.01, 200);
  cam.position.set(machineType.value === "smart" ? 1.3 : 2.9, machineType.value === "smart" ? 1.7 : 2.4, machineType.value === "smart" ? 2.2 : 3.2);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  container.appendChild(renderer.domElement);

  controls = new OrbitControls(cam, renderer.domElement);
  controls.target.set(0, 0.8, 0);
  controls.update();

  scene.add(new THREE.HemisphereLight(0xdfe6f0, 0x2a2e35, 0.8));
  const dir = new THREE.DirectionalLight(0xffffff, 1.6);
  dir.position.set(3, 5, 3);
  dir.castShadow = true;
  scene.add(dir);
  const fill = new THREE.DirectionalLight(0x88aaff, 0.4);
  fill.position.set(-2, 1, -2);
  scene.add(fill);

  const grid = new THREE.GridHelper(6, 12, 0x3a3f47, 0x262a31);
  grid.position.y = 0.001;
  scene.add(grid);

  machineRoot = new THREE.Group();
  scene.add(machineRoot);

  if (machineType.value === "smart") buildSmartScene();
  else buildMk4Scene();
}

function onResize() {
  if (!renderer || !viewportRef.value) return;
  const w = viewportRef.value.clientWidth;
  const h = viewportRef.value.clientHeight;
  cam.aspect = w / h;
  cam.updateProjectionMatrix();
  renderer.setSize(w, h);
}

let lastTs = 0;
function animate(ts: number) {
  animId = requestAnimationFrame(animate);
  const realDt = lastTs ? Math.min(0.05, (ts - lastTs) / 1000) : 0.016;
  lastTs = ts;
  const dt = realDt * simSpeed.value;

  if (machineType.value === "smart") {
    smart.step(dt);
    animateSmart(realDt);
  } else {
    mk4.step(dt);
    animateMk4(realDt);
  }

  controls.update();
  renderer.render(scene, cam);
}

watch(machineType, () => {
  smartPrevPhase = "";
  mk4PrevPhase = "";
  if (renderer && viewportRef.value) viewportRef.value.removeChild(renderer.domElement);
  renderer?.dispose();
  buildScene();
});

onMounted(() => {
  buildScene();
  window.addEventListener("resize", onResize);
  lastTs = 0;
  animId = requestAnimationFrame(animate);
  (window as any).__injectorEngine = engine;
  (window as any).__smartLoaded = () => smartRefs.loaded;
  (window as any).__injectorSceneInfo = () => {
    const count = (o: THREE.Object3D): number => {
      let n = 0;
      o.traverse((c: THREE.Object3D) => { if ((c as THREE.Mesh).isMesh) n += 1; });
      return n;
    };
    return {
      loaded: smartRefs.loaded,
      cadMeshes: count(smartRefs.cadRoot),
      triangles: renderer.info.render.triangles,
      calls: renderer.info.render.calls,
      viewportSize: viewportRef.value ? [viewportRef.value.clientWidth, viewportRef.value.clientHeight] : null,
    };
  };
  (window as any).__injectorMaterials = () => {
    const seen = new Set<string>();
    smartRefs.cadRoot.traverse((c: THREE.Object3D) => {
      if ((c as THREE.Mesh).isMesh) {
        const m = (c as THREE.Mesh).material;
        if (Array.isArray(m)) m.forEach(x => seen.add(x.type));
        else seen.add((m as THREE.Material)?.type ?? "UNDEFINED");
      }
    });
    return { materials: [...seen], defaults: new THREE.MeshStandardMaterial().type };
  };
  (window as any).__injectorCadBox = () => {
    if (!smartRefs.cadRoot) return null;
    smartRefs.cadRoot.updateWorldMatrix(true, false);
    const box = new THREE.Box3().setFromObject(smartRefs.cadRoot);
    const mn = box.min, mx = box.max;
    return {
      min: [mn.x, mn.y, mn.z].map(v => +v.toFixed(3)),
      max: [mx.x, mx.y, mx.z].map(v => +v.toFixed(3)),
      size: [mx.x - mn.x, mx.y - mn.y, mx.z - mn.z].map(v => +v.toFixed(3)),
      rotation: [smartRefs.cadRoot.rotation.x, smartRefs.cadRoot.rotation.y, smartRefs.cadRoot.rotation.z].map(v => +v.toFixed(3)),
    };
  };
  (window as any).__injectorPixelStats = () => {
    if (!renderer || !viewportRef.value) return null;
    renderer.render(scene, cam);
    const gl = renderer.getContext();
    const w = renderer.domElement.width, h = renderer.domElement.height;
    const buf = new Uint8Array(w * h * 4);
    gl.readPixels(0, 0, w, h, gl.RGBA, gl.UNSIGNED_BYTE, buf);
    const cols = (h * 0.5);
    const rows: number[] = [];
    const bands = 8;
    for (let by = 0; by < bands; by++) {
      let sum = 0, n = 0;
      for (let y = by * (h / bands); y < (by + 1) * (h / bands); y++) {
        for (let x = 0; x < w; x += 10) {
          const i = (Math.floor(y) * w + Math.floor(x)) * 4;
          sum += (buf[i] + buf[i + 1] + buf[i + 2]) / 3;
          n++;
        }
      }
      rows.push(Math.round(sum / n));
    }
    const slice = (x0: number, x1: number) => {
      let sum = 0, n = 0;
      for (let y = 0; y < h; y += 6) {
        for (let x = x0; x < x1; x += 6) {
          const i = (Math.floor(y) * w + Math.floor(x)) * 4;
          sum += (buf[i] + buf[i + 1] + buf[i + 2]) / 3;
          n++;
        }
      }
      return Math.round(sum / n);
    };
    return { w, h, rows, cols, left: slice(0, w * 0.25), center: slice(w * 0.38, w * 0.62), right: slice(w * 0.75, w) };
  };
  (window as any).__injectorStats = () => ({
    type: machineType.value,
    phase: engine.value.phase,
    message: engine.value.message,
    smart: { clampPos: +smart.clampPos.toFixed(3), extRot: +smart.extRot.toFixed(2), materialLevel: +smart.materialLevel.toFixed(3), mouldTemp: +smart.mouldTemp.toFixed(1) },
    mk4: { viseOpen: +mk4.viseOpen.toFixed(3), injectionRam: +mk4.injectionRam.toFixed(3), partsLeft: mk4.partsLeft, chamberTemp: +mk4.chamberTemp.toFixed(1), partDropDetected: mk4.partDropDetected },
  });
});

onUnmounted(() => {
  window.removeEventListener("resize", onResize);
  cancelAnimationFrame(animId);
  if (renderer && viewportRef.value) {
    viewportRef.value.removeChild(renderer.domElement);
    renderer.dispose();
  }
});
</script>

<style scoped>
.injector-panel {
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

.param-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}

.param-row label {
  width: 118px;
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

.sel {
  flex: 1;
  padding: 3px 6px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 11px;
}

.btn-row {
  display: flex;
  gap: 6px;
  margin-bottom: 6px;
}

.btn-row button {
  flex: 1;
  padding: 5px 8px;
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  background: var(--surface-variant);
  color: var(--on-surface);
  font-size: 11px;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.btn-row button.primary {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}

.btn-row button.danger {
  background: var(--error);
  color: var(--on-error);
  border-color: var(--error);
}

.btn-row button.secondary:hover {
  background: var(--primary-container);
}

.btn-row button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.hint {
  font-size: 11px;
  color: var(--tertiary);
  min-height: 14px;
}

.hint.ok {
  color: var(--primary);
}

.info-row {
  display: flex;
  justify-content: space-between;
  font-size: 12px;
  padding: 2px 0;
  color: var(--on-surface-variant);
}

.info-row span:last-child {
  color: var(--on-surface);
  font-family: monospace;
}

.info-row .msg {
  max-width: 150px;
  text-align: right;
  color: var(--tertiary);
}

.info-row span.ok {
  color: var(--primary);
}

.bar {
  height: 8px;
  border-radius: 4px;
  background: var(--surface-variant);
  overflow: hidden;
  margin: 4px 0;
}

.bar-fill {
  height: 100%;
  background: var(--primary);
  border-radius: 4px;
  transition: width 0.1s linear;
}

.jog {
  flex: 1;
  accent-color: var(--primary);
}

.val {
  font-size: 11px;
  font-family: monospace;
  color: var(--on-surface-variant);
  min-width: 34px;
  text-align: right;
}
</style>
