<template>
  <div class="factory-sim">
    <!-- 顶部工具栏 -->
    <div class="fs-toolbar">
      <div class="toolbar-group">
        <span class="material-symbols-outlined tb-icon">factory</span>
        <span class="toolbar-title">工厂仿真</span>
      </div>
      <div class="toolbar-group">
        <button
          class="tb-btn"
          :class="{ active: viewMode === 'exterior' }"
          @click="switchView('exterior')"
        >
          <span class="material-symbols-outlined">landscape</span>外景
        </button>
        <button
          class="tb-btn"
          :class="{ active: viewMode === 'interior' }"
          @click="switchView('interior')"
        >
          <span class="material-symbols-outlined">warehouse</span>内景
        </button>
        <span class="tb-separator" />
        <button
          class="tb-btn"
          :class="{ active: showRoof }"
          @click="toggleRoof"
          title="显示/隐藏屋顶"
        >
          <span class="material-symbols-outlined">house</span>屋顶
        </button>
        <button
          class="tb-btn"
          :class="{ active: showWalls }"
          @click="toggleWalls"
          title="显示/隐藏墙壁"
        >
          <span class="material-symbols-outlined">view_week</span>墙壁
        </button>
      </div>
      <div class="toolbar-spacer" />
      <div class="toolbar-group">
        <button class="tb-btn" @click="resetCamera">
          <span class="material-symbols-outlined">center_focus_strong</span
          >重置视角
        </button>
      </div>
      <div class="toolbar-group" v-if="selectedItem">
        <span class="tb-separator" />
        <span class="tb-selection-info">{{ selectedItem.name }}</span>
        <button class="tb-btn tb-btn-sm" @click="deselectItem">
          <span class="material-symbols-outlined">close</span>
        </button>
      </div>
    </div>

    <div class="fs-body">
      <!-- 左侧设备库 -->
      <aside class="fs-sidebar">
        <div class="sidebar-section">
          <div class="section-header">
            <span>设备库</span>
          </div>
          <div class="model-search">
            <input
              class="prop-input"
              v-model="searchQuery"
              placeholder="搜索设备..."
            />
          </div>

          <!-- 加工设备 -->
          <div class="equip-category">
            <div class="cat-header" @click="toggleCategory('machining')">
              <span class="material-symbols-outlined toggle-arrow">
                {{ expandedCats.machining ? "expand_more" : "chevron_right" }}
              </span>
              <span>加工设备</span>
              <span class="cat-count">{{
                filteredByCategory("machining").length
              }}</span>
            </div>
            <div v-show="expandedCats.machining" class="cat-items">
              <div
                v-for="m in filteredByCategory('machining')"
                :key="m.id"
                class="model-lib-item"
                @click="addEquipment(m)"
              >
                <div class="model-thumb">
                  <span class="material-symbols-outlined">{{ m.icon }}</span>
                </div>
                <div class="model-lib-info">
                  <div class="model-lib-name">{{ m.name }}</div>
                  <div class="model-lib-type">{{ m.category }}</div>
                </div>
              </div>
            </div>
          </div>

          <!-- 机器人 -->
          <div class="equip-category">
            <div class="cat-header" @click="toggleCategory('robot')">
              <span class="material-symbols-outlined toggle-arrow">
                {{ expandedCats.robot ? "expand_more" : "chevron_right" }}
              </span>
              <span>机器人</span>
              <span class="cat-count">{{
                filteredByCategory("robot").length
              }}</span>
            </div>
            <div v-show="expandedCats.robot" class="cat-items">
              <div
                v-for="m in filteredByCategory('robot')"
                :key="m.id"
                class="model-lib-item"
                @click="addEquipment(m)"
              >
                <div class="model-thumb">
                  <span class="material-symbols-outlined">{{ m.icon }}</span>
                </div>
                <div class="model-lib-info">
                  <div class="model-lib-name">{{ m.name }}</div>
                  <div class="model-lib-type">{{ m.category }}</div>
                </div>
              </div>
            </div>
          </div>

          <!-- 物流设备 -->
          <div class="equip-category">
            <div class="cat-header" @click="toggleCategory('logistics')">
              <span class="material-symbols-outlined toggle-arrow">
                {{ expandedCats.logistics ? "expand_more" : "chevron_right" }}
              </span>
              <span>物流设备</span>
              <span class="cat-count">{{
                filteredByCategory("logistics").length
              }}</span>
            </div>
            <div v-show="expandedCats.logistics" class="cat-items">
              <div
                v-for="m in filteredByCategory('logistics')"
                :key="m.id"
                class="model-lib-item"
                @click="addEquipment(m)"
              >
                <div class="model-thumb">
                  <span class="material-symbols-outlined">{{ m.icon }}</span>
                </div>
                <div class="model-lib-info">
                  <div class="model-lib-name">{{ m.name }}</div>
                  <div class="model-lib-type">{{ m.category }}</div>
                </div>
              </div>
            </div>
          </div>

          <!-- 基础设施 -->
          <div class="equip-category">
            <div class="cat-header" @click="toggleCategory('infra')">
              <span class="material-symbols-outlined toggle-arrow">
                {{ expandedCats.infra ? "expand_more" : "chevron_right" }}
              </span>
              <span>基础设施</span>
              <span class="cat-count">{{
                filteredByCategory("infra").length
              }}</span>
            </div>
            <div v-show="expandedCats.infra" class="cat-items">
              <div
                v-for="m in filteredByCategory('infra')"
                :key="m.id"
                class="model-lib-item"
                @click="addEquipment(m)"
              >
                <div class="model-thumb">
                  <span class="material-symbols-outlined">{{ m.icon }}</span>
                </div>
                <div class="model-lib-info">
                  <div class="model-lib-name">{{ m.name }}</div>
                  <div class="model-lib-type">{{ m.category }}</div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- 场景中的设备列表 -->
        <div
          class="sidebar-section"
          style="border-top: 1px solid var(--outline-variant)"
        >
          <div class="section-header">
            <span>场景设备 ({{ sceneEquipment.length }})</span>
          </div>
          <div class="scene-equip-list">
            <div
              v-for="eq in sceneEquipment"
              :key="eq.uid"
              class="scene-equip-item"
              :class="{ selected: selectedItem?.uid === eq.uid }"
              @click="selectEquipment(eq)"
            >
              <span class="material-symbols-outlined eq-icon">{{
                eq.icon
              }}</span>
              <span class="eq-name">{{ eq.name }}</span>
              <button class="icon-btn-sm" @click.stop="removeEquipment(eq.uid)">
                <span class="material-symbols-outlined">close</span>
              </button>
            </div>
          </div>
        </div>
      </aside>

      <!-- 中央3D视口 -->
      <main class="fs-viewport">
        <canvas ref="threeCanvas" />
        <!-- 视口信息 -->
        <div class="viewport-info">
          <span>{{ viewMode === "exterior" ? "外景视角" : "内景视角" }}</span>
          <span class="info-sep">|</span>
          <span>{{ sceneEquipment.length }} 台设备</span>
        </div>
        <!-- 空场景提示 -->
        <div
          class="viewport-hint"
          v-if="sceneEquipment.length === 0 && viewMode === 'interior'"
        >
          <span
            class="material-symbols-outlined"
            style="font-size: 48px; opacity: 0.2"
            >factory</span
          >
          <p>从左侧设备库添加设备到厂房</p>
        </div>
      </main>

      <!-- 右侧属性面板 -->
      <aside class="fs-props" v-if="selectedItem">
        <div class="props-header">
          <span class="material-symbols-outlined" style="font-size: 14px"
            >tune</span
          >
          属性 — {{ selectedItem.name }}
        </div>
        <div class="props-body">
          <div class="prop-section">
            <div class="prop-section-title">位置</div>
            <div class="prop-row">
              <label>X</label>
              <input
                type="number"
                class="prop-input"
                v-model.number="selectedItem.posX"
                step="0.5"
                @input="updateEquipmentTransform"
              />
            </div>
            <div class="prop-row">
              <label>Y</label>
              <input
                type="number"
                class="prop-input"
                v-model.number="selectedItem.posY"
                step="0.5"
                @input="updateEquipmentTransform"
              />
            </div>
            <div class="prop-row">
              <label>Z</label>
              <input
                type="number"
                class="prop-input"
                v-model.number="selectedItem.posZ"
                step="0.5"
                @input="updateEquipmentTransform"
              />
            </div>
          </div>
          <div class="prop-section">
            <div class="prop-section-title">旋转 (度)</div>
            <div class="prop-row">
              <label>Y轴</label>
              <input
                type="number"
                class="prop-input"
                v-model.number="selectedItem.rotY"
                step="5"
                @input="updateEquipmentTransform"
              />
            </div>
          </div>
          <div class="prop-section">
            <div class="prop-section-title">缩放</div>
            <div class="prop-row">
              <label>比例</label>
              <input
                type="number"
                class="prop-input"
                v-model.number="selectedItem.scale"
                step="0.1"
                min="0.1"
                max="10"
                @input="updateEquipmentTransform"
              />
            </div>
          </div>
          <div class="prop-section">
            <button
              class="tb-btn tb-btn-danger full-width"
              @click="removeEquipment(selectedItem.uid)"
            >
              <span class="material-symbols-outlined">delete</span>删除设备
            </button>
          </div>
        </div>
      </aside>
      <aside class="fs-props fs-props-empty" v-else>
        <div class="props-header">
          <span class="material-symbols-outlined" style="font-size: 14px"
            >info</span
          >
          信息
        </div>
        <div class="props-body">
          <div class="empty-hint">
            <p>点击场景中的设备查看属性</p>
            <p class="hint-sub">从左侧设备库添加设备</p>
          </div>
        </div>
      </aside>
    </div>
  </div>
</template>

<script setup lang="ts">
import {
  ref,
  reactive,
  computed,
  onMounted,
  onUnmounted,
  watch,
  nextTick,
} from "vue";
import * as THREE from "three";
import { useKinematicsStore } from "@/stores/kinematics";

const emit = defineEmits<{
  connect: [];
  disconnect: [];
}>();

const kinStore = useKinematicsStore();

// ==================== 类型 ====================
interface EquipmentLibItem {
  id: string;
  name: string;
  category: "machining" | "robot" | "logistics" | "infra";
  icon: string;
  color: number;
  width: number;
  height: number;
  depth: number;
}

interface PlacedEquipment {
  uid: string;
  libId: string;
  name: string;
  icon: string;
  color: number;
  posX: number;
  posY: number;
  posZ: number;
  rotY: number;
  scale: number;
  width: number;
  height: number;
  depth: number;
  mesh?: THREE.Group;
}

// ==================== 设备库 ====================
const equipmentLib: EquipmentLibItem[] = [
  // 加工设备
  {
    id: "vmc",
    name: "VMC 立铣",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x3d4459,
    width: 2.5,
    height: 2.8,
    depth: 2.0,
  },
  {
    id: "hmc",
    name: "HMC 卧铣",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x3d4459,
    width: 3.0,
    height: 2.5,
    depth: 2.5,
  },
  {
    id: "lathe",
    name: "数控车床",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x424754,
    width: 3.0,
    height: 1.8,
    depth: 1.5,
  },
  {
    id: "5axis",
    name: "五轴加工中心",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x4d8eff,
    width: 3.5,
    height: 3.0,
    depth: 3.0,
  },
  {
    id: "router",
    name: "开料机",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x2d3449,
    width: 5.0,
    height: 2.0,
    depth: 3.5,
  },
  {
    id: "grinder",
    name: "磨床",
    category: "machining",
    icon: "precision_manufacturing",
    color: 0x3d4459,
    width: 2.0,
    height: 1.6,
    depth: 1.5,
  },
  // 机器人
  {
    id: "robot6",
    name: "6轴机器人",
    category: "robot",
    icon: "precision_manufacturing",
    color: 0xff6b35,
    width: 0.8,
    height: 1.8,
    depth: 0.8,
  },
  {
    id: "scara",
    name: "SCARA 机器人",
    category: "robot",
    icon: "precision_manufacturing",
    color: 0x4caf50,
    width: 0.7,
    height: 1.2,
    depth: 0.7,
  },
  {
    id: "delta",
    name: "Delta 并联",
    category: "robot",
    icon: "view_in_ar",
    color: 0x2196f3,
    width: 0.6,
    height: 1.5,
    depth: 0.6,
  },
  {
    id: "cobot",
    name: "协作机器人",
    category: "robot",
    icon: "precision_manufacturing",
    color: 0xff9800,
    width: 0.5,
    height: 1.4,
    depth: 0.5,
  },
  // 物流设备
  {
    id: "conveyor",
    name: "传送带",
    category: "logistics",
    icon: "moving",
    color: 0x607d8b,
    width: 6.0,
    height: 0.8,
    depth: 0.6,
  },
  {
    id: "agv",
    name: "AGV 小车",
    category: "logistics",
    icon: "local_shipping",
    color: 0x2196f3,
    width: 1.0,
    height: 0.5,
    depth: 0.6,
  },
  {
    id: "forklift",
    name: "叉车",
    category: "logistics",
    icon: "local_shipping",
    color: 0xff9800,
    width: 1.5,
    height: 2.0,
    depth: 1.0,
  },
  {
    id: "pallet",
    name: "托盘货架",
    category: "logistics",
    icon: "warehouse",
    color: 0x795548,
    width: 1.2,
    height: 2.5,
    depth: 1.0,
  },
  // 基础设施
  {
    id: "pillar",
    name: "立柱",
    category: "infra",
    icon: "square_foot",
    color: 0x9e9e9e,
    width: 0.4,
    height: 5.0,
    depth: 0.4,
  },
  {
    id: "workbench",
    name: "工作台",
    category: "infra",
    icon: "table_restaurant",
    color: 0x8d6e63,
    width: 2.0,
    height: 0.9,
    depth: 1.0,
  },
  {
    id: "cabinet",
    name: "电气柜",
    category: "infra",
    icon: "view_in_ar",
    color: 0x455a64,
    width: 0.8,
    height: 2.0,
    depth: 0.6,
  },
  {
    id: "safety_fence",
    name: "安全围栏",
    category: "infra",
    icon: "fence",
    color: 0xffeb3b,
    width: 3.0,
    height: 1.8,
    depth: 0.1,
  },
];

// 添加自定义运动学设备
const kinEquipItems = computed<EquipmentLibItem[]>(() =>
  kinStore.allRobotTypes.map((r) => ({
    id: `kin_${r.id}`,
    name: r.name,
    category: "robot" as const,
    icon: "precision_manufacturing",
    color: 0xe91e63,
    width: 0.8,
    height: 1.8,
    depth: 0.8,
  })),
);

const allEquipLib = computed(() => [...equipmentLib, ...kinEquipItems.value]);

// ==================== 状态 ====================
const viewMode = ref<"exterior" | "interior">("interior");
const showRoof = ref(true);
const showWalls = ref(true);
const searchQuery = ref("");
const expandedCats = reactive({
  machining: true,
  robot: true,
  logistics: false,
  infra: false,
});
const sceneEquipment = ref<PlacedEquipment[]>([]);
const selectedItem = ref<PlacedEquipment | null>(null);

// Three.js
const threeCanvas = ref<HTMLCanvasElement | null>(null);
let renderer: THREE.WebGLRenderer | null = null;
let scene: THREE.Scene | null = null;
let camera: THREE.PerspectiveCamera | null = null;
let animationId: number | null = null;

// 工厂场景组
let factoryGroup: THREE.Group | null = null;
let roofGroup: THREE.Group | null = null;
let wallsGroup: THREE.Group | null = null;
let groundGroup: THREE.Group | null = null;
let exteriorGroup: THREE.Group | null = null;
let equipmentGroup: THREE.Group | null = null;

// 鼠标控制
let isMouseDown = false;
let isRightMouse = false;
let lastMouseX = 0;
let lastMouseY = 0;
let cameraTheta = Math.PI / 4;
let cameraPhi = Math.PI / 3;
let cameraDistance = 30;
let cameraTarget = new THREE.Vector3(0, 2, 0);

// ==================== 过滤 ====================
function toggleCategory(cat: keyof typeof expandedCats) {
  expandedCats[cat] = !expandedCats[cat];
}

function filteredByCategory(cat: string) {
  const q = searchQuery.value.toLowerCase();
  return allEquipLib.value.filter(
    (e) => e.category === cat && (!q || e.name.toLowerCase().includes(q)),
  );
}

// ==================== Three.js 初始化 ====================
onMounted(() => {
  initThreeJS();
});

onUnmounted(() => {
  if (animationId !== null) cancelAnimationFrame(animationId);
  renderer?.dispose();
});

function initThreeJS() {
  if (!threeCanvas.value) return;
  const canvas = threeCanvas.value;
  const parent = canvas.parentElement!;
  const w = parent.clientWidth;
  const h = parent.clientHeight;

  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x87ceeb);
  scene.fog = new THREE.Fog(0x87ceeb, 80, 200);

  camera = new THREE.PerspectiveCamera(50, w / h, 0.1, 500);

  renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
  renderer.setSize(w, h);
  renderer.setPixelRatio(window.devicePixelRatio);
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;

  // 光照
  scene.add(new THREE.AmbientLight(0xffffff, 0.5));
  const sunLight = new THREE.DirectionalLight(0xfff4e0, 1.2);
  sunLight.position.set(30, 40, 20);
  sunLight.castShadow = true;
  sunLight.shadow.mapSize.set(2048, 2048);
  sunLight.shadow.camera.near = 0.5;
  sunLight.shadow.camera.far = 150;
  sunLight.shadow.camera.left = -50;
  sunLight.shadow.camera.right = 50;
  sunLight.shadow.camera.top = 50;
  sunLight.shadow.camera.bottom = -50;
  scene.add(sunLight);

  // 内景补光
  const indoorLight = new THREE.PointLight(0xfff8e8, 0.6, 60);
  indoorLight.position.set(0, 8, 0);
  scene.add(indoorLight);

  // 创建场景
  createFactoryScene();

  // 鼠标事件
  canvas.addEventListener("mousedown", onMouseDown);
  canvas.addEventListener("mousemove", onMouseMove);
  canvas.addEventListener("mouseup", onMouseUp);
  canvas.addEventListener("mouseleave", onMouseUp);
  canvas.addEventListener("wheel", onWheel);
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());

  updateCamera();
  animate();

  // 监听窗口大小
  window.addEventListener("resize", onResize);
}

function onResize() {
  if (!threeCanvas.value || !renderer || !camera) return;
  const parent = threeCanvas.value.parentElement!;
  const w = parent.clientWidth;
  const h = parent.clientHeight;
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
  renderer.setSize(w, h);
}

// ==================== 工厂场景 ====================
function createFactoryScene() {
  if (!scene) return;

  factoryGroup = new THREE.Group();
  groundGroup = new THREE.Group();
  exteriorGroup = new THREE.Group();
  wallsGroup = new THREE.Group();
  roofGroup = new THREE.Group();
  equipmentGroup = new THREE.Group();

  buildGround();
  buildExterior();
  buildFactoryBuilding();

  factoryGroup.add(groundGroup);
  factoryGroup.add(exteriorGroup);
  factoryGroup.add(wallsGroup);
  factoryGroup.add(roofGroup);
  factoryGroup.add(equipmentGroup);
  scene.add(factoryGroup);
}

function buildGround() {
  if (!groundGroup) return;

  // 大地面
  const groundGeo = new THREE.PlaneGeometry(200, 200);
  const groundMat = new THREE.MeshStandardMaterial({
    color: 0x7cb342,
    roughness: 0.9,
  });
  const ground = new THREE.Mesh(groundGeo, groundMat);
  ground.rotation.x = -Math.PI / 2;
  ground.position.y = -0.01;
  ground.receiveShadow = true;
  ground.name = "ground";
  groundGroup.add(ground);

  // 厂房内地面（浅灰）
  const floorGeo = new THREE.PlaneGeometry(40, 30);
  const floorMat = new THREE.MeshStandardMaterial({
    color: 0xbdbdbd,
    roughness: 0.7,
  });
  const floor = new THREE.Mesh(floorGeo, floorMat);
  floor.rotation.x = -Math.PI / 2;
  floor.position.y = 0.01;
  floor.receiveShadow = true;
  floor.name = "floor";
  groundGroup.add(floor);

  // 地面网格线
  const gridHelper = new THREE.GridHelper(200, 40, 0x689f38, 0x689f38);
  gridHelper.position.y = 0.02;
  gridHelper.material.opacity = 0.15;
  (gridHelper.material as THREE.Material).transparent = true;
  groundGroup.add(gridHelper);
}

function buildExterior() {
  if (!exteriorGroup) return;

  // 公路（南北向）
  const roadGeo = new THREE.PlaneGeometry(8, 80);
  const roadMat = new THREE.MeshStandardMaterial({
    color: 0x424242,
    roughness: 0.8,
  });
  const road = new THREE.Mesh(roadGeo, roadMat);
  road.rotation.x = -Math.PI / 2;
  road.position.set(-25, 0.03, 0);
  road.receiveShadow = true;
  exteriorGroup.add(road);

  // 公路（东西向）
  const road2 = road.clone();
  road2.rotation.z = Math.PI / 2;
  road2.position.set(0, 0.03, -25);
  exteriorGroup.add(road2);

  // 公路中线
  const lineGeo = new THREE.PlaneGeometry(0.2, 78);
  const lineMat = new THREE.MeshStandardMaterial({ color: 0xffeb3b });
  const line1 = new THREE.Mesh(lineGeo, lineMat);
  line1.rotation.x = -Math.PI / 2;
  line1.position.set(-25, 0.04, 0);
  exteriorGroup.add(line1);

  const line2 = line1.clone();
  line2.rotation.z = Math.PI / 2;
  line2.position.set(0, 0.04, -25);
  exteriorGroup.add(line2);

  // 树木
  const treePositions = [
    [-35, 10],
    [-35, -10],
    [-35, 20],
    [-35, -20],
    [-35, 30],
    [-35, -30],
    [25, -15],
    [25, -25],
    [30, -15],
    [30, -25],
    [25, 15],
    [25, 25],
    [30, 15],
    [30, 25],
    [-15, -18],
    [-5, -18],
    [5, -18],
    [15, -18],
  ];
  for (const [tx, tz] of treePositions) {
    exteriorGroup.add(createTree(tx, tz));
  }

  // 停车场标记
  for (let i = 0; i < 6; i++) {
    const spotGeo = new THREE.PlaneGeometry(2.5, 5);
    const spotMat = new THREE.MeshStandardMaterial({
      color: 0x616161,
      roughness: 0.8,
    });
    const spot = new THREE.Mesh(spotGeo, spotMat);
    spot.rotation.x = -Math.PI / 2;
    spot.position.set(25 + i * 3, 0.03, -15);
    exteriorGroup.add(spot);
  }

  // 围栏（简化为柱子+线）
  for (let x = -40; x <= 40; x += 10) {
    exteriorGroup.add(createFencePost(x, 35));
    exteriorGroup.add(createFencePost(x, -35));
  }
  for (let z = -35; z <= 35; z += 10) {
    exteriorGroup.add(createFencePost(40, z));
    exteriorGroup.add(createFencePost(-40, z));
  }
}

function createTree(x: number, z: number): THREE.Group {
  const g = new THREE.Group();
  // 树干
  const trunkGeo = new THREE.CylinderGeometry(0.15, 0.2, 3, 8);
  const trunkMat = new THREE.MeshStandardMaterial({ color: 0x5d4037 });
  const trunk = new THREE.Mesh(trunkGeo, trunkMat);
  trunk.position.set(0, 1.5, 0);
  trunk.castShadow = true;
  g.add(trunk);
  // 树冠
  const crownGeo = new THREE.SphereGeometry(1.5, 8, 6);
  const crownMat = new THREE.MeshStandardMaterial({ color: 0x2e7d32 });
  const crown = new THREE.Mesh(crownGeo, crownMat);
  crown.position.set(0, 4, 0);
  crown.castShadow = true;
  g.add(crown);
  g.position.set(x, 0, z);
  return g;
}

function createFencePost(x: number, z: number): THREE.Group {
  const g = new THREE.Group();
  const postGeo = new THREE.CylinderGeometry(0.05, 0.05, 2.5, 6);
  const postMat = new THREE.MeshStandardMaterial({ color: 0x9e9e9e });
  const post = new THREE.Mesh(postGeo, postMat);
  post.position.set(0, 1.25, 0);
  g.add(post);
  g.position.set(x, 0, z);
  return g;
}

function buildFactoryBuilding() {
  if (!wallsGroup || !roofGroup) return;

  const fw = 40; // 厂房宽
  const fd = 30; // 厂房深
  const fh = 8; // 厂房高

  // 墙壁
  const wallMat = new THREE.MeshStandardMaterial({
    color: 0xe0e0e0,
    roughness: 0.6,
    side: THREE.DoubleSide,
  });

  // 前墙（有大门）
  const frontWall = createWallWithDoor(fw, fh, 0xe0e0e0);
  frontWall.position.set(0, fh / 2, fd / 2);
  wallsGroup.add(frontWall);

  // 后墙
  const backWall = createWall(fw, fh, 0xdedede);
  backWall.position.set(0, fh / 2, -fd / 2);
  wallsGroup.add(backWall);

  // 左墙（带窗户）
  const leftWall = createWallWithWindows(fd, fh, 0xe8e8e8);
  leftWall.rotation.y = Math.PI / 2;
  leftWall.position.set(-fw / 2, fh / 2, 0);
  wallsGroup.add(leftWall);

  // 右墙（带窗户）
  const rightWall = createWallWithWindows(fd, fh, 0xe8e8e8);
  rightWall.rotation.y = Math.PI / 2;
  rightWall.position.set(fw / 2, fh / 2, 0);
  wallsGroup.add(rightWall);

  // 屋顶
  const roofGeo = new THREE.BoxGeometry(fw + 1, 0.3, fd + 1);
  const roofMat = new THREE.MeshStandardMaterial({
    color: 0x78909c,
    roughness: 0.5,
  });
  const roof = new THREE.Mesh(roofGeo, roofMat);
  roof.position.y = fh;
  roof.castShadow = true;
  roofGroup.add(roof);

  // 屋顶钢梁（可见的结构）
  for (let x = -fw / 2 + 5; x <= fw / 2; x += 10) {
    const beamGeo = new THREE.BoxGeometry(0.2, 0.3, fd);
    const beamMat = new THREE.MeshStandardMaterial({ color: 0x546e7a });
    const beam = new THREE.Mesh(beamGeo, beamMat);
    beam.position.set(x, fh - 0.3, 0);
    roofGroup.add(beam);
  }

  // 内部照明灯
  for (let x = -15; x <= 15; x += 10) {
    for (let z = -10; z <= 10; z += 10) {
      const lampGeo = new THREE.BoxGeometry(1.5, 0.1, 0.3);
      const lampMat = new THREE.MeshStandardMaterial({
        color: 0xfff9c4,
        emissive: 0xfff9c4,
        emissiveIntensity: 0.3,
      });
      const lamp = new THREE.Mesh(lampGeo, lampMat);
      lamp.position.set(x, fh - 0.5, z);
      wallsGroup.add(lamp);
    }
  }

  // 柱子（内部支撑）
  for (let x = -15; x <= 15; x += 10) {
    for (let z of [-10, 10]) {
      const pillarGeo = new THREE.CylinderGeometry(0.2, 0.25, fh, 8);
      const pillarMat = new THREE.MeshStandardMaterial({ color: 0x90a4ae });
      const pillar = new THREE.Mesh(pillarGeo, pillarMat);
      pillar.position.set(x, fh / 2, z);
      pillar.castShadow = true;
      wallsGroup.add(pillar);
    }
  }

  // 地面标线
  const lineMat = new THREE.MeshStandardMaterial({ color: 0xfdd835 });
  // 通道标线
  for (let z of [-5, 5]) {
    const lineGeo = new THREE.PlaneGeometry(fw - 2, 0.15);
    const line = new THREE.Mesh(lineGeo, lineMat);
    line.rotation.x = -Math.PI / 2;
    line.position.set(0, 0.05, z);
    wallsGroup.add(line);
  }
}

function createWall(w: number, h: number, color: number): THREE.Mesh {
  const geo = new THREE.PlaneGeometry(w, h);
  const mat = new THREE.MeshStandardMaterial({
    color,
    side: THREE.DoubleSide,
    transparent: true,
    opacity: 0.85,
  });
  const wall = new THREE.Mesh(geo, mat);
  return wall;
}

function createWallWithDoor(w: number, h: number, color: number): THREE.Group {
  const g = new THREE.Group();
  const doorW = 6;
  const doorH = 5;

  // 左半墙
  const leftW = (w - doorW) / 2;
  if (leftW > 0) {
    const leftGeo = new THREE.PlaneGeometry(leftW, h);
    const mat = new THREE.MeshStandardMaterial({
      color,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.85,
    });
    const left = new THREE.Mesh(leftGeo, mat);
    left.position.set(-w / 2 + leftW / 2, 0, 0);
    g.add(left);
  }

  // 右半墙
  const rightW = (w - doorW) / 2;
  if (rightW > 0) {
    const rightGeo = new THREE.PlaneGeometry(rightW, h);
    const mat = new THREE.MeshStandardMaterial({
      color,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.85,
    });
    const right = new THREE.Mesh(rightGeo, mat);
    right.position.set(w / 2 - rightW / 2, 0, 0);
    g.add(right);
  }

  // 门上方
  const topH = h - doorH;
  if (topH > 0) {
    const topGeo = new THREE.PlaneGeometry(doorW, topH);
    const mat = new THREE.MeshStandardMaterial({
      color,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.85,
    });
    const top = new THREE.Mesh(topGeo, mat);
    top.position.set(0, doorH / 2 + topH / 2, 0);
    g.add(top);
  }

  // 门框
  const frameMat = new THREE.MeshStandardMaterial({ color: 0x455a64 });
  const frameThickness = 0.15;
  // 左框
  const lf = new THREE.Mesh(
    new THREE.BoxGeometry(frameThickness, doorH, frameThickness),
    frameMat,
  );
  lf.position.set(-doorW / 2, -h / 2 + doorH / 2, 0);
  g.add(lf);
  // 右框
  const rf = lf.clone();
  rf.position.x = doorW / 2;
  g.add(rf);
  // 上框
  const tf = new THREE.Mesh(
    new THREE.BoxGeometry(doorW, frameThickness, frameThickness),
    frameMat,
  );
  tf.position.set(0, -h / 2 + doorH, 0);
  g.add(tf);

  return g;
}

function createWallWithWindows(
  w: number,
  h: number,
  color: number,
): THREE.Group {
  const g = new THREE.Group();
  const wallMat = new THREE.MeshStandardMaterial({
    color,
    side: THREE.DoubleSide,
    transparent: true,
    opacity: 0.85,
  });
  const glassMat = new THREE.MeshStandardMaterial({
    color: 0x80deea,
    transparent: true,
    opacity: 0.4,
    side: THREE.DoubleSide,
  });

  // 基础墙体
  const baseGeo = new THREE.PlaneGeometry(w, h);
  const base = new THREE.Mesh(baseGeo, wallMat);
  g.add(base);

  // 窗户
  const winW = 2.5;
  const winH = 2.0;
  const winY = 3;
  const numWin = Math.floor(w / 5);
  const startX = (-(numWin - 1) * 5) / 2;
  for (let i = 0; i < numWin; i++) {
    const winGeo = new THREE.PlaneGeometry(winW, winH);
    const win = new THREE.Mesh(winGeo, glassMat);
    win.position.set(startX + i * 5, winY, 0.01);
    g.add(win);

    // 窗框
    const frameMat = new THREE.MeshStandardMaterial({ color: 0x455a64 });
    const ft = 0.08;
    // 上下框
    for (const dy of [-winH / 2, winH / 2]) {
      const f = new THREE.Mesh(
        new THREE.BoxGeometry(winW + ft * 2, ft, ft),
        frameMat,
      );
      f.position.set(startX + i * 5, winY + dy, 0.02);
      g.add(f);
    }
    // 左右框
    for (const dx of [-winW / 2, winW / 2]) {
      const f = new THREE.Mesh(new THREE.BoxGeometry(ft, winH, ft), frameMat);
      f.position.set(startX + i * 5 + dx, winY, 0.02);
      g.add(f);
    }
  }

  return g;
}

// ==================== 设备添加 ====================
function addEquipment(libItem: EquipmentLibItem) {
  const uid = `eq_${Date.now()}_${Math.random().toString(36).slice(2, 6)}`;
  const eq: PlacedEquipment = {
    uid,
    libId: libItem.id,
    name: libItem.name,
    icon: libItem.icon,
    color: libItem.color,
    posX: (Math.random() - 0.5) * 20,
    posY: libItem.height / 2,
    posZ: (Math.random() - 0.5) * 14,
    rotY: 0,
    scale: 1,
    width: libItem.width,
    height: libItem.height,
    depth: libItem.depth,
  };

  // 放置到厂房内
  eq.posY = 0;
  sceneEquipment.value.push(eq);
  buildEquipmentMesh(eq);
  selectEquipment(eq);
}

function buildEquipmentMesh(eq: PlacedEquipment) {
  if (!equipmentGroup) return;
  const g = new THREE.Group();
  const c = eq.color;

  const M = (clr: number, rough = 0.6) =>
    new THREE.MeshStandardMaterial({ color: clr, roughness: rough });
  const metal = (clr: number) => M(clr, 0.35);
  const dark = M(0x2b2b2b, 0.7);
  const white = M(0xf5f5f5, 0.5);
  const glass = new THREE.MeshStandardMaterial({
    color: 0x90caf9,
    transparent: true,
    opacity: 0.4,
    roughness: 0.1,
  });

  function box(w: number, h: number, d: number, mat: THREE.Material) {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
    m.castShadow = true;
    m.receiveShadow = true;
    return m;
  }
  function cyl(
    rt: number,
    rb: number,
    h: number,
    mat: THREE.Material,
    seg = 16,
  ) {
    const m = new THREE.Mesh(new THREE.CylinderGeometry(rt, rb, h, seg), mat);
    m.castShadow = true;
    return m;
  }

  switch (eq.libId) {
    // ========== VMC 立式加工中心 ==========
    case "vmc": {
      // 床身
      const bed = box(2.5, 0.3, 2.0, dark);
      bed.position.y = 0.15;
      g.add(bed);
      // 工作台
      const table = box(1.6, 0.15, 1.4, metal(0x616161));
      table.position.set(0, 0.45, 0);
      g.add(table);
      // 立柱
      const col = box(0.4, 2.4, 0.4, M(c));
      col.position.set(-0.8, 1.35, 0);
      g.add(col);
      // 主轴箱
      const head = box(0.6, 0.6, 0.6, M(0x1565c0));
      head.position.set(-0.8, 1.6, 0);
      g.add(head);
      // 主轴
      const spindle = cyl(0.08, 0.08, 0.7, metal(0xccc));
      spindle.position.set(-0.8, 1.05, 0);
      g.add(spindle);
      // 刀具
      const tool = cyl(0.03, 0.02, 0.3, metal(0xffab00));
      tool.position.set(-0.8, 0.55, 0);
      g.add(tool);
      // 防护门
      const door = box(1.0, 1.8, 0.05, glass);
      door.position.set(-0.1, 1.2, 1.02);
      g.add(door);
      // 控制面板
      const panel = box(0.3, 0.5, 0.08, M(0x333));
      panel.position.set(0.9, 1.0, 1.05);
      g.add(panel);
      // 屏幕
      const scr = box(0.22, 0.15, 0.02, M(0x1b5e20));
      scr.position.set(0.9, 1.15, 1.1);
      g.add(scr);
      break;
    }
    // ========== HMC 卧式加工中心 ==========
    case "hmc": {
      const bed = box(3.0, 0.35, 2.5, dark);
      bed.position.y = 0.175;
      g.add(bed);
      // 工作台（带B轴旋转台）
      const table = box(1.4, 0.15, 1.4, metal(0x616161));
      table.position.set(0.5, 0.55, 0);
      g.add(table);
      const bTable = cyl(0.5, 0.5, 0.2, M(0x1565c0));
      bTable.position.set(0.5, 0.75, 0);
      g.add(bTable);
      // 立柱
      const col = box(0.5, 2.5, 0.5, M(c));
      col.position.set(-1.0, 1.4, 0);
      g.add(col);
      // 卧式主轴箱
      const head = box(0.8, 0.5, 0.5, M(0x1565c0));
      head.position.set(-0.3, 1.6, 0);
      g.add(head);
      // 水平主轴
      const sp = cyl(0.08, 0.08, 0.8, metal(0xccc));
      sp.rotation.z = Math.PI / 2;
      sp.position.set(0.3, 1.6, 0);
      g.add(sp);
      // 刀具
      const tl = cyl(0.03, 0.02, 0.25, metal(0xffab00));
      tl.rotation.z = Math.PI / 2;
      tl.position.set(0.85, 1.6, 0);
      g.add(tl);
      // 防护门
      const dr = box(1.4, 2.0, 0.05, glass);
      dr.position.set(0.3, 1.2, 1.28);
      g.add(dr);
      break;
    }
    // ========== 数控车床 ==========
    case "lathe": {
      // 床身
      const bed = box(3.0, 0.25, 1.5, dark);
      bed.position.y = 0.125;
      g.add(bed);
      // 主轴箱（左侧）
      const hs = box(0.6, 0.8, 1.0, M(c));
      hs.position.set(-1.1, 0.65, 0);
      g.add(hs);
      // 卡盘
      const chuck = cyl(0.35, 0.35, 0.15, metal(0x888));
      chuck.rotation.z = Math.PI / 2;
      chuck.position.set(-0.7, 0.65, 0);
      g.add(chuck);
      // 尾座（右侧）
      const ts = box(0.4, 0.6, 0.6, M(c));
      ts.position.set(1.1, 0.55, 0);
      g.add(ts);
      // 刀架
      const tp = box(0.3, 0.3, 0.3, M(0x1565c0));
      tp.position.set(0, 0.75, 0.45);
      g.add(tp);
      // 刀具
      const kn = box(0.05, 0.03, 0.25, metal(0xffab00));
      kn.position.set(0, 0.75, 0.65);
      g.add(kn);
      // 导轨
      const rail1 = box(2.6, 0.04, 0.06, metal(0xaaa));
      rail1.position.set(0, 0.38, 0.35);
      g.add(rail1);
      const rail2 = rail1.clone();
      rail2.position.z = -0.35;
      g.add(rail2);
      // 防护门
      const door = box(1.2, 1.0, 0.05, glass);
      door.position.set(0, 0.7, 0.78);
      g.add(door);
      break;
    }
    // ========== 五轴加工中心 ==========
    case "5axis": {
      const bed = box(3.5, 0.3, 3.0, dark);
      bed.position.y = 0.15;
      g.add(bed);
      // 立柱
      const col = box(0.5, 2.6, 0.5, M(c));
      col.position.set(-1.3, 1.6, 0);
      g.add(col);
      // 主轴箱
      const hb = box(0.7, 0.7, 0.7, M(0x1565c0));
      hb.position.set(-1.3, 2.2, 0);
      g.add(hb);
      // 主轴
      const sp = cyl(0.09, 0.09, 0.8, metal(0xccc));
      sp.position.set(-1.3, 1.5, 0);
      g.add(sp);
      // 转台（A+C）
      const rt = cyl(0.55, 0.55, 0.25, M(0x1565c0));
      rt.position.set(0.3, 0.55, 0);
      g.add(rt);
      // 工件
      const wp = box(0.6, 0.5, 0.6, M(0x4caf50));
      wp.position.set(0.3, 1.05, 0);
      g.add(wp);
      // 刀具
      const tl = cyl(0.03, 0.02, 0.3, metal(0xffab00));
      tl.position.set(-1.3, 0.85, 0);
      g.add(tl);
      break;
    }
    // ========== 开料机 Router ==========
    case "router": {
      // 床身
      const bed = box(5.0, 0.2, 3.5, dark);
      bed.position.y = 0.1;
      g.add(bed);
      // 台面
      const top = box(4.8, 0.08, 3.3, M(0x795548));
      top.position.y = 0.24;
      g.add(top);
      // 左立柱
      const cL = box(0.25, 2.0, 0.25, M(c));
      cL.position.set(-2.2, 1.2, 0);
      g.add(cL);
      // 右立柱
      const cR = box(0.25, 2.0, 0.25, M(c));
      cR.position.set(2.2, 1.2, 0);
      g.add(cR);
      // 横梁
      const beam = box(4.6, 0.2, 0.3, M(c));
      beam.position.set(0, 2.1, 0);
      g.add(beam);
      // 主轴头
      const hd = box(0.35, 0.5, 0.35, M(0x1565c0));
      hd.position.set(0, 1.75, 0);
      g.add(hd);
      // 主轴
      const sp = cyl(0.06, 0.06, 0.4, metal(0xccc));
      sp.position.set(0, 1.3, 0);
      g.add(sp);
      // 刀具
      const tl = cyl(0.02, 0.015, 0.2, metal(0xffab00));
      tl.position.set(0, 0.95, 0);
      g.add(tl);
      break;
    }
    // ========== 磨床 ==========
    case "grinder": {
      const bed = box(2.0, 0.2, 1.5, dark);
      bed.position.y = 0.1;
      g.add(bed);
      // 工作台
      const tbl = box(1.4, 0.1, 1.2, metal(0x616161));
      tbl.position.y = 0.3;
      g.add(tbl);
      // 砂轮罩
      const guard = cyl(0.4, 0.4, 0.15, M(0xc62828));
      guard.position.set(0, 0.9, -0.3);
      g.add(guard);
      // 砂轮
      const wheel = cyl(0.35, 0.35, 0.08, M(0x9e9e9e));
      wheel.position.set(0, 0.9, -0.15);
      g.add(wheel);
      // 立柱
      const col = box(0.3, 1.5, 0.3, M(c));
      col.position.set(0, 0.95, -0.45);
      g.add(col);
      break;
    }
    // ========== 6轴机器人 ==========
    case "robot6": {
      // 底座
      const base = cyl(0.25, 0.3, 0.2, dark);
      base.position.y = 0.1;
      g.add(base);
      // J1 转台
      const j1 = cyl(0.18, 0.18, 0.12, M(0xff6b35));
      j1.position.y = 0.26;
      g.add(j1);
      // 肩部
      const shoulder = cyl(0.12, 0.12, 0.15, M(0xff6b35));
      shoulder.position.y = 0.4;
      g.add(shoulder);
      // 大臂
      const upper = box(0.1, 0.55, 0.1, M(0xff8a50));
      upper.position.set(0, 0.75, 0);
      g.add(upper);
      // 肘关节
      const elbow = cyl(0.09, 0.09, 0.1, M(0xff6b35));
      elbow.position.set(0, 1.05, 0);
      g.add(elbow);
      // 小臂
      const fore = box(0.08, 0.4, 0.08, M(0xff8a50));
      fore.position.set(0, 1.3, 0);
      g.add(fore);
      // 手腕
      const wrist = cyl(0.06, 0.06, 0.08, M(0xff6b35));
      wrist.position.set(0, 1.55, 0);
      g.add(wrist);
      // 末端法兰
      const flange = cyl(0.04, 0.04, 0.04, metal(0xaaa));
      flange.position.set(0, 1.62, 0);
      g.add(flange);
      break;
    }
    // ========== SCARA 机器人 ==========
    case "scara": {
      const base = cyl(0.2, 0.25, 0.2, dark);
      base.position.y = 0.1;
      g.add(base);
      // J1
      const j1 = cyl(0.1, 0.1, 0.08, M(0x4caf50));
      j1.position.y = 0.24;
      g.add(j1);
      // 大臂
      const arm1 = box(0.5, 0.06, 0.1, M(0x66bb6a));
      arm1.position.set(0.25, 0.3, 0);
      g.add(arm1);
      // J2
      const j2 = cyl(0.08, 0.08, 0.06, M(0x4caf50));
      j2.position.set(0.5, 0.3, 0);
      g.add(j2);
      // 小臂
      const arm2 = box(0.4, 0.05, 0.08, M(0x66bb6a));
      arm2.position.set(0.75, 0.3, 0);
      g.add(arm2);
      // Z轴丝杆
      const screw = cyl(0.025, 0.025, 0.3, metal(0xaaa));
      screw.position.set(0.95, 0.25, 0);
      g.add(screw);
      // 末端
      const end = cyl(0.04, 0.04, 0.03, metal(0xffab00));
      end.position.set(0.95, 0.12, 0);
      g.add(end);
      break;
    }
    // ========== Delta 并联机器人 ==========
    case "delta": {
      // 顶部平台
      const top = cyl(0.3, 0.3, 0.06, M(0x2196f3));
      top.position.y = 1.3;
      g.add(top);
      // 三根导轨
      for (let i = 0; i < 3; i++) {
        const a = (i * 120 * Math.PI) / 180;
        const rx = Math.cos(a) * 0.22;
        const rz = Math.sin(a) * 0.22;
        const rail = cyl(0.02, 0.02, 0.9, M(c));
        rail.position.set(rx, 0.85, rz);
        g.add(rail);
        // 连杆
        const rod = box(0.02, 0.5, 0.02, M(0x90a4ae));
        rod.position.set(rx * 0.5, 0.75, rz * 0.5);
        g.add(rod);
      }
      // 底部动平台
      const bottom = cyl(0.12, 0.12, 0.04, M(0x42a5f5));
      bottom.position.y = 0.35;
      g.add(bottom);
      // 吸盘
      const cup = cyl(0.04, 0.06, 0.06, M(0x333));
      cup.position.y = 0.28;
      g.add(cup);
      break;
    }
    // ========== 协作机器人 ==========
    case "cobot": {
      const base = cyl(0.18, 0.22, 0.15, dark);
      base.position.y = 0.075;
      g.add(base);
      const j1 = cyl(0.1, 0.1, 0.08, M(0xff9800));
      j1.position.y = 0.19;
      g.add(j1);
      // 大臂
      const arm1 = box(0.07, 0.45, 0.07, M(0xffb74d));
      arm1.position.set(0, 0.5, 0);
      g.add(arm1);
      // 肘关节
      const ej = cyl(0.06, 0.06, 0.07, M(0xff9800));
      ej.position.set(0, 0.76, 0);
      g.add(ej);
      // 小臂
      const arm2 = box(0.06, 0.35, 0.06, M(0xffb74d));
      arm2.position.set(0, 0.98, 0);
      g.add(arm2);
      // 腕关节
      const wj = cyl(0.04, 0.04, 0.06, M(0xff9800));
      wj.position.set(0, 1.18, 0);
      g.add(wj);
      // 末端
      const fl = cyl(0.03, 0.03, 0.03, metal(0xaaa));
      fl.position.set(0, 1.25, 0);
      g.add(fl);
      break;
    }
    // ========== 传送带 ==========
    case "conveyor": {
      // 支架腿
      const legMat = M(0x616161);
      for (let x = -eq.width / 2 + 0.3; x <= eq.width / 2 - 0.3; x += 1.2) {
        for (const z of [-eq.depth / 2 + 0.08, eq.depth / 2 - 0.08]) {
          const leg = box(0.05, eq.height * 0.5, 0.05, legMat);
          leg.position.set(x, eq.height * 0.25, z);
          g.add(leg);
        }
      }
      // 横梁
      for (let x = -eq.width / 2 + 0.3; x <= eq.width / 2 - 0.3; x += 1.2) {
        const cross = box(0.05, 0.05, eq.depth * 0.9, legMat);
        cross.position.set(x, eq.height * 0.5, 0);
        g.add(cross);
      }
      // 皮带面
      const belt = box(eq.width - 0.1, 0.03, eq.depth * 0.85, M(0x212121));
      belt.position.y = eq.height * 0.53;
      g.add(belt);
      // 滚筒
      const rollerMat = metal(0x888);
      for (let x = -eq.width / 2 + 0.15; x <= eq.width / 2 - 0.15; x += 0.5) {
        const roller = cyl(0.04, 0.04, eq.depth * 0.8, rollerMat);
        roller.rotation.x = Math.PI / 2;
        roller.position.set(x, eq.height * 0.5, 0);
        g.add(roller);
      }
      // 电机（端部）
      const motor = box(0.2, 0.2, 0.2, M(0x1565c0));
      motor.position.set(eq.width / 2 - 0.1, eq.height * 0.5, 0);
      g.add(motor);
      break;
    }
    // ========== AGV 小车 ==========
    case "agv": {
      // 底盘
      const chassis = box(1.0, 0.12, 0.6, M(0x1565c0));
      chassis.position.y = 0.15;
      g.add(chassis);
      // 车身
      const body = box(0.8, 0.15, 0.5, M(0x2196f3));
      body.position.y = 0.29;
      g.add(body);
      // 轮子
      const wheelMat = dark;
      for (const [wx, wz] of [
        [-0.35, 0.3],
        [0.35, 0.3],
        [-0.35, -0.3],
        [0.35, -0.3],
      ]) {
        const wheel = cyl(0.08, 0.08, 0.04, wheelMat, 12);
        wheel.rotation.x = Math.PI / 2;
        wheel.position.set(wx, 0.08, wz);
        g.add(wheel);
      }
      // 导航灯
      const nav = cyl(0.04, 0.04, 0.06, M(0x4caf50));
      nav.position.set(0, 0.4, 0);
      g.add(nav);
      // 货物区
      const cargo = box(0.6, 0.08, 0.4, M(0x90a4ae));
      cargo.position.set(0, 0.41, 0);
      g.add(cargo);
      break;
    }
    // ========== 叉车 ==========
    case "forklift": {
      // 底盘
      const ch = box(1.2, 0.15, 0.8, M(0xf57c00));
      ch.position.y = 0.15;
      g.add(ch);
      // 车身
      const bd = box(0.8, 0.6, 0.7, M(0xff9800));
      bd.position.set(-0.1, 0.55, 0);
      g.add(bd);
      // 驾驶室
      const cab = box(0.5, 0.55, 0.6, M(0x333));
      cab.position.set(-0.15, 0.95, 0);
      g.add(cab);
      // 挡风玻璃
      const glass2 = box(0.45, 0.35, 0.02, glass);
      glass2.position.set(-0.15, 1.0, 0.32);
      g.add(glass2);
      // 门架
      const mast = box(0.06, 1.6, 0.06, M(0x424242));
      mast.position.set(0.55, 0.95, 0);
      g.add(mast);
      const mast2 = mast.clone();
      mast2.position.z = 0.25;
      g.add(mast2);
      // 货叉
      const fork1 = box(0.5, 0.04, 0.06, M(0x888));
      fork1.position.set(0.75, 0.35, -0.1);
      g.add(fork1);
      const fork2 = fork1.clone();
      fork2.position.z = 0.1;
      g.add(fork2);
      // 轮子
      for (const [wx, wz] of [
        [-0.45, 0.35],
        [0.4, 0.35],
        [-0.45, -0.35],
        [0.4, -0.35],
      ]) {
        const wh = cyl(0.1, 0.1, 0.06, dark, 12);
        wh.rotation.x = Math.PI / 2;
        wh.position.set(wx, 0.1, wz);
        g.add(wh);
      }
      break;
    }
    // ========== 托盘货架 ==========
    case "pallet": {
      // 立柱
      const pMat = M(0x795548);
      for (const [px, pz] of [
        [-0.5, -0.4],
        [0.5, -0.4],
        [-0.5, 0.4],
        [0.5, 0.4],
      ]) {
        const p = box(0.05, eq.height, 0.05, pMat);
        p.position.set(px, eq.height / 2, pz);
        g.add(p);
      }
      // 层板
      for (let y = 0.4; y < eq.height; y += 0.6) {
        const shelf = box(eq.width - 0.1, 0.04, eq.depth - 0.1, M(0x8d6e63));
        shelf.position.y = y;
        g.add(shelf);
        // 横撑
        const bar = box(eq.width - 0.1, 0.03, 0.03, pMat);
        bar.position.set(0, y + 0.03, eq.depth / 2 - 0.05);
        g.add(bar);
        const bar2 = bar.clone();
        bar2.position.z = -eq.depth / 2 + 0.05;
        g.add(bar2);
      }
      // 托盘上的箱子
      const cBox = box(0.5, 0.3, 0.4, M(0x607d8b));
      cBox.position.set(0, 0.57, 0);
      g.add(cBox);
      break;
    }
    // ========== 立柱 ==========
    case "pillar": {
      const p = cyl(0.15, 0.18, eq.height, M(0x90a4ae));
      p.position.y = eq.height / 2;
      g.add(p);
      // 底座
      const bp = box(0.5, 0.1, 0.5, M(0x757575));
      bp.position.y = 0.05;
      g.add(bp);
      break;
    }
    // ========== 工作台 ==========
    case "workbench": {
      // 台面
      const top = box(eq.width, 0.06, eq.depth, M(0x8d6e63));
      top.position.y = 0.87;
      g.add(top);
      // 桌腿
      const legMat = M(0x616161);
      for (const [lx, lz] of [
        [-0.9, -0.4],
        [0.9, -0.4],
        [-0.9, 0.4],
        [0.9, 0.4],
      ]) {
        const leg = box(0.05, 0.84, 0.05, legMat);
        leg.position.set(lx, 0.42, lz);
        g.add(leg);
      }
      // 横撑
      const cross = box(eq.width - 0.2, 0.04, 0.04, legMat);
      cross.position.set(0, 0.3, 0);
      g.add(cross);
      // 抽屉
      const drawer = box(0.5, 0.15, 0.3, M(0x78909c));
      drawer.position.set(0, 0.72, 0.2);
      g.add(drawer);
      break;
    }
    // ========== 电气柜 ==========
    case "cabinet": {
      // 柜体
      const body = box(eq.width, eq.height * 0.9, eq.depth, M(0x455a64));
      body.position.y = eq.height * 0.45;
      g.add(body);
      // 门
      const door = box(eq.width * 0.95, eq.height * 0.85, 0.02, M(0x37474f));
      door.position.set(0, eq.height * 0.45, eq.depth / 2 + 0.01);
      g.add(door);
      // 门把手
      const handle = box(0.02, 0.12, 0.03, metal(0xaaa));
      handle.position.set(
        eq.width * 0.35,
        eq.height * 0.5,
        eq.depth / 2 + 0.03,
      );
      g.add(handle);
      // 通风口
      for (let y = eq.height * 0.2; y < eq.height * 0.4; y += 0.06) {
        const vent = box(eq.width * 0.5, 0.02, 0.01, M(0x263238));
        vent.position.set(0, y, eq.depth / 2 + 0.02);
        g.add(vent);
      }
      // 顶部
      const lid = box(eq.width + 0.04, 0.04, eq.depth + 0.04, M(0x546e7a));
      lid.position.y = eq.height * 0.92;
      g.add(lid);
      break;
    }
    // ========== 安全围栏 ==========
    case "safety_fence": {
      const fMat = M(0xfdd835);
      const frameMat = M(0xf9a825);
      // 立柱
      for (const fx of [-eq.width / 2, eq.width / 2]) {
        const post = box(0.04, eq.height, 0.04, frameMat);
        post.position.set(fx, eq.height / 2, 0);
        g.add(post);
      }
      // 横杆
      for (const fy of [0.1, eq.height * 0.5, eq.height - 0.05]) {
        const bar = box(eq.width, 0.025, 0.025, frameMat);
        bar.position.set(0, fy, 0);
        g.add(bar);
      }
      // 网格（横线）
      for (let y = 0.2; y < eq.height - 0.1; y += 0.15) {
        const wire = box(eq.width - 0.1, 0.008, 0.008, fMat);
        wire.position.set(0, y, 0);
        g.add(wire);
      }
      // 网格（竖线）
      for (let x = -eq.width / 2 + 0.15; x < eq.width / 2; x += 0.2) {
        const vwire = box(0.008, eq.height - 0.2, 0.008, fMat);
        vwire.position.set(x, eq.height / 2, 0);
        g.add(vwire);
      }
      break;
    }
    // ========== 自定义运动学机器人 ==========
    default: {
      if (eq.libId.startsWith("kin_")) {
        const base = cyl(0.25, 0.3, 0.2, dark);
        base.position.y = 0.1;
        g.add(base);
        const j1 = cyl(0.18, 0.18, 0.1, M(0xe91e63));
        j1.position.y = 0.25;
        g.add(j1);
        const arm = box(0.08, 0.8, 0.08, M(0xf06292));
        arm.position.set(0, 0.7, 0);
        g.add(arm);
        const wrist = cyl(0.06, 0.06, 0.08, M(0xe91e63));
        wrist.position.set(0, 1.15, 0);
        g.add(wrist);
        const fl = cyl(0.03, 0.03, 0.03, metal(0xaaa));
        fl.position.set(0, 1.22, 0);
        g.add(fl);
      } else {
        // 通用：蓝箱子
        const b = box(eq.width, eq.height * 0.7, eq.depth, M(c));
        b.position.y = eq.height * 0.35;
        g.add(b);
      }
    }
  }

  // 名称标签
  const labelBg = box(eq.width * 0.6, 0.18, 0.01, M(0x212121));
  labelBg.position.set(0, eq.height + 0.15, 0);
  g.add(labelBg);

  g.position.set(eq.posX, eq.posY, eq.posZ);
  g.rotation.y = (eq.rotY * Math.PI) / 180;
  g.scale.setScalar(eq.scale);
  g.userData = { uid: eq.uid };

  eq.mesh = g;
  equipmentGroup!.add(g);
}

function removeEquipment(uid: string) {
  const idx = sceneEquipment.value.findIndex((e) => e.uid === uid);
  if (idx >= 0) {
    const eq = sceneEquipment.value[idx];
    if (eq.mesh && equipmentGroup) {
      equipmentGroup.remove(eq.mesh);
    }
    sceneEquipment.value.splice(idx, 1);
    if (selectedItem.value?.uid === uid) {
      selectedItem.value = null;
    }
  }
}

function selectEquipment(eq: PlacedEquipment) {
  selectedItem.value = eq;
}

function deselectItem() {
  selectedItem.value = null;
}

function updateEquipmentTransform() {
  const eq = selectedItem.value;
  if (!eq || !eq.mesh) return;
  eq.mesh.position.set(eq.posX, eq.posY, eq.posZ);
  eq.mesh.rotation.y = (eq.rotY * Math.PI) / 180;
  eq.mesh.scale.setScalar(eq.scale);
}

// ==================== 视图切换 ====================
function switchView(mode: "exterior" | "interior") {
  viewMode.value = mode;
  if (mode === "interior") {
    cameraDistance = 15;
    cameraTarget.set(0, 3, 0);
    cameraPhi = Math.PI / 3;
  } else {
    cameraDistance = 45;
    cameraTarget.set(0, 3, 0);
    cameraPhi = Math.PI / 4;
  }
  updateCamera();
}

function toggleRoof() {
  showRoof.value = !showRoof.value;
  if (roofGroup) roofGroup.visible = showRoof.value;
}

function toggleWalls() {
  showWalls.value = !showWalls.value;
  if (wallsGroup) wallsGroup.visible = showWalls.value;
}

// ==================== 相机控制 ====================
function updateCamera() {
  if (!camera) return;
  const x =
    cameraTarget.x +
    cameraDistance * Math.cos(cameraPhi) * Math.cos(cameraTheta);
  const y = cameraTarget.y + cameraDistance * Math.sin(cameraPhi);
  const z =
    cameraTarget.z +
    cameraDistance * Math.cos(cameraPhi) * Math.sin(cameraTheta);
  camera.position.set(x, Math.max(1, y), z);
  camera.lookAt(cameraTarget);
}

function onMouseDown(e: MouseEvent) {
  isMouseDown = true;
  isRightMouse = e.button === 2;
  lastMouseX = e.clientX;
  lastMouseY = e.clientY;
}

function onMouseMove(e: MouseEvent) {
  if (!isMouseDown) return;
  const dx = e.clientX - lastMouseX;
  const dy = e.clientY - lastMouseY;
  lastMouseX = e.clientX;
  lastMouseY = e.clientY;

  if (isRightMouse) {
    // 右键平移
    const right = new THREE.Vector3();
    const up = new THREE.Vector3(0, 1, 0);
    camera!.getWorldDirection(right);
    right.cross(up).normalize();
    const panSpeed = cameraDistance * 0.002;
    cameraTarget.add(right.multiplyScalar(-dx * panSpeed));
    cameraTarget.y += dy * panSpeed;
  } else {
    // 左键旋转
    cameraTheta += dx * 0.005;
    cameraPhi = Math.max(
      0.05,
      Math.min(Math.PI / 2 - 0.05, cameraPhi + dy * 0.005),
    );
  }
  updateCamera();
}

function onMouseUp() {
  isMouseDown = false;
}

function onWheel(e: WheelEvent) {
  cameraDistance = Math.max(3, Math.min(100, cameraDistance + e.deltaY * 0.02));
  updateCamera();
}

function resetCamera() {
  cameraTheta = Math.PI / 4;
  cameraPhi = Math.PI / 3;
  cameraDistance = viewMode.value === "interior" ? 15 : 45;
  cameraTarget.set(0, 3, 0);
  updateCamera();
}

function animate() {
  animationId = requestAnimationFrame(animate);
  if (renderer && scene && camera) {
    renderer.render(scene, camera);
  }
}
</script>

<style scoped>
.factory-sim {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  font-family: "Inter", "Segoe UI", sans-serif;
}

/* 工具栏 */
.fs-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 12px;
  background: var(--surface-container);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
  height: 36px;
}
.toolbar-group {
  display: flex;
  align-items: center;
  gap: 4px;
}
.toolbar-title {
  font-size: 12px;
  font-weight: 700;
  color: var(--on-surface);
  white-space: nowrap;
}
.toolbar-spacer {
  flex: 1;
}
.tb-icon {
  font-size: 16px;
  color: var(--primary);
}
.tb-btn {
  display: flex;
  align-items: center;
  gap: 3px;
  padding: 3px 8px;
  background: var(--surface-variant);
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 10px;
  font-family: inherit;
  cursor: pointer;
  transition: all 0.12s;
  white-space: nowrap;
}
.tb-btn:hover {
  background: var(--surface-container-high);
}
.tb-btn.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border-color: var(--primary);
}
.tb-btn .material-symbols-outlined {
  font-size: 13px;
}
.tb-btn-sm {
  padding: 2px 4px;
}
.tb-btn-danger {
  color: var(--error);
  border-color: var(--error);
}
.tb-btn-danger:hover {
  background: rgba(244, 67, 54, 0.1);
}
.tb-separator {
  width: 1px;
  height: 16px;
  background: var(--outline-variant);
  margin: 0 4px;
}
.tb-selection-info {
  font-size: 10px;
  color: var(--secondary);
  font-weight: 600;
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

/* 主体 */
.fs-body {
  flex: 1;
  display: flex;
  overflow: hidden;
}

/* 左侧 */
.fs-sidebar {
  width: 220px;
  flex-shrink: 0;
  background: var(--surface-container-low);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}
.sidebar-section {
  display: flex;
  flex-direction: column;
}
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 10px;
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.04em;
  color: var(--on-surface);
  border-bottom: 1px solid var(--outline-variant);
  background: var(--surface-container);
}
.model-search {
  padding: 6px 8px;
  border-bottom: 1px solid var(--outline-variant);
}
.prop-input {
  flex: 1;
  padding: 3px 6px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: 3px;
  color: var(--on-surface);
  font-size: 10px;
  font-family: "JetBrains Mono", monospace;
  outline: none;
  width: 100%;
  box-sizing: border-box;
}
.prop-input:focus {
  border-color: var(--primary);
}

/* 设备分类 */
.equip-category {
  border-bottom: 1px solid var(--outline-variant);
}
.cat-header {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 5px 8px;
  font-size: 10px;
  font-weight: 600;
  color: var(--on-surface);
  cursor: pointer;
  user-select: none;
  transition: background 0.12s;
}
.cat-header:hover {
  background: var(--surface-variant);
}
.toggle-arrow {
  font-size: 14px;
  color: var(--on-surface-variant);
  transition: transform 0.15s;
}
.cat-count {
  margin-left: auto;
  font-size: 9px;
  color: var(--on-surface-variant);
  background: var(--surface-variant);
  padding: 0 4px;
  border-radius: 8px;
}
.cat-items {
  padding: 2px 0;
}
.model-lib-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 10px;
  cursor: pointer;
  transition: background 0.12s;
}
.model-lib-item:hover {
  background: var(--surface-variant);
}
.model-thumb {
  width: 26px;
  height: 26px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--primary-container);
  border-radius: 4px;
  flex-shrink: 0;
}
.model-thumb .material-symbols-outlined {
  font-size: 15px;
  color: var(--on-primary-container);
}
.model-lib-info {
  flex: 1;
  min-width: 0;
}
.model-lib-name {
  font-size: 10px;
  font-weight: 600;
  color: var(--on-surface);
}
.model-lib-type {
  font-size: 9px;
  color: var(--on-surface-variant);
}

/* 场景设备列表 */
.scene-equip-list {
  max-height: 200px;
  overflow-y: auto;
  padding: 2px 0;
}
.scene-equip-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 3px 8px;
  cursor: pointer;
  transition: background 0.12s;
  font-size: 10px;
  color: var(--on-surface);
}
.scene-equip-item:hover {
  background: var(--surface-variant);
}
.scene-equip-item.selected {
  background: var(--primary-container);
}
.eq-icon {
  font-size: 14px;
  color: var(--secondary);
}
.eq-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.icon-btn-sm {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 16px;
  height: 16px;
  background: none;
  border: none;
  border-radius: 3px;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.icon-btn-sm:hover {
  background: var(--error-container);
  color: var(--error);
}
.icon-btn-sm .material-symbols-outlined {
  font-size: 12px;
}

/* 3D 视口 */
.fs-viewport {
  flex: 1;
  position: relative;
  overflow: hidden;
  background: #87ceeb;
}
.fs-viewport canvas {
  display: block;
  width: 100%;
  height: 100%;
}
.viewport-info {
  position: absolute;
  top: 8px;
  left: 8px;
  padding: 3px 8px;
  background: rgba(0, 0, 0, 0.6);
  border-radius: 4px;
  font-size: 10px;
  color: #ddd;
  pointer-events: none;
}
.info-sep {
  margin: 0 6px;
  opacity: 0.4;
}
.viewport-hint {
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  pointer-events: none;
  color: var(--on-surface-variant);
}
.viewport-hint p {
  font-size: 12px;
  opacity: 0.5;
}

/* 右侧属性面板 */
.fs-props {
  width: 220px;
  flex-shrink: 0;
  background: var(--surface-container-low);
  border-left: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}
.fs-props-empty {
  width: 220px;
}
.props-header {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 10px;
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.04em;
  color: var(--on-surface);
  border-bottom: 1px solid var(--outline-variant);
  background: var(--surface-container);
  flex-shrink: 0;
}
.props-body {
  flex: 1;
  overflow-y: auto;
  padding: 6px 10px;
}
.prop-section {
  margin-bottom: 10px;
}
.prop-section-title {
  font-size: 10px;
  font-weight: 700;
  color: var(--secondary);
  margin-bottom: 4px;
  padding-bottom: 2px;
  border-bottom: 1px solid var(--outline-variant);
}
.prop-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 4px;
}
.prop-row label {
  font-size: 10px;
  color: var(--on-surface-variant);
  min-width: 28px;
  flex-shrink: 0;
}
.prop-row .prop-input {
  width: 0;
}
.full-width {
  width: 100%;
  justify-content: center;
}
.empty-hint {
  text-align: center;
  padding: 20px 10px;
}
.empty-hint p {
  font-size: 11px;
  color: var(--on-surface-variant);
}
.hint-sub {
  font-size: 10px;
  margin-top: 4px;
  opacity: 0.7;
}

/* 滚动条 */
.fs-sidebar::-webkit-scrollbar,
.fs-props::-webkit-scrollbar,
.scene-equip-list::-webkit-scrollbar {
  width: 4px;
}
.fs-sidebar::-webkit-scrollbar-track,
.fs-props::-webkit-scrollbar-track,
.scene-equip-list::-webkit-scrollbar-track {
  background: transparent;
}
.fs-sidebar::-webkit-scrollbar-thumb,
.fs-props::-webkit-scrollbar-thumb,
.scene-equip-list::-webkit-scrollbar-thumb {
  background: #334155;
  border-radius: 2px;
}
</style>
