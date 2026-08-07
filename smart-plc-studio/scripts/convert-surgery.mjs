/* 离线转换 DejaVu OBJ → 减面 → 单位化 → GLB
 * 用法: node scripts/convert-surgery.mjs
 * 输出: packages/editor/public/models/surgery/*.glb
 * 纹理不嵌入 GLB(浏览器运行时单独加载 PNG)。
 */
import { readFileSync } from "fs";
import { mkdir, copyFile } from "fs/promises";
import { dirname, join, basename } from "path";
import { fileURLToPath } from "url";
import * as THREE from "three";
import { OBJLoader } from "three/examples/jsm/loaders/OBJLoader.js";
import { SimplifyModifier } from "three/examples/jsm/modifiers/SimplifyModifier.js";
import { GLTFExporter } from "three/examples/jsm/exporters/GLTFExporter.js";

const __dirname = dirname(fileURLToPath(import.meta.url));
const REPO = join(__dirname, "..");

if (!globalThis.FileReader) {
  globalThis.FileReader = class {
    result = null;
    onloadend = null;
    readAsArrayBuffer(blob) {
      blob.arrayBuffer().then((buf) => {
        this.result = buf;
        this.onloadend?.({ target: this });
      });
    }
  };
}
const _warn = console.warn;
console.warn = (...args) => {
  if (typeof args[0] === "string" && args[0].startsWith("THREE.OBJLoader: Unexpected line")) return;
  _warn(...args);
};
const SOURCE = "/Users/junjun/ros/DejaVu";
const OUT = join(REPO, "packages/editor/public/models/surgery");

const loader = new OBJLoader();
const simplifier = new SimplifyModifier();

async function toGLB(mesh, outFile) {
  const exporter = new GLTFExporter();
  const bin = await exporter.parseAsync(mesh, { binary: true });
  const file = join(OUT, outFile);
  const { writeFile } = await import("fs/promises");
  await writeFile(file, Buffer.from(bin));
  console.log("  →", outFile, `${(bin.byteLength / 1024).toFixed(0)} KB`);
}

function normalize(mesh, targetUnit = 1.0) {
  const geo = mesh.geometry;
  geo.computeBoundingBox();
  const bb = geo.boundingBox;
  const size = new THREE.Vector3();
  bb.getSize(size);
  const longest = Math.max(size.x, size.y, size.z) || 1e-6;
  const k = targetUnit / longest;
  geo.scale(k, k, k);
  geo.computeBoundingBox();
  bb.copy(geo.boundingBox);
  const center = bb.getCenter(new THREE.Vector3());
  geo.translate(-center.x, -center.y, -center.z);
  mesh.geometry = geo;
  return mesh;
}

function decimate(mesh, targetVerts) {
  const geo = mesh.geometry;
  if (geo.attributes.position.count <= targetVerts) return mesh;
  const g = simplifier.modify(geo, targetVerts);
  g.computeVertexNormals();
  mesh.geometry = g;
  return mesh;
}

async function loadObj(path) {
  const text = readFileSync(join(SOURCE, path), "utf8");
  return loader.parse(text);
}

async function convertPart({ src, out, targetVerts, color = null, materialType = "standard" }) {
  const obj = await loadObj(src);
  let mesh;
  if (obj.children.length) {
    mesh = new THREE.Group();
    obj.children.forEach((c) => {
      const m = normalize(decimate(c, targetVerts ?? Infinity));
      mesh.add(m);
    });
  } else {
    mesh = normalize(decimate(obj, targetVerts ?? Infinity));
  }
  const mat = new THREE.MeshStandardMaterial({
    color: color ?? 0xffffff,
    roughness: 0.7,
    metalness: 0.05,
  });
  mesh.traverse((o) => {
    if (o.isMesh) {
      o.material = mat;
      o.castShadow = true;
    }
  });
  await toGLB(mesh, out);
}

async function main() {
  await mkdir(OUT, { recursive: true });

  if (process.argv.includes("--report")) {
    const mains = [
      ["liver", "scenes/liver/data/parenchyma_uv.obj"],
      ["kidney", "scenes/kidney/data/kidney_pig_surface_smooth.obj"],
      ["brain", "scenes/brain/data/volume_simplified.obj"],
    ];
    console.log("SOFA 场景坐标(cm) → GLB 归一化坐标 变换:  glb = v*100*k - center*k");
    console.log("center 为 OBJ 原始(mm)包围盒中心, maxDim 为最长边(mm), k = 1/maxDim");
    const report = {};
    for (const [id, path] of mains) {
      const obj = await loadObj(path);
      const mesh = obj.children.length ? obj.children[0] : obj;
      const geo = mesh.geometry;
      geo.computeBoundingBox();
      const size = new THREE.Vector3();
      geo.boundingBox.getSize(size);
      const maxDim = Math.max(size.x, size.y, size.z) || 1e-6;
      const k = 1 / maxDim;
      const center = geo.boundingBox.getCenter(new THREE.Vector3());
      report[id] = {
        centerMm: [center.x, center.y, center.z],
        maxDimMm: maxDim,
        scaleCmToGlb: 100 * k,
        offsetCmToGlb: [-center.x * k, -center.y * k, -center.z * k],
      };
      console.log(id, JSON.stringify(report[id]));
    }
    console.log("REPORT_JSON=" + JSON.stringify(report));
    return;
  }

  console.log("== 肝脏 liver ==");
  await convertPart({ src: "scenes/liver/data/parenchyma_uv.obj", out: "liver_main.glb" });
  await convertPart({ src: "scenes/liver/data/tumor.obj", out: "liver_tumor.glb", color: 0xb0793a });
  await convertPart({ src: "scenes/liver/data/hepatic_vein.obj", out: "liver_hepatic_vein.glb", targetVerts: 9000, color: 0x8a4a6a });
  await convertPart({ src: "scenes/liver/data/portal_vein.obj", out: "liver_portal_vein.glb", targetVerts: 9000, color: 0x4a5a8a });

  console.log("== 肾脏 kidney ==");
  await convertPart({ src: "scenes/kidney/data/kidney_pig_surface_smooth.obj", out: "kidney_main.glb" });
  await convertPart({ src: "scenes/kidney/data/kidney_pig_vessels_smooth.obj", out: "kidney_vessels.glb", color: 0x9a5a3a });
  await convertPart({ src: "scenes/kidney/data/tumor.obj", out: "kidney_tumor.glb", color: 0xb0793a });

  console.log("== 大脑 brain ==");
  await convertPart({ src: "scenes/brain/data/volume_simplified.obj", out: "brain_main.glb" });

  console.log("== 纹理 ==");
  const textures = [
    "scenes/liver/data/liver-texture.png",
    "scenes/kidney/data/kidney-texture.png",
    "scenes/brain/data/texture.png",
  ];
  for (const t of textures) {
    const name = basename(t);
    await copyFile(join(SOURCE, t), join(OUT, name));
    console.log("  copy", name);
  }
  console.log("完成");
}

main().catch((e) => { console.error(e); process.exit(1); });
