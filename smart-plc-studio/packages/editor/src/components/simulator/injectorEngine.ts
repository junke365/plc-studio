/* Smart Injector 注塑机仿真引擎
 * 参考 /Users/junjun/ros/Smart_Injector/06_Program/Automatik_quick_n_dirty/*.ino
 * 部件: 挤出机(螺杆 NEMA34) + 合模机构(2×NEMA17) + 加热(q1..q3) + 顶出机构
 * 自动循环: 合模 → 注塑(forward) → 保压(delay) → 开模 → 后退(backward) → 等待
 */

export interface InjectorParams {
  cycle: {
    rotationsFor: number;    // 注塑前进转数
    rotationsBack: number;   // 注塑后退转数
    delayBetween: number;    // 注塑与开模间隔 (s)
    waitBeforeOpen: number;  // 开模前等待 (s)
  };
  extruder: {
    speedRpm: number;        // 注塑转速 (rpm)
    suckbackRpm: number;     // 后退转速 (rpm)
    gearRatio: number;       // 减速比 6:1
    stepsPerRev: number;     // 电机每转步数 (400)
  };
  clamp: {
    closeOverSteps: number;  // 合模过行程步数 (endstop1 后)
    openOverSteps: number;   // 开模过行程步数 (endstop2 后)
    stepsPerSec: number;     // 合模电机步进速度 (steps/s)
    closeFastSec: number;    // 合模快速段时长
    openFastSec: number;     // 开模快速段时长
  };
  heating: {
    targetTemp: number;      // 目标温度 °C
    ambientTemp: number;     // 环境温度
    heatRate: number;        // 加热速率 °C/s
    coolRate: number;        // 散热速率 °C/s
    minInjectTemp: number;   // 允许注塑的最低温度
  };
}

export const DEFAULT_PARAMS: InjectorParams = {
  cycle: { rotationsFor: 70, rotationsBack: 18, delayBetween: 1, waitBeforeOpen: 1 },
  extruder: { speedRpm: 33, suckbackRpm: 20, gearRatio: 6, stepsPerRev: 400 },
  clamp: { closeOverSteps: 9000, openOverSteps: 32000, stepsPerSec: 2500, closeFastSec: 2.5, openFastSec: 4 },
  heating: { targetTemp: 200, ambientTemp: 25, heatRate: 8, coolRate: 0.8, minInjectTemp: 150 },
};

export type InjectorPhase =
  | "idle" | "closing" | "injecting" | "holding" | "opening" | "backing" | "waiting" | "done";

const clamp = (v: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, v));

export class InjectorEngine {
  params: InjectorParams;

  phase: InjectorPhase = "idle";
  running = false;
  elapsed = 0;
  message = "待机 · 按 [开始循环] 启动";

  clampPos = 0;          // 0 全开(open endstop) ~ 1 全闭(closed endstop)
  extRot = 0;            // 挤出机螺杆累计转数
  screwRpm = 0;          // 螺杆当前转速
  mouldTemp = 25;
  extruderTemp = 25;
  materialLevel = 0;     // 型腔填充 0..1
  endstop1 = false;      // 合模限位(触到模具)
  endstop2 = true;       // 开模限位(全开)
  cycleCount = 0;
  partEjected = false;
  heatingOn = false;
  mouldClosed = false;

  private holdTimer = 0;
  private waitTimer = 0;
  private injectStart = 0;

  constructor(params: InjectorParams = DEFAULT_PARAMS) {
    this.params = JSON.parse(JSON.stringify(params));
  }

  setParams(p: InjectorParams) {
    this.params = JSON.parse(JSON.stringify(p));
  }

  get clampClosed(): boolean {
    return this.clampPos >= 0.999;
  }

  get closeOverTime(): number {
    return this.params.clamp.closeOverSteps / this.params.clamp.stepsPerSec;
  }

  get openOverTime(): number {
    return this.params.clamp.openOverSteps / this.params.clamp.stepsPerSec;
  }

  get clampSteps(): number {
    const p = this.params.clamp;
    const fast = p.closeFastSec * p.stepsPerSec;
    if (this.clampPos <= 0.9) return (this.clampPos / 0.9) * fast;
    return fast + ((this.clampPos - 0.9) / 0.1) * p.closeOverSteps;
  }

  toggleHeating() {
    this.heatingOn = !this.heatingOn;
    this.message = this.heatingOn ? "加热开启" : "加热关闭";
  }

  startCycle() {
    this.partEjected = false;
    this.materialLevel = 0;
    this.phase = "closing";
    this.running = true;
    this.message = "合模中";
  }

  pause() {
    this.running = false;
    this.message = "已暂停";
  }

  resume() {
    if (this.phase === "idle" || this.phase === "done") return;
    this.running = true;
  }

  manualBackward() {
    if (this.running) return;
    this.phase = "backing";
    this.running = true;
    this.message = "手动后退(泄压)";
  }

  reset() {
    this.running = false;
    this.phase = "idle";
    this.clampPos = 0;
    this.extRot = 0;
    this.screwRpm = 0;
    this.materialLevel = 0;
    this.mouldTemp = this.params.heating.ambientTemp;
    this.extruderTemp = this.params.heating.ambientTemp;
    this.endstop1 = false;
    this.endstop2 = true;
    this.partEjected = false;
    this.mouldClosed = false;
    this.cycleCount = 0;
    this.elapsed = 0;
    this.message = "已复位";
  }

  private updateTemps(dt: number) {
    const h = this.params.heating;
    const step = (v: number, target: number, rate: number) => {
      const d = target - v;
      return v + clamp(d, -rate * dt, rate * dt);
    };
    if (this.heatingOn) {
      this.mouldTemp = step(this.mouldTemp, h.targetTemp, h.heatRate);
      this.extruderTemp = step(this.extruderTemp, h.targetTemp, h.heatRate);
    } else {
      this.mouldTemp = step(this.mouldTemp, h.ambientTemp, h.coolRate);
      this.extruderTemp = step(this.extruderTemp, h.ambientTemp, h.coolRate);
    }
  }

  step(dt: number) {
    this.updateTemps(dt);
    if (!this.running) return;
    this.elapsed += dt;
    const p = this.params;
    const ramp = 60 * dt;

    switch (this.phase) {
      case "closing": {
        this.screwRpm = 0;
        const fast = 1 / p.clamp.closeFastSec;
        const slow = 0.1 / this.closeOverTime;
        if (this.clampPos < 0.9) {
          this.clampPos = Math.min(0.9, this.clampPos + fast * dt);
          if (this.clampPos >= 0.9) this.endstop1 = true;
        } else {
          this.clampPos = Math.min(1, this.clampPos + slow * dt);
        }
        if (this.clampPos >= 1) {
          this.clampPos = 1;
          this.mouldClosed = true;
          this.phase = "injecting";
          this.injectStart = this.extRot;
          this.message = "注塑中";
        }
        break;
      }
      case "injecting": {
        this.screwRpm = Math.min(p.extruder.speedRpm, this.screwRpm + ramp);
        this.extRot += (this.screwRpm / 60) * dt;
        this.materialLevel = clamp((this.extRot - this.injectStart) / p.cycle.rotationsFor, 0, 1);
        if (this.extRot >= this.injectStart + p.cycle.rotationsFor) {
          this.extRot = this.injectStart + p.cycle.rotationsFor;
          this.materialLevel = 1;
          this.phase = "holding";
          this.holdTimer = p.cycle.delayBetween;
          this.message = "保压冷却";
        }
        break;
      }
      case "holding": {
        this.screwRpm = 0;
        this.holdTimer -= dt;
        if (this.holdTimer <= 0) {
          this.phase = "opening";
          this.message = "开模";
        }
        break;
      }
      case "opening": {
        this.screwRpm = 0;
        const fast = 1 / p.clamp.openFastSec;
        const slow = 0.1 / this.openOverTime;
        if (this.clampPos > 0.1) {
          this.clampPos = Math.max(0.1, this.clampPos - fast * dt);
          if (this.clampPos <= 0.1) this.endstop2 = true;
        } else {
          this.clampPos = Math.max(0, this.clampPos - slow * dt);
        }
        if (this.clampPos < 0.45 && !this.partEjected) {
          this.partEjected = true;
          this.message = "顶出产品";
        }
        if (this.clampPos <= 0) {
          this.clampPos = 0;
          this.mouldClosed = false;
          this.phase = "backing";
          this.message = "螺杆后退(泄压)";
        }
        break;
      }
      case "backing": {
        this.screwRpm = Math.max(-p.extruder.suckbackRpm, this.screwRpm - ramp * 1.5);
        this.extRot += (this.screwRpm / 60) * dt;
        const target = this.injectStart + p.cycle.rotationsFor - p.cycle.rotationsBack;
        if (this.extRot <= target) {
          this.extRot = target;
          this.screwRpm = 0;
          this.phase = "waiting";
          this.waitTimer = p.cycle.waitBeforeOpen;
          this.message = "等待下一循环";
        }
        break;
      }
      case "waiting": {
        this.screwRpm = 0;
        this.waitTimer -= dt;
        if (this.waitTimer <= 0) {
          this.cycleCount++;
          this.phase = "idle";
          this.running = false;
          this.materialLevel = 0;
          this.message = `第 ${this.cycleCount} 次循环完成`;
        }
        break;
      }
      default:
        break;
    }
  }
}
