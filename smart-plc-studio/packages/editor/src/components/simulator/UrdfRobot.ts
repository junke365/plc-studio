import * as THREE from "three";
import { ColladaLoader } from "three/examples/jsm/loaders/ColladaLoader";
import { STLLoader } from "three/examples/jsm/loaders/STLLoader";
import { OBJLoader } from "three/examples/jsm/loaders/OBJLoader";

export interface UrdfJointInfo {
  name: string;
  type: "revolute" | "continuous" | "prismatic" | "fixed" | "floating" | "planar";
  axis: THREE.Vector3;
  lower?: number;
  upper?: number;
  mimic?: { joint: string; multiplier: number; offset: number };
  childLink: string;
  motion: THREE.Object3D;
}

export interface UrdfJointValue {
  name: string;
  value: number;
  limit?: { lower: number; upper: number };
}

const meshCache = new Map<string, Promise<THREE.Object3D>>();

function parseVec3(text: string | null | undefined, fallback = new THREE.Vector3()): THREE.Vector3 {
  if (!text) return fallback;
  const parts = text.trim().split(/\s+/).map(Number);
  if (parts.length < 3) return fallback;
  return new THREE.Vector3(parts[0], parts[1], parts[2]);
}

function rpyToQuat(text: string | null | undefined): THREE.Quaternion {
  if (!text) return new THREE.Quaternion();
  const parts = text.trim().split(/\s+/).map(Number);
  if (parts.length < 3) return new THREE.Quaternion();
  // URDF rpy 是外旋(fixed-axis) XYZ = Three.js 内旋 ZYX
  const euler = new THREE.Euler(parts[0], parts[1], parts[2], "ZYX");
  return new THREE.Quaternion().setFromEuler(euler);
}

export function loadMesh(url: string, zUpWorld = false): Promise<THREE.Object3D> {
  const cacheKey = zUpWorld ? `${url}#zup` : url;
  let cached = meshCache.get(cacheKey);
  if (cached) return cached;
  // 用 fetch + 重试替代 THREE Loader.load，避免浏览器并发限制导致的 ERR_ABORTED
  const p = loadMeshWithRetry(url, 3, zUpWorld);
  meshCache.set(cacheKey, p);
  return p;
}

async function loadMeshWithRetry(url: string, retries: number, zUpWorld: boolean): Promise<THREE.Object3D> {
  const lower = url.toLowerCase();
  for (let attempt = 0; attempt <= retries; attempt++) {
    try {
      const res = await fetch(url);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const buf = await res.arrayBuffer();
      if (lower.endsWith(".dae")) {
        const loader = new ColladaLoader();
        const text = new TextDecoder().decode(buf);
        const collada = loader.parse(text, url);
        if (zUpWorld) {
          // URDF 链接坐标系为 Z-up；ColladaLoader 已把 Z_UP 自动转成 Y-up。
          // 这里把网格转回链接系的 Z-up，再由外层 root 做 URDF(Z-up)->three(Y-up)。
          const zUp = /<up_axis>\s*Z_UP/i.test(text);
          collada.scene.rotation.set(zUp ? Math.PI / 2 : -Math.PI / 2, 0, 0);
        }
        return collada.scene;
      } else if (lower.endsWith(".stl")) {
        const loader = new STLLoader();
        const geometry = loader.parse(buf);
        geometry.computeVertexNormals();
        const mat = new THREE.MeshStandardMaterial({
          color: 0xbbbbcc,
          metalness: 0.4,
          roughness: 0.4,
        });
        const mesh = new THREE.Mesh(geometry, mat);
        const group = new THREE.Group();
        group.add(mesh);
        return group;
      } else if (lower.endsWith(".obj")) {
        const loader = new OBJLoader();
        const text = new TextDecoder().decode(buf);
        return loader.parse(text);
      }
      throw new Error(`不支持的网格格式: ${url}`);
    } catch (err) {
      if (attempt === retries) throw err;
      // 指数退避: 100ms, 200ms, 400ms...
      await new Promise((r) => setTimeout(r, 100 * Math.pow(2, attempt)));
    }
  }
  throw new Error(`unreachable: ${url}`);
}

let pkgMapCache: Map<string, string> | null = null;
let pkgMapStamp = 0;

async function packageDirUrl(pkgName: string): Promise<string | null> {
  const now = Date.now();
  if (!pkgMapCache || now - pkgMapStamp > 60000) {
    try {
      const res = await fetch("/ros-packages.json");
      if (res.ok) {
        pkgMapCache = new Map(Object.entries((await res.json()) as Record<string, string>));
        pkgMapStamp = now;
      }
    } catch {
      /* ignore */
    }
  }
  return pkgMapCache?.get(pkgName) ?? null;
}

async function resolveMeshUrlAsync(filename: string, meshBase: string, rosBase: string): Promise<string> {
  if (filename.startsWith("/") || /^https?:\/\//.test(filename)) return filename;
  if (filename.startsWith("package://")) {
    const rest = filename.slice("package://".length);
    const slash = rest.indexOf("/");
    const pkg = slash >= 0 ? rest.slice(0, slash) : rest;
    const path = slash >= 0 ? rest.slice(slash + 1) : "";
    if (rosBase) {
      const dir = await packageDirUrl(pkg);
      if (dir) return `${dir}/${path}`;
    }
    return `${meshBase}/${path}`;
  }
  return `${meshBase}/${filename}`;
}

export class UrdfRobot {
  root = new THREE.Group();
  readonly urdfUrl: string;
  readonly meshBase: string;
  readonly rosBase: string;
  readonly zUpWorld: boolean;
  links = new Map<string, THREE.Object3D>();
  joints = new Map<string, UrdfJointInfo>();

  constructor(urdfUrl: string, meshBase = "", rosBase = "", zUpWorld = false) {
    this.urdfUrl = urdfUrl;
    this.meshBase = meshBase;
    this.rosBase = rosBase;
    this.zUpWorld = zUpWorld;
  }

  get jointNames(): string[] {
    return [...this.joints.values()]
      .filter((j) => j.type === "revolute" || j.type === "continuous" || j.type === "prismatic")
      .map((j) => j.name);
  }

  getJointValues(): UrdfJointValue[] {
    const out: UrdfJointValue[] = [];
    for (const j of this.joints.values()) {
      if (j.type !== "revolute" && j.type !== "continuous" && j.type !== "prismatic") continue;
      if (j.mimic) continue;
      const value = this.effectiveValue(j);
      out.push({
        name: j.name,
        value,
        limit: j.lower !== undefined && j.upper !== undefined ? { lower: j.lower, upper: j.upper } : undefined,
      });
    }
    return out;
  }

  private effectiveValue(joint: UrdfJointInfo, visiting = new Set<string>()): number {
    if (joint.mimic) {
      if (visiting.has(joint.name)) return 0;
      visiting.add(joint.name);
      const src = this.joints.get(joint.mimic.joint);
      if (src) {
        return this.effectiveValue(src, visiting) * joint.mimic.multiplier + joint.mimic.offset;
      }
    }
    return joint.motion.userData.value ?? 0;
  }

  setJointValue(name: string, radians: number) {
    const joint = this.joints.get(name);
    if (!joint) return;
    let v = radians;
    if (joint.lower !== undefined && joint.upper !== undefined) {
      v = Math.max(joint.lower, Math.min(joint.upper, v));
    }
    joint.motion.userData.value = v;
    this.applyCurrentState();
  }

  setJointValues(values: Record<string, number>) {
    for (const [name, value] of Object.entries(values)) this.setJointValue(name, value);
  }

  private applyJoint(joint: UrdfJointInfo) {
    const value = this.effectiveValue(joint);
    if (joint.type === "prismatic") {
      joint.motion.position.copy(joint.axis).multiplyScalar(value);
    } else if (joint.type === "revolute" || joint.type === "continuous") {
      joint.motion.quaternion.setFromAxisAngle(joint.axis, value);
    }
  }

  applyCurrentState() {
    for (const j of this.joints.values()) this.applyJoint(j);
  }

  reset() {
    for (const j of this.joints.values()) j.motion.userData.value = 0;
    this.applyCurrentState();
  }

  getLinkPose(name: string): { position: THREE.Vector3; quaternion: THREE.Quaternion } | null {
    const link = this.links.get(name);
    if (!link) return null;
    const matrix = new THREE.Matrix4();
    link.updateWorldMatrix(true, false);
    matrix.copy(link.matrixWorld);
    const position = new THREE.Vector3().setFromMatrixPosition(matrix);
    const quaternion = new THREE.Quaternion().setFromRotationMatrix(matrix);
    return { position, quaternion };
  }

  async load(): Promise<UrdfRobot> {
    const res = await fetch(this.urdfUrl);
    if (!res.ok) throw new Error(`加载 URDF 失败: ${this.urdfUrl} (${res.status})`);
    const xml = await res.text();
    const doc = new DOMParser().parseFromString(xml, "text/xml");

    // 收集所有 visual 网格的加载 Promise，最后统一 await
    // (不再二次 fetch URDF，避免 ERR_ABORTED 与重复加载 collision .stl)
    const pendingMeshes: Promise<unknown>[] = [];

    const materialMap = new Map<string, THREE.MeshStandardMaterial>();
    doc.querySelectorAll("material[name]").forEach((el) => {
      const colorEl = el.querySelector("color");
      const name = el.getAttribute("name")!;
      if (!colorEl) return;
      const rgba = (colorEl.getAttribute("rgba") || "1 1 1 1").trim().split(/\s+/).map(Number);
      materialMap.set(name, new THREE.MeshStandardMaterial({
        color: new THREE.Color(rgba[0], rgba[1], rgba[2]),
        transparent: rgba.length > 3 && rgba[3] < 1,
        opacity: rgba[3] ?? 1,
      }));
    });

    doc.querySelectorAll("link").forEach((linkEl) => {
      const name = linkEl.getAttribute("name")!;
      const linkObj = new THREE.Object3D();
      linkObj.name = name;
      const visualEls = linkEl.querySelectorAll("visual");
      if (visualEls.length === 0 && !linkEl.querySelector("collision")) {
        this.links.set(name, linkObj);
        return;
      }
      visualEls.forEach((visual) => {
        const geoEl = visual.querySelector("geometry");
        if (!geoEl) return;
        const origin = new THREE.Object3D();
        origin.position.copy(parseVec3(visual.querySelector("origin")?.getAttribute("xyz")));
        origin.quaternion.copy(rpyToQuat(visual.querySelector("origin")?.getAttribute("rpy")));

        const meshEl = geoEl.querySelector("mesh");
        if (meshEl) {
          const filename = meshEl.getAttribute("filename");
          if (filename) {
            const scale = parseVec3(meshEl.getAttribute("scale"), new THREE.Vector3(1, 1, 1));
            const p = resolveMeshUrlAsync(filename, this.meshBase, this.rosBase)
              .then((url) => loadMesh(url, this.zUpWorld))
              .then((group) => {
                const clone = group.clone(true);
                clone.traverse((child) => {
                  if ((child as THREE.Mesh).isMesh) {
                    child.scale.multiply(scale);
                  }
                });
                origin.add(clone);
              })
              .catch((err) => console.warn(`网格加载失败 ${filename}:`, err));
            pendingMeshes.push(p);
          }
        } else {
          const box = geoEl.querySelector("box");
          if (box) {
            const size = parseVec3(box.getAttribute("size"), new THREE.Vector3(1, 1, 1));
            if (size.length() > 1e-9) {
              origin.add(new THREE.Mesh(
                new THREE.BoxGeometry(size.x, size.y, size.z),
                this.materialFor(visual, materialMap),
              ));
            }
          }
          const cyl = geoEl.querySelector("cylinder");
          if (cyl) {
            const r = parseFloat(cyl.getAttribute("radius") || "0");
            const len = parseFloat(cyl.getAttribute("length") || "0");
            if (r > 0 && len > 0) {
              const mesh = new THREE.Mesh(
                new THREE.CylinderGeometry(r, r, len, 24),
                this.materialFor(visual, materialMap),
              );
              mesh.rotation.x = Math.PI / 2;
              origin.add(mesh);
            }
          }
          const sphere = geoEl.querySelector("sphere");
          if (sphere) {
            const r = parseFloat(sphere.getAttribute("radius") || "0");
            if (r > 0) {
              origin.add(new THREE.Mesh(
                new THREE.SphereGeometry(r, 24, 16),
                this.materialFor(visual, materialMap),
              ));
            }
          }
        }
        linkObj.add(origin);
      });
      this.links.set(name, linkObj);
    });

    const parentedLinks = new Set<string>();
    let skipped = 0;
    doc.querySelectorAll("joint").forEach((jointEl) => {
      const name = jointEl.getAttribute("name")!;
      const type = jointEl.getAttribute("type")! as UrdfJointInfo["type"];
      const parentName = jointEl.querySelector("parent")?.getAttribute("link");
      const childName = jointEl.querySelector("child")?.getAttribute("link");
      const parent = parentName ? this.links.get(parentName) : undefined;
      const child = childName ? this.links.get(childName) : undefined;
      if (!parent || !child || !childName) {
        skipped++;
        return;
      }
      parentedLinks.add(childName);

      const joint = {} as UrdfJointInfo;
      joint.name = name;
      joint.type = type;
      joint.childLink = childName!;
      joint.axis = parseVec3(jointEl.querySelector("axis")?.getAttribute("xyz"), new THREE.Vector3(0, 0, 1));
      const limitEl = jointEl.querySelector("limit");
      if (limitEl) {
        const lower = parseFloat(limitEl.getAttribute("lower") || "NaN");
        const upper = parseFloat(limitEl.getAttribute("upper") || "NaN");
        if (!Number.isNaN(lower)) joint.lower = lower;
        if (!Number.isNaN(upper)) joint.upper = upper;
      }
      const mimicEl = jointEl.querySelector("mimic");
      if (mimicEl) {
        const mJoint = mimicEl.getAttribute("joint");
        if (mJoint) {
          joint.mimic = {
            joint: mJoint,
            multiplier: parseFloat(mimicEl.getAttribute("multiplier") || "1"),
            offset: parseFloat(mimicEl.getAttribute("offset") || "0"),
          };
        }
      }

      const jointObj = new THREE.Object3D();
      jointObj.name = name;
      jointObj.position.copy(parseVec3(jointEl.querySelector("origin")?.getAttribute("xyz")));
      jointObj.quaternion.copy(rpyToQuat(jointEl.querySelector("origin")?.getAttribute("rpy")));

      const motion = new THREE.Object3D();
      motion.userData.value = 0;
      joint.motion = motion;

      jointObj.add(motion);
      motion.add(child);
      parent.add(jointObj);

      if (type === "revolute" || type === "continuous" || type === "prismatic") {
        this.joints.set(name, joint);
      }
    });

    console.log(`[UrdfRobot] ${this.urdfUrl}: links=${this.links.size}, joints=${this.joints.size}, skipped=${skipped}, jointNames=[${[...this.joints.keys()].join(", ")}]`);

    for (const link of this.links.values()) {
      if (!parentedLinks.has(link.name)) this.root.add(link);
    }
    this.applyCurrentState();
    await Promise.all(pendingMeshes);
    this.applyCurrentState();
    return this;
  }

  private materialFor(
    visual: Element,
    materialMap: Map<string, THREE.MeshStandardMaterial>,
  ): THREE.Material {
    const name = visual.querySelector("material")?.getAttribute("name");
    if (name && materialMap.has(name)) return materialMap.get(name)!;
    return new THREE.MeshStandardMaterial({ color: 0xcccccc, metalness: 0.4, roughness: 0.5 });
  }

  dispose() {
    this.root.traverse((child) => {
      if ((child as THREE.Mesh).isMesh) {
        const mesh = child as THREE.Mesh;
        mesh.geometry?.dispose();
        const mat = mesh.material as THREE.Material | THREE.Material[];
        if (Array.isArray(mat)) mat.forEach((m) => m.dispose());
        else mat?.dispose();
      }
    });
  }
}
