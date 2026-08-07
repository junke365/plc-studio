import * as THREE from "three";

export interface SoftBodyOptions {
  edgeStiffness?: number;
  anchorStiffness?: number;
  damping?: number;
  substeps?: number;
  grabRadius?: number;
  cutRadius?: number;
}

interface Edge {
  a: number;
  b: number;
  rest: number;
  on: boolean;
}

const _tmp = new THREE.Vector3();

function segDist(
  ax: number, ay: number, az: number,
  bx: number, by: number, bz: number,
  cx: number, cy: number, cz: number,
  dx: number, dy: number, dz: number
): number {
  const r = (dx - ax) * (cx - ax) + (dy - ay) * (cy - ay) + (dz - az) * (cz - az);
  const s = (dx - ax) * (bx - ax) + (dy - ay) * (by - ay) + (dz - az) * (bz - az);
  const t = (dx - bx) * (cx - bx) + (dy - by) * (cy - by) + (dz - bz) * (cz - bz);
  const u = (dx - bx) * (bx - ax) + (dy - by) * (by - ay) + (dz - bz) * (bz - az);
  const abx = bx - ax, aby = by - ay, abz = bz - az;
  const ll = abx * abx + aby * aby + abz * abz;
  const m = ll > 1e-12 ? ll : 1e-12;
  const v = (r * u - s * t) / (m > 0 ? m : 1);
  const w = (r + v) / m;
  const s1 = Math.min(1, Math.max(0, w));
  const s2 = Math.min(1, Math.max(0, v));
  const lx = ax + (bx - ax) * s1;
  const ly = ay + (by - ay) * s1;
  const lz = az + (bz - az) * s1;
  const mx = cx + (dx - cx) * s2;
  const my = cy + (dy - cy) * s2;
  const mz = cz + (dz - cz) * s2;
  const ox = lx - mx, oy = ly - my, oz = lz - mz;
  return Math.sqrt(ox * ox + oy * oy + oz * oz);
}

export class SoftBody {
  rest = new Float32Array(0);
  pos = new Float32Array(0);
  vel = new Float32Array(0);
  faces: Int32Array = new Int32Array(0);
  edges: Edge[] = [];
  displayToPhys: Int32Array = new Int32Array(0);
  physToDisplay: Int32Array = new Int32Array(0);
  cutFlag: Uint8Array = new Uint8Array(0);
  private grabbed: Uint8Array = new Uint8Array(0);
  private grabbedCount = 0;
  private n = 0;
  private geo: THREE.BufferGeometry;
  private normals: Float32Array = new Float32Array(0);
  private kEdge: number;
  private kAnchor: number;
  private damp: number;
  private substeps: number;
  grabRadius: number;
  cutRadius: number;

  constructor(geo: THREE.BufferGeometry, opts: SoftBodyOptions = {}) {
    this.geo = geo;
    this.kEdge = opts.edgeStiffness ?? 0.6;
    this.kAnchor = opts.anchorStiffness ?? 1.4;
    this.damp = opts.damping ?? 0.92;
    this.substeps = opts.substeps ?? 4;
    this.grabRadius = opts.grabRadius ?? 0.012;
    this.cutRadius = opts.cutRadius ?? 0.01;
    this.weld();
  }

  private weld() {
    const src = this.geo.getAttribute("position") as THREE.BufferAttribute;
    const N = src.count;
    this.geo.computeBoundingBox();
    const box = this.geo.boundingBox!;
    const size = new THREE.Vector3();
    box.getSize(size);
    const maxDim = Math.max(size.x, size.y, size.z);
    const cell = maxDim * 2e-3 || 1e-4;

    const positions: number[] = [];
    const map = new Map<number, number>();
    this.displayToPhys = new Int32Array(N);
    this.physToDisplay = new Int32Array(0);

    for (let i = 0; i < N; i++) {
      const x = src.getX(i), y = src.getY(i), z = src.getZ(i);
      const cx = Math.round(x / cell), cy = Math.round(y / cell), cz = Math.round(z / cell);
      const key = (cx * 73856093) ^ (cy * 19349663) ^ (cz * 83492791);
      const found = map.get(key);
      if (found !== undefined) {
        const px = found * 3;
        const dx = positions[px] - x;
        const dy = positions[px + 1] - y;
        const dz = positions[px + 2] - z;
        if (dx * dx + dy * dy + dz * dz <= cell * cell) {
          this.displayToPhys[i] = found;
          continue;
        }
      }
      const idx = positions.length / 3;
      positions.push(x, y, z);
      this.displayToPhys[i] = idx;
      const next = new Int32Array(this.physToDisplay.length + 1);
      next.set(this.physToDisplay);
      next[idx] = i;
      this.physToDisplay = next;
      map.set(key, idx);
    }

    this.n = positions.length;
    this.rest = new Float32Array(positions);
    this.pos = new Float32Array(positions);
    this.vel = new Float32Array(this.n * 3);
    this.cutFlag = new Uint8Array(this.n);
    this.grabbed = new Uint8Array(this.n);
    this.normals = new Float32Array(this.n * 3);

    const index = this.geo.getIndex();
    const triCount = index ? index.count / 3 : N / 3;
    const triArr = new Int32Array(triCount * 3);
    if (index) {
      for (let i = 0; i < index.count; i++) triArr[i] = this.displayToPhys[index.getX(i)];
    } else {
      for (let i = 0; i < N; i++) triArr[i] = this.displayToPhys[i];
    }
    this.faces = triArr;

    const edgeSet = new Set<string>();
    for (let i = 0; i < triCount; i++) {
      const a = triArr[i * 3], b = triArr[i * 3 + 1], c = triArr[i * 3 + 2];
      for (const [p, q] of [[a, b], [b, c], [c, a]] as const) {
        const key = p < q ? `${p}:${q}` : `${q}:${p}`;
        if (!edgeSet.has(key)) {
          edgeSet.add(key);
          const rest = Math.hypot(
            this.rest[p * 3] - this.rest[q * 3],
            this.rest[p * 3 + 1] - this.rest[q * 3 + 1],
            this.rest[p * 3 + 2] - this.rest[q * 3 + 2]
          );
          this.edges.push({ a: p, b: q, rest, on: true });
        }
      }
    }
  }

  setGrabTarget(x: number, y: number, z: number) {
    for (let i = 0; i < this.n; i++) {
      if (!this.grabbed[i]) continue;
      this.pos[i * 3] = x;
      this.pos[i * 3 + 1] = y;
      this.pos[i * 3 + 2] = z;
      this.vel[i * 3] = 0;
      this.vel[i * 3 + 1] = 0;
      this.vel[i * 3 + 2] = 0;
    }
  }

  grab(x: number, y: number, z: number) {
    const r = this.grabRadius * 1.5;
    const r2 = r * r;
    for (let i = 0; i < this.n; i++) {
      if (this.grabbed[i]) continue;
      const dx = this.pos[i * 3] - x;
      const dy = this.pos[i * 3 + 1] - y;
      const dz = this.pos[i * 3 + 2] - z;
      if (dx * dx + dy * dy + dz * dz <= r2) {
        this.grabbed[i] = 1;
        this.grabbedCount++;
      }
    }
  }

  release() {
    this.grabbed.fill(0);
    this.grabbedCount = 0;
  }

  private resetNormals() {
    this.normals.fill(0);
  }

  cut(ax: number, ay: number, az: number, bx: number, by: number, bz: number) {
    const r = this.cutRadius;
    const r2 = r * r;
    for (const e of this.edges) {
      if (!e.on) continue;
      const i = e.a * 3, j = e.b * 3;
      const d = segDist(
        this.pos[i], this.pos[i + 1], this.pos[i + 2],
        this.pos[j], this.pos[j + 1], this.pos[j + 2],
        ax, ay, az, bx, by, bz
      );
      if (d < r) {
        e.on = false;
        this.cutFlag[e.a] = 1;
        this.cutFlag[e.b] = 1;
        const cx = (this.pos[i] + this.pos[j]) / 2;
        const cy = (this.pos[i + 1] + this.pos[j + 1]) / 2;
        const cz = (this.pos[i + 2] + this.pos[j + 2]) / 2;
        const nx = cx - ax, ny = cy - ay, nz = cz - az;
        const nl = Math.sqrt(nx * nx + ny * ny + nz * nz) || 1;
        const push = 0.12 * this.cutRadius;
        this.pos[i] += (nx / nl) * push;
        this.pos[j] += (nx / nl) * push;
      }
    }
    for (let i = 0; i < this.n; i++) {
      if (this.cutFlag[i]) continue;
      const dx = this.pos[i * 3] - ax;
      const dy = this.pos[i * 3 + 1] - ay;
      const dz = this.pos[i * 3 + 2] - az;
      if (dx * dx + dy * dy + dz * dz < r2 * 4) {
        this.cutFlag[i] = 1;
      }
    }
  }

  solve(dt: number) {
    const h = dt / this.substeps;
    const kE = this.kEdge;
    const kA = this.kAnchor;
    for (let s = 0; s < this.substeps; s++) {
      const fx = new Float32Array(this.n).fill(0);
      const fy = new Float32Array(this.n).fill(0);
      const fz = new Float32Array(this.n).fill(0);

      for (const e of this.edges) {
        if (!e.on) continue;
        const i = e.a * 3, j = e.b * 3;
        let dx = this.pos[j] - this.pos[i];
        let dy = this.pos[j + 1] - this.pos[i + 1];
        let dz = this.pos[j + 2] - this.pos[i + 2];
        const dist = Math.sqrt(dx * dx + dy * dy + dz * dz) || 1e-6;
        const f = (kE * (dist - e.rest)) / dist;
        dx *= f; dy *= f; dz *= f;
        fx[e.a] += dx; fy[e.a] += dy; fz[e.a] += dz;
        fx[e.b] -= dx; fy[e.b] -= dy; fz[e.b] -= dz;
      }

      for (let i = 0; i < this.n; i++) {
        if (this.grabbed[i]) continue;
        const x = i * 3, y = i * 3 + 1, z = i * 3 + 2;
        fx[i] += (this.rest[x] - this.pos[x]) * kA;
        fy[i] += (this.rest[y] - this.pos[y]) * kA;
        fz[i] += (this.rest[z] - this.pos[z]) * kA;

        this.vel[x] += fx[i] * h;
        this.vel[y] += fy[i] * h;
        this.vel[z] += fz[i] * h;
        this.vel[x] *= this.damp;
        this.vel[y] *= this.damp;
        this.vel[z] *= this.damp;
        this.pos[x] += this.vel[x] * h;
        this.pos[y] += this.vel[y] * h;
        this.pos[z] += this.vel[z] * h;
      }
    }
  }

  syncToGeometry() {
    const geo = this.geo;
    const src = geo.getAttribute("position") as THREE.BufferAttribute;
    if (!src) return;
    let normAttr = geo.getAttribute("normal") as THREE.BufferAttribute | undefined;
    let colAttr = geo.getAttribute("color") as THREE.BufferAttribute | undefined;
    if (!normAttr) {
      normAttr = new THREE.BufferAttribute(new Float32Array(src.count * 3), 3);
      geo.setAttribute("normal", normAttr);
    }
    if (!colAttr) {
      colAttr = new THREE.BufferAttribute(new Float32Array(src.count * 3), 3);
      geo.setAttribute("color", colAttr);
    }

    this.resetNormals();
    for (let i = 0; i < this.faces.length; i += 3) {
      const a = this.faces[i], b = this.faces[i + 1], c = this.faces[i + 2];
      _tmp.set(
        (this.pos[b * 3 + 1] - this.pos[a * 3 + 1]) * (this.pos[c * 3 + 2] - this.pos[a * 3 + 2]) -
        (this.pos[b * 3 + 2] - this.pos[a * 3 + 2]) * (this.pos[c * 3 + 1] - this.pos[a * 3 + 1]),
        (this.pos[b * 3 + 2] - this.pos[a * 3 + 2]) * (this.pos[c * 3] - this.pos[a * 3]) -
        (this.pos[b * 3] - this.pos[a * 3]) * (this.pos[c * 3 + 2] - this.pos[a * 3 + 2]),
        (this.pos[b * 3] - this.pos[a * 3]) * (this.pos[c * 3 + 1] - this.pos[a * 3 + 1]) -
        (this.pos[b * 3 + 1] - this.pos[a * 3 + 1]) * (this.pos[c * 3] - this.pos[a * 3])
      );
      _tmp.normalize();
      for (const v of [a, b, c]) {
        const k = v * 3;
        this.normals[k] += _tmp.x;
        this.normals[k + 1] += _tmp.y;
        this.normals[k + 2] += _tmp.z;
      }
    }

    const N = src.count;
    for (let i = 0; i < N; i++) {
      const p = this.displayToPhys[i];
      const px = p * 3;
      src.setXYZ(i, this.pos[px], this.pos[px + 1], this.pos[px + 2]);
      if (normAttr) {
        let nx = this.normals[px], ny = this.normals[px + 1], nz = this.normals[px + 2];
        const nl = Math.sqrt(nx * nx + ny * ny + nz * nz) || 1;
        normAttr.setXYZ(i, nx / nl, ny / nl, nz / nl);
      }
      if (colAttr) {
        if (this.cutFlag[p]) {
          const v = 1 - 0.75 * Math.min(1, this.cutFlag[p]);
          colAttr.setXYZ(i, v * 0.55, v * 0.08, v * 0.06);
        } else {
          colAttr.setXYZ(i, 1, 1, 1);
        }
      }
    }
    src.needsUpdate = true;
    if (normAttr) normAttr.needsUpdate = true;
    if (colAttr) colAttr.needsUpdate = true;
    geo.computeBoundingSphere();
  }

  get vertexCount() {
    return this.n;
  }
}
