import * as THREE from "three";
import { SoftBody } from "./SoftBody";
import type { OrganPhysicsDriver } from "./OrganPhysics";

export class MiniSofaPhysics implements OrganPhysicsDriver {
  readonly name = "JS 内置 (MiniSofa)";
  body: SoftBody;
  private geo: THREE.BufferGeometry;

  constructor(geo: THREE.BufferGeometry, opts: { grabRadius?: number; cutRadius?: number } = {}) {
    this.geo = geo;
    this.body = new SoftBody(geo, {
      edgeStiffness: 0.55,
      anchorStiffness: 1.5,
      damping: 0.9,
      substeps: 3,
      grabRadius: opts.grabRadius ?? 0.011,
      cutRadius: opts.cutRadius ?? 0.008,
    });
  }

  get vertexCount() {
    return this.body.vertexCount;
  }

  get syncedCount() {
    return this.body.displayToPhys.length;
  }

  solve(dt: number) {
    this.body.solve(dt);
    this.body.syncToGeometry();
  }

  grab(x: number, y: number, z: number) {
    this.body.grab(x, y, z);
  }

  setGrabTarget(x: number, y: number, z: number) {
    this.body.setGrabTarget(x, y, z);
  }

  release() {
    this.body.release();
  }

  cut(ax: number, ay: number, az: number, bx: number, by: number, bz: number) {
    this.body.cut(ax, ay, az, bx, by, bz);
  }

  reset() {
    const src = this.geo.getAttribute("position") as THREE.BufferAttribute;
    for (let i = 0; i < src.count; i++) {
      src.setXYZ(i, this.body.rest[this.body.displayToPhys[i] * 3], this.body.rest[this.body.displayToPhys[i] * 3 + 1], this.body.rest[this.body.displayToPhys[i] * 3 + 2]);
    }
    src.needsUpdate = true;
    this.body.rest.forEach((v, i) => {
      this.body.pos[i] = v;
      this.body.vel[i] = 0;
    });
    this.body.cutFlag.fill(0);
    for (const e of this.body.edges) e.on = true;
    this.release();
    this.body.syncToGeometry();
  }

  dispose() {
    // 几何由场景统一释放
  }
}
