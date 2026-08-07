import { JSDOM } from "jsdom";
import * as THREE from "three";
import { readFileSync } from "fs";
import { ColladaLoader } from "three/examples/jsm/loaders/ColladaLoader";

const dom = new JSDOM("");
(globalThis as any).DOMParser = dom.window.DOMParser;
(globalThis as any).Node = dom.window.Node;
(THREE.TextureLoader.prototype as any).load = () => new THREE.Texture();
(THREE as any).Cache.enabled = false;

function bbox(o: THREE.Object3D) {
  const b = new THREE.Box3(); b.setFromObject(o);
  const s = new THREE.Vector3(); b.getSize(s);
  return s.toArray().map(v => v.toFixed(3)).join("x");
}

async function dump(url: string) {
  const text = readFileSync(`/Users/junjun/plc-studio/smart-plc-studio/packages/editor/public/models/px4/${url}`, "utf8");
  const scene = new ColladaLoader().parse(text, url).scene;
  scene.rotation.set(0, 0, 0);
  const names: Record<string, string> = {};
  scene.traverse((o: any) => {
    if (o.isMesh) names[o.name || "(unnamed)"] = `${o.geometry.name ?? ""} | bbox ${bbox(o).split("x").join("x")}`;
  });
  console.log(url, JSON.stringify(names, null, 1));
  // overall
  const all = scene.clone(true);
  const b = new THREE.Box3(); b.setFromObject(all);
  const s = new THREE.Vector3(); b.getSize(s);
  console.log("  TOTAL size", s.toArray().map(v=>v.toFixed(3)).join(" x "));
}

await dump("OGV/meshes/ogv.dae");
await dump("explorer_r2/meshes/r2.dae");
