/* Buster Beagle 3D MK4 注塑机仿真引擎
 * 参考 /Users/junjun/ros/MK4_Automation/MK4_Automation/MK4_Automation.ino
 * 部件: 气动虎钳(vise) + 料斗螺旋给料(auger) + 气动注塑缸(ram) + PID 加热
 *       + 顶出(气动+servo) + 光学掉件检测 + 旋转编码器菜单
 * 自动循环: 合钳 → 注塑 → 补料 → 保压 → 开钳 → 顶出 → 暂停 → 下一件
 */

export interface Mk4Params {
  cycle: {
    injectTime: number;     // 注塑时间 (s)
    viseHoldTime: number;   // 合钳保持时间 (s)
    shotSize: number;       // 补料/料斗时间 (s)
    numOfParts: number;     // 目标件数
    cyclePause: number;     // 件间暂停 (s)
  };
  heating: {
    targetTemp: number;     // 料筒目标温度 °C
    ambientTemp: number;    // 环境温度
    heatRate: number;       // 加热速率 °C/s
    coolRate: number;       // 散热速率 °C/s
  };
  timings: {
    viseCloseSec: number;   // 合钳行程时长
    injectWaitSec: number;  // 合钳后注塑前等待 (5s)
    openDelaySec: number;   // 开钳到顶出等待 (5s)
    ejectDelaySec: number;  // 顶出到掉件延时
    hopperDuringClose: number; // 合钳时料斗喂料时长 (3s)
  };
}

export const DEFAULT_PARAMS: Mk4Params = {
  cycle: { injectTime: 20, viseHoldTime: 30, shotSize: 20, numOfParts: 10, cyclePause: 60 },
  heating: { targetTemp: 220, ambientTemp: 25, heatRate: 6, coolRate: 0.6 },
  timings: { viseCloseSec: 3, injectWaitSec: 5, openDelaySec: 5, ejectDelaySec: 1.5, hopperDuringClose: 3 },
};

export type Mk4Phase =
  | "menu" | "closing_vise" | "inject_wait" | "injecting"
  | "refilling" | "holding" | "opening_vise" | "ejecting" | "pausing" | "done";

const clamp = (v: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, v));

export class Mk4Engine {
  params: Mk4Params;

  phase: Mk4Phase = "menu";
  running = false;
  elapsed = 0;
  message = "菜单 · 按 [开始任务] 启动";

  viseOpen = 0;            // 0 钳口全开 ~ 1 钳口夹紧
  injectionRam = 0;        // 注塑缸伸出 0..1
  augerRpm = 0;            // 料斗螺旋转速
  chamberTemp = 25;        // 料筒温度
  partFill = 0;            // 型腔充填 0..1
  partDropped = false;     // 本件已掉件
  partDropDetected = false;// 光学传感器检测到
  pneumaticInjection = false; // 顶出气动阀
  servoAngle = 0;          // 顶出 servo 角度 0..180
  heatingOn = false;
  hallSensorVise = false;  // 合钳到位霍尔
  hallSensorInject = false;// 注塑霍尔
  partsLeft = 0;
  totalParts = 0;
  partsMade = 0;
  noDetectionCount = 0;
  timeLeft = 0;            // 当前阶段剩余秒数

  private timers: Record<string, number> = {};
  private sosTimer = 0;
  private sosState = 0;
  private servoDir = 1;

  constructor(params: Mk4Params = DEFAULT_PARAMS) {
    this.params = JSON.parse(JSON.stringify(params));
    this.partsLeft = this.params.cycle.numOfParts;
    this.totalParts = this.params.cycle.numOfParts;
  }

  setParams(p: Mk4Params) {
    this.params = JSON.parse(JSON.stringify(p));
    if (this.phase === "menu") {
      this.partsLeft = this.params.cycle.numOfParts;
      this.totalParts = this.params.cycle.numOfParts;
    }
  }

  toggleHeating() {
    this.heatingOn = !this.heatingOn;
    this.message = this.heatingOn ? "加热开启" : "加热关闭";
  }

  startJob() {
    this.partsLeft = this.params.cycle.numOfParts;
    this.totalParts = this.params.cycle.numOfParts;
    this.partsMade = 0;
    this.noDetectionCount = 0;
    this.partDropped = false;
    this.partDropDetected = false;
    this.viseOpen = 0;
    this.injectionRam = 0;
    this.partFill = 0;
    this.phase = "closing_vise";
    this.timers = { close: 0, hopper: this.params.timings.hopperDuringClose };
    this.running = true;
    this.message = "合钳 · 等待到位传感器";
  }

  pause() {
    this.running = false;
    this.message = "已暂停";
  }

  resume() {
    if (this.phase === "menu" || this.phase === "done") return;
    this.running = true;
  }

  manualInject() {
    if (this.running) return;
    this.running = true;
    this.phase = "injecting";
    this.injectionRam = 0;
    this.timers.inject = this.params.cycle.injectTime;
    this.message = "手动注塑";
  }

  manualHopper() {
    if (this.running) return;
    this.running = true;
    this.phase = "refilling";
    this.timers.refill = this.params.cycle.shotSize;
    this.message = "手动补料";
  }

  reset() {
    this.running = false;
    this.phase = "menu";
    this.viseOpen = 0;
    this.injectionRam = 0;
    this.augerRpm = 0;
    this.partFill = 0;
    this.partDropped = false;
    this.partDropDetected = false;
    this.pneumaticInjection = false;
    this.servoAngle = 0;
    this.hallSensorVise = false;
    this.hallSensorInject = false;
    this.partsLeft = this.params.cycle.numOfParts;
    this.totalParts = this.params.cycle.numOfParts;
    this.partsMade = 0;
    this.noDetectionCount = 0;
    this.elapsed = 0;
    this.message = "已复位";
  }

  private updateTemps(dt: number) {
    const h = this.params.heating;
    const d = (this.heatingOn ? h.targetTemp : h.ambientTemp) - this.chamberTemp;
    const rate = this.heatingOn ? h.heatRate : h.coolRate;
    this.chamberTemp += clamp(d, -rate * dt, rate * dt);
  }

  private stepTimer(key: string, dt: number) {
    if (this.timers[key] == null) this.timers[key] = 0;
    this.timers[key] += dt;
  }

  private countdown(key: string, dt: number) {
    if (this.timers[key] == null) this.timers[key] = 0;
    this.timers[key] -= dt;
    this.timeLeft = Math.max(0, this.timers[key]);
  }

  step(dt: number) {
    this.updateTemps(dt);
    if (!this.running) return;
    this.elapsed += dt;
    const c = this.params.cycle;
    const t = this.params.timings;

    switch (this.phase) {
      case "closing_vise": {
        this.viseOpen = clamp(this.viseOpen + dt / t.viseCloseSec, 0, 1);
        this.stepTimer("hopper", dt);
        this.augerRpm = this.timers.hopper < t.hopperDuringClose ? 60 : 0;
        if (this.viseOpen >= 1) {
          this.viseOpen = 1;
          this.hallSensorVise = true;
          this.augerRpm = 0;
          this.phase = "inject_wait";
          this.timers.wait = t.injectWaitSec;
          this.message = "钳口锁定 · 等待注塑";
        }
        break;
      }
      case "inject_wait": {
        this.augerRpm = 0;
        this.countdown("wait", dt);
        if (this.timers.wait <= 0) {
          this.phase = "injecting";
          this.timers.inject = c.injectTime;
          this.message = "注塑中";
        }
        break;
      }
      case "injecting": {
        this.injectionRam = clamp(this.injectionRam + dt / 2, 0, 1);
        this.partFill = clamp(this.partFill + dt / Math.max(1, c.injectTime), 0, 1);
        this.hallSensorInject = this.injectionRam > 0.1;
        this.countdown("inject", dt);
        if (this.timers.inject <= 0) {
          this.injectionRam = 1;
          this.phase = "refilling";
          this.timers.refill = c.shotSize;
          this.message = "补料 · 回填料筒";
        }
        break;
      }
      case "refilling": {
        this.augerRpm = 90;
        this.countdown("refill", dt);
        if (this.timers.refill <= 0) {
          this.augerRpm = 0;
          this.phase = "holding";
          this.timers.hold = c.viseHoldTime;
          this.message = "保压冷却";
        }
        break;
      }
      case "holding": {
        this.augerRpm = 0;
        this.countdown("hold", dt);
        if (this.timers.hold <= 0) {
          this.phase = "opening_vise";
          this.timers.open = t.openDelaySec;
          this.message = "开钳";
        }
        break;
      }
      case "opening_vise": {
        this.viseOpen = clamp(this.viseOpen - dt / t.viseCloseSec, 0, 1);
        this.countdown("open", dt);
        if (this.viseOpen <= 0.05) this.hallSensorVise = false;
        if (this.timers.open <= 0) {
          this.phase = "ejecting";
          this.pneumaticInjection = true;
          this.partDropped = false;
          this.timers.drop = t.ejectDelaySec;
          this.sosTimer = 0;
          this.message = "顶出 · 等待掉件";
        }
        break;
      }
      case "ejecting": {
        this.injectionRam = 0;
        // SOS 莫尔斯码闪烁顶出阀
        this.sosTimer += dt;
        const SOS = [0.133, 0.133, 0.133, 0.4, 0.4, 0.4, 0.133, 0.133, 0.133];
        const onFor = SOS[this.sosState % SOS.length];
        this.pneumaticInjection = this.sosTimer % (onFor + 0.133) < onFor;
        if ((this.sosTimer % (onFor + 0.133)) < dt) this.sosState++;
        // servo 顶出辅助: 0→180→0
        this.servoAngle += this.servoDir * 90 * dt;
        if (this.servoAngle >= 180) { this.servoAngle = 180; this.servoDir = -1; }
        if (this.servoAngle <= 0) { this.servoAngle = 0; this.servoDir = 1; }
        this.countdown("drop", dt);
        if (this.timers.drop <= 0 && !this.partDropped) {
          this.partDropped = true;
          this.partDropDetected = true;
          this.message = "检测到掉件";
        }
        if (this.partDropped) {
          this.pneumaticInjection = false;
          this.partFill = 0;
          this.partsLeft--;
          this.partsMade++;
          if (this.partsLeft > 0) {
            this.phase = "pausing";
            this.timers.pause = c.cyclePause;
            this.message = "件间暂停";
          } else {
            this.phase = "done";
            this.running = false;
            this.message = `任务完成 · 共 ${this.partsMade} 件`;
          }
        }
        break;
      }
      case "pausing": {
        this.countdown("pause", dt);
        if (this.timers.pause <= 0) {
          this.phase = "closing_vise";
          this.timers = { close: 0, hopper: this.params.timings.hopperDuringClose };
          this.hallSensorVise = false;
          this.message = "合钳 · 下一件";
        }
        break;
      }
      default:
        break;
    }
  }
}
