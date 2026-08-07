import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader";

export type OrganId = "liver" | "kidney" | "brain";

interface PartSpec {
  id: string;
  url: string;
  relScale?: number;
  relPos?: [number, number, number];
  opacity?: number;
  color?: number;
}

export interface OrganConfig {
  id: OrganId;
  name: string;
  displaySize: number;
  texture?: string;
  mainUrl: string;
  parts: PartSpec[];
  grabRadius?: number;
  cutRadius?: number;
  partsOpacity?: number;
}

export const ORGAN_CONFIGS: Record<OrganId, OrganConfig> = {
  liver: {
    id: "liver",
    name: "肝脏 (Liver)",
    displaySize: 0.17,
    texture: "/models/surgery/liver-texture.png",
    mainUrl: "/models/surgery/liver_main.glb",
    grabRadius: 0.011,
    cutRadius: 0.008,
    partsOpacity: 0.5,
    parts: [
      { id: "hepatic_vein", url: "/models/surgery/liver_hepatic_vein.glb", relScale: 0.62, opacity: 0.5, color: 0x9a5a7a },
      { id: "portal_vein", url: "/models/surgery/liver_portal_vein.glb", relScale: 0.62, opacity: 0.5, color: 0x5a6a9a },
      { id: "tumor", url: "/models/surgery/liver_tumor.glb", relScale: 0.11, relPos: [0.16, 0.24, 0.1], color: 0xc07a3a },
    ],
  },
  kidney: {
    id: "kidney",
    name: "肾脏 (Kidney)",
    displaySize: 0.13,
    texture: "/models/surgery/kidney-texture.png",
    mainUrl: "/models/surgery/kidney_main.glb",
    grabRadius: 0.01,
    cutRadius: 0.007,
    partsOpacity: 0.6,
    parts: [
      { id: "vessels", url: "/models/surgery/kidney_vessels.glb", relScale: 0.55, opacity: 0.6, color: 0x9a5a3a },
      { id: "tumor", url: "/models/surgery/kidney_tumor.glb", relScale: 0.1, relPos: [0.14, 0.28, 0.05], color: 0xc07a3a },
    ],
  },
  brain: {
    id: "brain",
    name: "大脑 (Brain)",
    displaySize: 0.15,
    texture: "/models/surgery/brain-texture.png",
    mainUrl: "/models/surgery/brain_main.glb",
    grabRadius: 0.012,
    cutRadius: 0.009,
    partsOpacity: 1,
    parts: [],
  },
};

export interface OrganHandle {
  config: OrganConfig;
  group: THREE.Group;
  mainMesh: THREE.Mesh;
  parts: { id: string; mesh: THREE.Mesh; group: THREE.Group }[];
  dispose(): void;
}

const loader = new GLTFLoader();

function loadGLB(url: string): Promise<THREE.Group> {
  return new Promise((resolve, reject) => {
    loader.load(url, (gltf) => resolve(gltf.scene), undefined, reject);
  });
}

function ensureGeometry(mesh: THREE.Mesh) {
  let geo = mesh.geometry as THREE.BufferGeometry;
  if (!geo) {
    geo = new THREE.BufferGeometry();
    mesh.geometry = geo;
  }
  if (!(geo as THREE.BufferGeometry).getAttribute("position")) {
    throw new Error(`mesh without position: ${mesh.name}`);
  }
  return geo;
}

export async function loadOrgan(config: OrganConfig, textureLoader?: THREE.TextureLoader): Promise<OrganHandle> {
  const group = new THREE.Group();

  const mainScene = await loadGLB(config.mainUrl);
  const mainMesh = mainScene.getObjectByProperty("isMesh", true) as THREE.Mesh;
  const mainGeo = ensureGeometry(mainMesh);
  const mainMat = new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.72,
    metalness: 0.0,
  });
  if (config.texture && textureLoader) {
    const tex = textureLoader.load(config.texture);
    tex.colorSpace = THREE.SRGBColorSpace;
    tex.anisotropy = 4;
    mainMat.map = tex;
  }
  mainMat.vertexColors = true;
  mainMat.needsUpdate = true;
  mainGeo.computeVertexNormals();
  mainMesh.material = mainMat;
  mainMesh.castShadow = true;
  mainMesh.receiveShadow = true;
  group.add(mainMesh);

  const parts: OrganHandle["parts"] = [];
  for (const spec of config.parts) {
    try {
      const partScene = await loadGLB(spec.url);
      const partMesh = partScene.getObjectByProperty("isMesh", true) as THREE.Mesh;
      const partGeo = ensureGeometry(partMesh);
      partGeo.computeVertexNormals();
      const mat = new THREE.MeshStandardMaterial({
        color: spec.color ?? 0xffffff,
        roughness: 0.6,
        metalness: 0.1,
        transparent: true,
        opacity: spec.opacity ?? 1,
      });
      partMesh.material = mat;
      partMesh.castShadow = true;

      const partBBox = new THREE.Box3().setFromObject(partMesh);
      const size = new THREE.Vector3();
      partBBox.getSize(size);
      const scale = spec.relScale ?? 0.5;
      partMesh.scale.setScalar(scale / Math.max(size.x, size.y, size.z));

      const mainBBox = new THREE.Box3().setFromObject(mainMesh);
      const center = new THREE.Vector3();
      mainBBox.getCenter(center);
      const mainSize = new THREE.Vector3();
      mainBBox.getSize(mainSize);
      const pos = spec.relPos ?? [0, 0, 0];
      partMesh.position.set(
        center.x + pos[0] * mainSize.x,
        center.y + pos[1] * mainSize.y,
        center.z + pos[2] * mainSize.z
      );

      const partGroup = new THREE.Group();
      partGroup.add(partMesh);
      group.add(partGroup);
      parts.push({ id: spec.id, mesh: partMesh, group: partGroup });
    } catch (e) {
      console.warn(`part ${spec.id} 加载失败:`, e);
    }
  }

  const handle: OrganHandle = {
    config,
    group,
    mainMesh,
    parts,
    dispose() {
      mainGeo.dispose();
      if (mainMat.map) mainMat.map.dispose();
      mainMat.dispose();
      group.traverse((o) => {
        if ((o as THREE.Mesh).isMesh) {
          (o as THREE.Mesh).geometry?.dispose();
          const m = (o as THREE.Mesh).material as THREE.Material | THREE.Material[];
          if (Array.isArray(m)) m.forEach((mm) => mm.dispose());
          else m?.dispose();
        }
      });
    },
  };

  handle.group.scale.setScalar(config.displaySize);
  return handle;
}
