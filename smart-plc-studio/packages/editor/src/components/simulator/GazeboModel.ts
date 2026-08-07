import * as THREE from "three";
import { loadMesh } from "./UrdfRobot";

export interface GazeboJointInfo {
  name: string;
  type: string;
  axis: THREE.Vector3;
  lower?: number;
  upper?: number;
  childLink: string;
  motion: THREE.Object3D;
}

export interface GazeboJointValue {
  name: string;
  value: number;
  limit?: { lower: number; upper: number };
}

const PX4_BASE = "/models/px4";

function parseVec3(text: string | null | undefined, fallback = new THREE.Vector3()): THREE.Vector3 {
  if (!text) return fallback;
  const parts = text.trim().split(/\s+/).map(Number);
  if (parts.length < 3) return fallback;
  return new THREE.Vector3(parts[0], parts[1], parts[2]);
}

function parsePose(text: string | null | undefined): { position: THREE.Vector3; quaternion: THREE.Quaternion } {
  const out = { position: new THREE.Vector3(), quaternion: new THREE.Quaternion() };
  if (!text) return out;
  const parts = text.trim().split(/\s+/).map(Number);
  if (parts.length < 6) return out;
  out.position.set(parts[0], parts[1], parts[2]);
  // SDF pose rpy: 固定轴 XYZ (roll-pitch-yaw)
  const euler = new THREE.Euler(parts[3], parts[4], parts[5], "ZYX");
  out.quaternion.setFromEuler(euler);
  return out;
}

function resolveUri(uri: string, meshBase: string, modelName: string): string | null {
  if (/^https?:\/\//.test(uri)) return uri;
  if (uri.startsWith("model://")) {
    const rest = uri.slice("model://".length);
    const slash = rest.indexOf("/");
    const pkg = slash >= 0 ? rest.slice(0, slash) : rest;
    const path = slash >= 0 ? rest.slice(slash + 1) : "";
    if (pkg === modelName) return `${meshBase}/${path}`;
    return `${PX4_BASE}/${pkg}/${path}`;
  }
  if (uri.startsWith("file://")) return null;
  if (uri.startsWith("//")) return null;
  return `${meshBase}/${uri}`;
}

function sdfMaterial(visual: Element): { color?: THREE.Color; albedo?: string; script?: string } {
  const mat = visual.querySelector("material");
  if (!mat) return {};
  const diffuse = mat.querySelector("diffuse");
  let color: THREE.Color | undefined;
  if (diffuse) {
    const c = diffuse.textContent?.trim().split(/\s+/).map(Number);
    if (c && c.length >= 3) {
      color = new THREE.Color(Math.min(1, c[0]), Math.min(1, c[1]), Math.min(1, c[2]));
    }
  }
  const albedo = mat.querySelector("albedo_map");
  let albedoUri: string | undefined;
  if (albedo) albedoUri = albedo.textContent?.trim() ?? undefined;
  const scriptName = mat.querySelector("script name");
  return { color, albedo: albedoUri, script: scriptName?.textContent?.trim() ?? undefined };
}

const SCRIPT_COLORS: Record<string, number> = {
  "Gazebo/DarkGrey": 0x555555,
  "Gazebo/Grey": 0x888888,
  "Gazebo/Red": 0xcc3333,
  "Gazebo/Green": 0x33aa33,
  "Gazebo/Blue": 0x3366cc,
  "Gazebo/White": 0xdddddd,
  "Gazebo/Black": 0x222222,
};

export class GazeboModel {
  root = new THREE.Group();
  readonly sdfUrl: string;
  readonly meshBase: string;
  readonly modelName: string;
  links = new Map<string, THREE.Object3D>();
  joints = new Map<string, GazeboJointInfo>();

  constructor(sdfUrl: string, meshBase: string, modelName = "") {
    this.sdfUrl = sdfUrl;
    this.meshBase = meshBase;
    this.modelName = modelName;
  }

  get jointNames(): string[] {
    return [...this.joints.values()]
      .filter((j) => j.type === "revolute" || j.type === "continuous" || j.type === "prismatic" || j.type === "universal")
      .map((j) => j.name);
  }

  getJointValues(): GazeboJointValue[] {
    const out: GazeboJointValue[] = [];
    for (const j of this.joints.values()) {
      if (j.type !== "revolute" && j.type !== "continuous" && j.type !== "prismatic") continue;
      const value = j.motion.userData.value ?? 0;
      out.push({
        name: j.name,
        value,
        limit: j.lower !== undefined && j.upper !== undefined ? { lower: j.lower, upper: j.upper } : undefined,
      });
    }
    return out;
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

  private applyJoint(joint: GazeboJointInfo) {
    const value = joint.motion.userData.value ?? 0;
    if (joint.type === "prismatic") {
      joint.motion.position.copy(joint.axis).multiplyScalar(value);
    } else {
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

  private materialFor(visual: Element): THREE.MeshStandardMaterial {
    const { color, albedo, script } = sdfMaterial(visual);
    const mat = new THREE.MeshStandardMaterial({
      color: color ?? SCRIPT_COLORS[script ?? ""] ?? 0xcccccc,
      metalness: 0.3,
      roughness: 0.55,
    });
    if (albedo) {
      const url = resolveUri(albedo, this.meshBase, this.modelName);
      if (url) {
        new THREE.TextureLoader().load(url, (tex) => {
          mat.map = tex;
          mat.needsUpdate = true;
        });
      }
    }
    return mat;
  }

  /**
   * 标记需要对网格做 Z-up → Y-up 补偿的类别
   * 当 root 会被旋转时，这些网格已经是 Y-up 了，需要反向补偿
   */
  meshCompensationAngle = 0;

  private buildGeometry(geoEl: Element, visual: Element): THREE.Object3D | null {
    const meshEl = geoEl.querySelector("mesh");
    if (meshEl) {
      const uri = meshEl.querySelector("uri")?.textContent?.trim();
      if (!uri) return null;
      const url = resolveUri(uri, this.meshBase, this.modelName);
      if (!url) return null;
      const scale = parseVec3(meshEl.querySelector("scale")?.textContent, new THREE.Vector3(1, 1, 1));
      const holder = new THREE.Group();
      holder.userData.pendingMesh = true;
      loadMesh(url, true)
        .then((group) => {
          const clone = group.clone(true);
          clone.traverse((child) => {
            const mesh = child as THREE.Mesh;
            if (mesh.isMesh) {
              mesh.scale.multiply(scale);
              mesh.material = this.materialFor(visual);
              // 对网格做 Z-up→Y-up 旋转补偿
              if (this.meshCompensationAngle !== 0) {
                mesh.rotateX(this.meshCompensationAngle);
              }
            }
          });
          holder.add(clone);
          holder.userData.pendingMesh = false;
        })
        .catch((err) => {
          console.warn(`网格加载失败 ${url}:`, err);
          holder.userData.pendingMesh = false;
        });
      return holder;
    }
    const box = geoEl.querySelector("box");
    if (box) {
      const size = parseVec3(box.querySelector("size")?.textContent, new THREE.Vector3(1, 1, 1));
      if (size.length() > 1e-9) {
        return new THREE.Mesh(new THREE.BoxGeometry(size.x, size.y, size.z), this.materialFor(visual));
      }
    }
    const cyl = geoEl.querySelector("cylinder");
    if (cyl) {
      const r = parseFloat(cyl.querySelector("radius")?.textContent ?? "");
      const len = parseFloat(cyl.querySelector("length")?.textContent ?? "");
      if (r > 0 && len > 0) {
        const mesh = new THREE.Mesh(new THREE.CylinderGeometry(r, r, len, 24), this.materialFor(visual));
        mesh.rotation.x = Math.PI / 2;
        return mesh;
      }
    }
    const sphere = geoEl.querySelector("sphere");
    if (sphere) {
      const r = parseFloat(sphere.querySelector("radius")?.textContent ?? "");
      if (r > 0) return new THREE.Mesh(new THREE.SphereGeometry(r, 24, 16), this.materialFor(visual));
    }
    const plane = geoEl.querySelector("plane");
    if (plane) {
      const size = parseVec3(plane.querySelector("size")?.textContent, new THREE.Vector3(1, 1, 1));
      const normal = parseVec3(plane.querySelector("normal")?.textContent, new THREE.Vector3(0, 0, 1));
      const mesh = new THREE.Mesh(new THREE.PlaneGeometry(size.x, size.y), this.materialFor(visual));
      if (Math.abs(normal.z) < 0.999) {
        mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), normal.clone().normalize());
      }
      return mesh;
    }
    return null;
  }

  async load(): Promise<GazeboModel> {
    const res = await fetch(this.sdfUrl);
    if (!res.ok) throw new Error(`加载 SDF 失败: ${this.sdfUrl} (${res.status})`);
    const xml = await res.text();
    const doc = new DOMParser().parseFromString(xml, "text/xml");
    const model = doc.querySelector("model");
    if (!model) throw new Error("SDF 中找不到 <model> 节点");

    const modelPose = parsePose(model.querySelector(":scope > pose")?.textContent ?? model.querySelector("pose")?.textContent);
    this.root.position.copy(modelPose.position);
    this.root.quaternion.copy(modelPose.quaternion);

    model.querySelectorAll(":scope > link").forEach((linkEl) => {
      const name = linkEl.getAttribute("name") ?? "";
      const link = new THREE.Object3D();
      link.name = name;
      const linkPose = parsePose(linkEl.querySelector(":scope > pose")?.textContent ?? linkEl.querySelector("pose")?.textContent);
      link.position.copy(linkPose.position);
      link.quaternion.copy(linkPose.quaternion);
      linkEl.querySelectorAll(":scope > visual").forEach((visual) => {
        const geoEl = visual.querySelector("geometry");
        if (!geoEl) return;
        const geo = this.buildGeometry(geoEl, visual);
        if (!geo) return;
        const origin = new THREE.Object3D();
        const visPose = parsePose(visual.querySelector(":scope > pose")?.textContent ?? visual.querySelector("pose")?.textContent);
        origin.position.copy(visPose.position);
        origin.quaternion.copy(visPose.quaternion);
        origin.add(geo);
        link.add(origin);
      });
      this.links.set(name, link);
    });

    const parentedLinks = new Set<string>();
    let skipped = 0;
    model.querySelectorAll(":scope > joint").forEach((jointEl) => {
      const name = jointEl.getAttribute("name") ?? "";
      const type = jointEl.getAttribute("type") ?? "fixed";
      const parentName = jointEl.querySelector("parent")?.textContent?.trim();
      const childName = jointEl.querySelector("child")?.textContent?.trim();
      const parent = parentName ? this.links.get(parentName) : undefined;
      const child = childName ? this.links.get(childName) : undefined;
      if (!parent || !child || !childName) {
        skipped++;
        return;
      }
      parentedLinks.add(childName);

      const joint: GazeboJointInfo = {
        name,
        type,
        childLink: childName,
        axis: parseVec3(jointEl.querySelector("axis xyz")?.textContent, new THREE.Vector3(0, 0, 1)),
        motion: new THREE.Object3D(),
      };
      const lower = parseFloat(jointEl.querySelector("axis limit lower")?.textContent ?? "NaN");
      const upper = parseFloat(jointEl.querySelector("axis limit upper")?.textContent ?? "NaN");
      if (!Number.isNaN(lower)) joint.lower = lower;
      if (!Number.isNaN(upper)) joint.upper = upper;
      joint.motion.userData.value = 0;

      const jointObj = new THREE.Object3D();
      jointObj.name = name;
      const jPose = parsePose(jointEl.querySelector(":scope > pose")?.textContent ?? jointEl.querySelector("pose")?.textContent);
      jointObj.position.copy(jPose.position);
      jointObj.quaternion.copy(jPose.quaternion);

      jointObj.add(joint.motion);
      joint.motion.add(child);
      parent.add(jointObj);

      if (type === "revolute" || type === "continuous" || type === "prismatic" || type === "universal") {
        this.joints.set(name, joint);
      }
    });

    console.log(`[GazeboModel] ${this.sdfUrl}: links=${this.links.size}, joints=${this.joints.size}, skipped=${skipped}, jointNames=[${[...this.joints.keys()].join(", ")}]`);

    for (const link of this.links.values()) {
      if (!parentedLinks.has(link.name)) this.root.add(link);
    }
    this.applyCurrentState();
    await this.waitForMeshes();
    this.applyCurrentState();
    return this;
  }

  private async waitForMeshes(timeoutMs = 20000): Promise<void> {
    const deadline = Date.now() + timeoutMs;
    const root = this.root;
    while (Date.now() < deadline) {
      let pending = false;
      root.traverse((obj) => {
        if (obj.userData.pendingMesh) pending = true;
      });
      if (!pending) return;
      await new Promise((r) => setTimeout(r, 100));
    }
  }

  /**
   * 修正网格朝向: 对所有 mesh geometry 应用额外旋转
   * 当 root 已经被旋转 (Z-up → Y-up) 时, Y-up 网格需要反向旋转补偿
   */
  compensateMeshRotation(radiansX: number) {
    if (Math.abs(radiansX) < 1e-9) return;
    this.root.traverse((obj) => {
      if ((obj as THREE.Mesh).isMesh) {
        // 补偿网格的初始旋转
        const mesh = obj as THREE.Mesh;
        mesh.rotateX(radiansX);
      }
    });
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
