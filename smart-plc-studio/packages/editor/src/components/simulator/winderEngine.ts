/* BLDC 绕线机仿真引擎
 * 移植自 /Users/junjun/ros/winder (aotenjo-xyz/winder) 的 winding.py / ws.py 逻辑
 * 4 台电机:
 *   M0 沿定子轴滑动(分配绕线范围)  M1 旋转定子(选齿槽)  M2 绕线臂(绕圈)  M3 张紧力
 */

export interface MotorParams {
  direction: boolean;
  velocity: number;
  wind_range_start?: number;
  wind_range_end?: number;
  end_to_zero?: number;
  zero?: number;
  end_to_rotating_position?: number;
  angle_to_prevent_collision?: number;
  pull_wire_torque?: number;
  wind_torque?: number;
}

export interface WinderParams {
  motors: [MotorParams, MotorParams, MotorParams, MotorParams];
  winding: {
    turns: number;
    starts_at: number;
    winding_config: string;
    dont_move_m3: boolean;
  };
}

export const DEFAULT_PARAMS: WinderParams = {
  motors: [
    { direction: false, velocity: 5.0, wind_range_start: -25.5, wind_range_end: -14.0, end_to_zero: 9.0 },
    { direction: true, velocity: 5.0, zero: -0.04, end_to_rotating_position: 6.0 },
    { direction: true, velocity: 20.0, zero: 0.23, angle_to_prevent_collision: 1.0 },
    { direction: false, velocity: 5.0, pull_wire_torque: 0.15, wind_torque: 0.04 },
  ],
  winding: { turns: 75, starts_at: 0, winding_config: "AaAabBbBCcCcaAaABbBbcCcC", dont_move_m3: false },
};

export const WINDING_PRESETS: Record<string, { label: string; config: string }> = {
  "12n14p": { label: "12 槽 14 极", config: "AabBCcaABbcC" },
  "24n22p": { label: "24 槽 22 极", config: "AaAabBbBCcCcaAaABbBbcCcC" },
  "36n40p": { label: "36 槽 40 极", config: "AaABbBCcCAaABbBCcCAaABbBCcCAaABbBCcC" },
};

export const M2_GEAR_RATIO = 1;

export type WinderPhase = "idle" | "init" | "wind" | "around" | "done" | "error";

interface Cmd {
  kind: "set" | "wait" | "wind" | "around" | "init" | "label";
  id?: number;
  target?: number;
  teethIdx?: number;
  wire?: number;
  clockwise?: boolean;
  nextWire?: number;
  text?: string;
  sub?: number;
  issued?: boolean;
}

function getTeethIndexMatrix(cfg: string): number[][] {
  const low = cfg.toLowerCase();
  const a: number[] = [], b: number[] = [], c: number[] = [];
  for (let i = 0; i < low.length; i++) {
    if (low[i] === "a") a.push(i);
    else if (low[i] === "b") b.push(i);
    else if (low[i] === "c") c.push(i);
  }
  return [a, b, c];
}

function isClockwise(cfg: string, teethIdx: number): boolean {
  return cfg[teethIdx] === cfg[teethIdx].toLowerCase();
}

export function easeOutSineM0(progress: number, range: [number, number]): number {
  const d = Math.abs(range[1] - range[0]);
  let p = progress;
  if (p > 0.5) p = 1 - p;
  p *= 2;
  return d * Math.sin((p * Math.PI) / 2) + range[0];
}

export class WinderEngine {
  params: WinderParams;
  positions: [number, number, number, number] = [0, 0, 0, 0];
  targets: [number, number, number, number] = [0, 0, 0, 0];

  teethCount = 0;
  matrix: number[][] = [[], [], []];
  numTooth = 0;
  m0Range: [number, number] = [0, 0];
  m0Zero = 0;
  m1Zero = 0;
  m2Zero = 0;
  m1RotatingPosition = 0;
  m2Angle = 0;
  m3WindTorque = 0;
  m3PullTorque = 0;
  m3SlowTorque = 0.03;

  running = false;
  phase: WinderPhase = "idle";
  currentWire = -1;
  windIdx = 0;
  currentTeethIdx = 0;
  turnsDone = 0;
  elapsed = 0;
  message = "待机";

  private cmds: Cmd[] = [];
  private windStart = 0;
  private windTarget = 0;

  constructor(params: WinderParams = DEFAULT_PARAMS) {
    this.params = JSON.parse(JSON.stringify(params));
    this.recompute();
  }

  recompute() {
    const p = this.params;
    const cfg = p.winding.winding_config.trim();
    this.teethCount = cfg.length;
    this.matrix = getTeethIndexMatrix(cfg);
    this.numTooth = Math.floor(cfg.length / 3);
    const m0 = p.motors[0], m1 = p.motors[1], m2 = p.motors[2], m3 = p.motors[3];
    this.m0Range = [m0.wind_range_start ?? -25.5, m0.wind_range_end ?? -14];
    this.m0Zero = (m0.end_to_zero ?? 9) + this.m0Range[1];
    this.m1Zero = m1.zero ?? 0;
    this.m2Zero = m2.zero ?? 0;
    this.m1RotatingPosition = (m1.end_to_rotating_position ?? 6) + this.m0Range[1];
    this.m2Angle = m2.angle_to_prevent_collision ?? 1;
    this.m3WindTorque = p.winding.dont_move_m3 ? 0 : (m3.wind_torque ?? 0.04);
    this.m3PullTorque = p.winding.dont_move_m3 ? 0 : (m3.pull_wire_torque ?? 0.15);
  }

  setParams(p: WinderParams) {
    this.params = JSON.parse(JSON.stringify(p));
    this.recompute();
  }

  get velocity(): [number, number, number, number] {
    return this.params.motors.map((m) => m.velocity) as [number, number, number, number];
  }

  setTarget(id: number, target: number) {
    const dir = this.params.motors[id].direction;
    let v = dir ? target : -target;
    if (id === 2) v *= M2_GEAR_RATIO;
    this.targets[id] = v;
  }

  getCurrentTeeth(): number {
    const diff = Math.abs(this.m1Zero - this.positions[1]);
    let n = Math.round(diff / ((Math.PI * 2) / this.teethCount));
    if (n >= this.teethCount) n %= this.teethCount;
    return n;
  }

  private waitAll(ids: number[], tol = 0.05): boolean {
    return ids.every((id) => Math.abs(this.positions[id] - this.targets[id]) < tol);
  }

  private issue(cmd: Cmd) {
    switch (cmd.kind) {
      case "init":
        this.setTarget(1, this.m1Zero);
        this.setTarget(0, this.m0Zero);
        this.setTarget(2, this.m2Zero);
        this.setTarget(3, 0);
        break;
      case "set":
        this.setTarget(cmd.id!, cmd.target!);
        break;
      case "wind": {
        const turns = this.params.winding.turns;
        this.windStart = this.positions[2];
        this.windTarget = this.windStart + Math.PI * 2 * turns;
        this.setTarget(2, this.windTarget);
        this.setTarget(3, this.m3WindTorque);
        break;
      }
      case "around": {
        const startFromCw = isClockwise(this.params.winding.winding_config, this.matrix[cmd.nextWire!][this.params.winding.starts_at]);
        if (startFromCw) {
          this.m1Zero -= Math.PI * 2 * 2;
          this.setTarget(1, this.m1Zero);
        } else {
          this.m1Zero += Math.PI * 2 * 3;
          this.setTarget(1, this.m1Zero);
        }
        break;
      }
      default:
        break;
    }
  }

  private stepCmd(): boolean {
    const cmd = this.cmds[0];
    if (!cmd) return true;
    switch (cmd.kind) {
      case "init":
        return this.waitAll([0, 1, 2, 3], 0.1);
      case "set":
        this.issue(cmd);
        this.cmds.shift();
        return false;
      case "wait":
        return this.waitAll([cmd.id!], 0.1);
      case "label":
        this.message = cmd.text ?? "";
        this.cmds.shift();
        return false;
      case "wind": {
        if (!cmd.issued) { this.issue(cmd); cmd.issued = true; }
        this.currentWire = cmd.wire ?? -1;
        const total = Math.abs(this.windTarget - this.windStart);
        const done = Math.abs(this.positions[2] - this.windStart);
        const progress = total > 0 ? Math.min(1, done / total) : 1;
        this.setTarget(0, easeOutSineM0(progress, this.m0Range));
        this.turnsDone = Math.floor(progress * this.params.winding.turns);
        this.currentTeethIdx = cmd.teethIdx ?? 0;
        this.windIdx = cmd.sub ?? 0;
        if (Math.abs(this.positions[2] - this.windTarget) < 0.1) {
          this.setTarget(0, this.m1RotatingPosition);
          this.cmds.shift();
        }
        return false;
      }
      case "around":
        if (!cmd.issued) { this.issue(cmd); cmd.issued = true; }
        this.currentWire = -1;
        if (Math.abs(this.positions[1] - this.targets[1]) < 0.1) this.cmds.shift();
        return false;
      default:
        this.cmds.shift();
        return false;
    }
  }

  step(dt: number) {
    if (!this.running) return;
    this.elapsed += dt;
    const vel = this.velocity;
    for (let i = 0; i < 4; i++) {
      const diff = this.targets[i] - this.positions[i];
      const maxMove = vel[i] * dt;
      if (Math.abs(diff) <= maxMove) this.positions[i] = this.targets[i];
      else this.positions[i] += Math.sign(diff) * maxMove;
    }
    if (this.phase === "idle" || this.phase === "done") return;
    if (this.stepCmd()) {
      this.cmds.shift();
      if (!this.cmds.length) {
        this.phase = "done";
        this.message = "绕线完成";
        this.running = false;
      }
    }
  }

  initPosition() {
    this.stopSequence();
    this.cmds = [
      { kind: "label", text: "初始化位置" },
      { kind: "init" },
      { kind: "wait", id: 0 },
      { kind: "set", id: 3, target: this.m3PullTorque },
    ];
    this.phase = "init";
    this.running = true;
  }

  startContinuous() {
    this.stopSequence();
    this.cmds = [{ kind: "label", text: "连续绕线 (A → 换轴 → B → 换轴 → C)" }, { kind: "init" }];
    const cfg = this.params.winding.winding_config;
    const n = this.teethCount;
    for (let wire = 0; wire < 3; wire++) {
      if (wire > 0) this.cmds.push({ kind: "around", nextWire: wire });
      const teeth = this.matrix[wire];
      for (let i = this.params.winding.starts_at; i < this.numTooth; i++) {
        const teethIdx = teeth[i];
        this.cmds.push({
          kind: "label",
          text: `线 ${"ABC"[wire]} · 齿 ${teethIdx} · ${i + 1}/${this.numTooth}`,
        });
        this.cmds.push({ kind: "set", id: 1, target: this.m1Zero - ((Math.PI * 2) / n) * teethIdx });
        this.cmds.push({ kind: "wait", id: 1 });
        this.cmds.push({ kind: "set", id: 3, target: this.m3PullTorque });
        this.cmds.push({ kind: "set", id: 3, target: this.m3WindTorque });
        this.cmds.push({
          kind: "wind",
          teethIdx,
          wire,
          clockwise: isClockwise(cfg, teethIdx),
          sub: i,
        });
      }
    }
    this.phase = "wind";
    this.running = true;
    this.currentWire = 0;
  }

  stopSequence() {
    this.cmds = [];
    this.running = false;
    this.phase = "idle";
    this.message = "已停止";
  }

  estop() {
    this.stopSequence();
    for (let i = 0; i < 4; i++) this.targets[i] = this.positions[i];
  }

  reset() {
    this.stopSequence();
    this.positions = [0, 0, 0, 0];
    this.targets = [0, 0, 0, 0];
    this.elapsed = 0;
    this.turnsDone = 0;
    this.currentWire = -1;
    this.windIdx = 0;
    this.currentTeethIdx = 0;
    this.recompute();
    this.message = "已复位";
  }

  syncPositions(pos: [number, number, number, number]) {
    for (let i = 0; i < 4; i++) {
      this.positions[i] = pos[i];
      this.targets[i] = pos[i];
    }
  }
}
