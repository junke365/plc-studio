import * as THREE from "three";
import { SoftBody } from "./SoftBody";
import type { OrganPhysicsDriver } from "./OrganPhysics";

export type SofaBridgeStatus = "idle" | "connecting" | "online" | "error";

export interface SofaBridgeEvents {
  onStatus?: (status: SofaBridgeStatus) => void;
  onStats?: (stats: { fps: number; bytesPerFrame: number }) => void;
}

export class SofaBridgePhysics implements OrganPhysicsDriver {
  readonly name = "SOFA 桥接 (真实物理)";
  body: SoftBody;
  private ws: WebSocket | null = null;
  private events?: SofaBridgeEvents;
  private lastFrame = 0;
  private frameCount = 0;
  private fps = 0;
  private bytesPerFrame = 0;
  private connecting = false;

  constructor(geo: THREE.BufferGeometry, events?: SofaBridgeEvents) {
    this.body = new SoftBody(geo);
    this.events = events;
  }

  get vertexCount() {
    return this.body.vertexCount;
  }

  get syncedCount() {
    return this.body.displayToPhys.length;
  }

  get connected() {
    return this.ws?.readyState === WebSocket.OPEN;
  }

  setStatus(s: SofaBridgeStatus) {
    this.events?.onStatus?.(s);
  }

  connect(url: string) {
    if (this.connecting || this.connected) return;
    this.connecting = true;
    this.setStatus("connecting");
    try {
      this.ws = new WebSocket(url);
      this.ws.binaryType = "arraybuffer";
      this.ws.onopen = () => {
        this.connecting = false;
        this.frameCount = 0;
        this.lastFrame = performance.now();
        this.setStatus("online");
      };
      this.ws.onmessage = (ev) => {
        if (typeof ev.data === "string") {
          this.handleJson(ev.data);
          return;
        }
        const buf = ev.data as ArrayBuffer;
        if (buf.byteLength >= this.body.pos.byteLength) {
          const view = new Float32Array(buf);
          this.applyState(view);
          this.bytesPerFrame = buf.byteLength;
          this.frameCount++;
        }
      };
      this.ws.onclose = () => {
        this.connecting = false;
        this.ws = null;
        this.setStatus("idle");
      };
      this.ws.onerror = () => {
        this.connecting = false;
        this.setStatus("error");
      };
    } catch (e) {
      this.connecting = false;
      this.setStatus("error");
    }
  }

  disconnect() {
    this.ws?.close();
    this.ws = null;
    this.connecting = false;
    this.setStatus("idle");
  }

  private handleJson(json: string) {
    try {
      const msg = JSON.parse(json);
      if (msg?.type === "info") {
        this.events?.onStats?.({
          fps: Number(msg.fps ?? 0),
          bytesPerFrame: this.bytesPerFrame,
        });
      }
    } catch {
      /* ignore */
    }
  }

  private applyState(pos: Float32Array) {
    const n = Math.min(this.body.vertexCount, Math.floor(pos.length / 3));
    const b = this.body.pos;
    const map = this.body.physToDisplay;
    for (let p = 0; p < n; p++) {
      const d = map[p] * 3;
      b[p * 3] = pos[d];
      b[p * 3 + 1] = pos[d + 1];
      b[p * 3 + 2] = pos[d + 2];
    }
    const now = performance.now();
    const dt = now - this.lastFrame;
    if (dt >= 1000) {
      this.fps = (this.frameCount * 1000) / dt;
      this.frameCount = 0;
      this.lastFrame = now;
      this.events?.onStats?.({ fps: this.fps, bytesPerFrame: this.bytesPerFrame });
    }
  }

  solve(_dt: number) {
    if (this.body.pos.length) {
      this.body.syncToGeometry();
    }
  }

  sendCmd(payload: Record<string, unknown>) {
    if (this.connected && this.ws) {
      this.ws.send(JSON.stringify({ type: "surgery_cmd", ...payload }));
    }
  }

  grab(x: number, y: number, z: number) {
    this.sendCmd({ action: "grab", x, y, z });
  }

  setGrabTarget(x: number, y: number, z: number) {
    this.sendCmd({ action: "grab_target", x, y, z });
  }

  release() {
    this.sendCmd({ action: "release" });
  }

  cut(ax: number, ay: number, az: number, bx: number, by: number, bz: number) {
    this.sendCmd({ action: "cut", ax, ay, az, bx, by, bz });
  }

  reset() {
    this.sendCmd({ action: "reset" });
    for (let i = 0; i < this.body.rest.length; i++) this.body.pos[i] = this.body.rest[i];
    this.body.syncToGeometry();
  }

  dispose() {
    this.disconnect();
  }
}
