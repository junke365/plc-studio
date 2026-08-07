import { JSDOM } from "jsdom";
import * as THREE from "three";
import { readFileSync, existsSync } from "fs";
import { join } from "path";
import { ColladaLoader } from "three/examples/jsm/loaders/ColladaLoader";
import { OBJLoader } from "three/examples/jsm/loaders/OBJLoader";

const dom = new JSDOM("");
(globalThis as any).DOMParser = dom.window.DOMParser;
(globalThis as any).Node = dom.window.Node;

const PX4 = "/Users/junjun/plc-studio/smart-plc-studio/packages/editor/public/models/px4";

(THREE.TextureLoader.prototype as any).load = () => new THREE.Texture();

function bbox(o: THREE.Object3D) {
  const b = new THREE.Box3();
  b.setFromObject(o);
  const s = new THREE.Vector3(); b.getSize(s);
  const c = new THREE.Vector3(); b.getCenter(c);
  return { min: b.min.toArray().map(v=>v.toFixed(3)), max: b.max.toArray().map(v=>v.toFixed(3)), size: s.toArray().map(v=>v.toFixed(3)), center: c.toArray().map(v=>v.toFixed(3)) };
}

async function loadDae(url: string) {
  const path = join(PX4, url.replace(/^\/models\/px4\//, ""));
  const text = readFileSync(path, "utf8");
  const loader = new ColladaLoader();
  const collada = loader.parse(text, url);
  return collada.scene;
}

function loadObj(url: string) {
  const path = join(PX4, url.replace(/^\/models\/px4\//, ""));
  const text = readFileSync(path, "utf8");
  return new OBJLoader().parse(text);
}

// 1. hatchback OBJ raw orientation (ground truth - no dae conversion)
console.log("=== hatchback.obj raw (SDF applies scale 0.0254 + yaw90) ===");
const hb = loadObj("/models/px4/hatchback/meshes/hatchback.obj");
console.log("raw:", JSON.stringify(bbox(hb)));

// 2. parrot hull dae - check up_axis tag + authoring bbox
console.log("\n=== parrot_bebop_2 hull.dae ===");
const raw = readFileSync(join(PX4, "parrot_bebop_2/meshes/hull.dae"), "utf8");
const zup = /<up_axis>\s*Z_UP/i.test(raw);
console.log("up_axis Z_UP:", zup);

// 3. Which rotation makes hull bbox have Z as the height (smallest)?
//    Drone body in Gazebo Z-up: should be flat (Z small), X/Y span.
const hull = await loadDae("/models/px4/parrot_bebop_2/meshes/hull.dae");
// authoring (identity, i.e. what Gazebo effectively sees = Z-up)
const A = hull.clone(true);
A.rotation.set(0,0,0);
console.log("rotation=0 (authoring as-is):", JSON.stringify(bbox(A)));
const B = hull.clone(true);
B.rotation.set(Math.PI/2,0,0); // current UrdfRobot undo for Z_UP
console.log("rotation=+90 (current undo):", JSON.stringify(bbox(B)));
const C = hull.clone(true);
C.rotation.set(-Math.PI/2,0,0); // ColladaLoader default
console.log("rotation=-90 (ColladaLoader):", JSON.stringify(bbox(C)));

// 4. A multi-link model: x500 base NXP-HGD-CF.dae
console.log("\n=== x500 NXP-HGD-CF.dae ===");
const xraw = readFileSync(join(PX4, "x500/meshes/NXP-HGD-CF.dae"), "utf8");
console.log("up_axis Z_UP:", /<up_axis>\s*Z_UP/i.test(xraw));
const xm = await loadDae("/models/px4/x500/meshes/NXP-HGD-CF.dae");
const A2 = xm.clone(true); A2.rotation.set(0,0,0);
console.log("rotation=0:", JSON.stringify(bbox(A2)));
const B2 = xm.clone(true); B2.rotation.set(Math.PI/2,0,0);
console.log("rotation=+90:", JSON.stringify(bbox(B2)));

// 5. explorer_r2 wheels: find a wheel dae and check it's a flat disc
console.log("\n=== explorer_r2 mesh list ===");
console.log(existsSync(join(PX4, "explorer_r2/meshes")) ? readFileSync(join(PX4, "explorer_r2/model.sdf"), "utf8").match(/meshes\/[A-Za-z0-9_.-]+\.dae/g) : "none");
