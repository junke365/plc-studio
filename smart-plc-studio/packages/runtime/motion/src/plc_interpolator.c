#include "plc_interpolator.h"
#include <string.h>
#include <math.h>

#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

void plc_interp_init(Interpolator *interp, float cycleTime)
{
  if (!interp) return;
  memset(interp, 0, sizeof(Interpolator));
  interp->mode = INTERP_MODE_IDLE;
  interp->cycleTime = (cycleTime > 0) ? cycleTime : 0.001f;  /* 默认 1ms */
  interp->usePositionMode = true;
}

int plc_interp_loadSegment(Interpolator *interp, const PlannerSegment *seg)
{
  if (!interp || !seg) return -1;

  interp->currentSegment = *seg;
  interp->segmentLength = seg->length;
  interp->segmentProgress = 0;
  interp->elapsedTime = 0;
  interp->currentS = 0;
  interp->currentVelocity = seg->entryVelocity;
  interp->hasSegment = true;
  interp->segmentTime = 0;

  /* 估计段总时间 */
  if (seg->cruiseVelocity > 0) {
    interp->segmentTime = seg->length / seg->cruiseVelocity;
  } else if (seg->plannedVelocity > 0) {
    interp->segmentTime = seg->length / seg->plannedVelocity;
  } else {
    interp->segmentTime = 0.001f;
  }

  return 0;
}

/* ==================== S 曲线速度计算 ==================== */

float plc_interp_sCurveVelocity(const PlannerSegment *seg, float s)
{
  if (!seg) return 0;
  s = CLAMP(s, 0.0f, 1.0f);

  float vEntry = seg->entryVelocity;
  float vCruise = seg->cruiseVelocity;
  float vExit = seg->exitVelocity;
  float ta = seg->accelerateUntil;
  float td = seg->decelerateAfter;

  if (ta <= 0 && td >= 1.0f) {
    return vCruise;
  }
  if (s <= ta && ta > 0) {
    /* 加速段: v(s) = v_entry + (v_cruise - v_entry) * (s / ta) 带 S 曲线缓入缓出 */
    float ratio = s / ta;
    float sCurve = ratio * ratio * (3.0f - 2.0f * ratio);
    return vEntry + (vCruise - vEntry) * sCurve;
  } else if (s >= td && td < 1.0f) {
    /* 减速段 */
    float ratio = (s - td) / (1.0f - td);
    float sCurve = ratio * ratio * (3.0f - 2.0f * ratio);
    return vCruise - (vCruise - vExit) * sCurve;
  } else {
    /* 匀速段 */
    return vCruise;
  }
}

int plc_interp_computeSCurve(const PlannerSegment *seg, SCurveProfile *profile)
{
  if (!seg || !profile) return -1;
  memset(profile, 0, sizeof(SCurveProfile));

  float ta = seg->accelerateUntil * seg->length / (seg->cruiseVelocity + 1e-10f);
  float td = (1.0f - seg->decelerateAfter) * seg->length / (seg->cruiseVelocity + 1e-10f);
  float tc = (seg->decelerateAfter - seg->accelerateUntil) * seg->length / (seg->cruiseVelocity + 1e-10f);

  if (tc < 0) tc = 0;

  profile->accelTime = ta;
  profile->decelTime = td;
  profile->cruiseTime = tc;
  profile->totalTime = ta + tc + td;
  profile->maxReachedVel = seg->cruiseVelocity;

  return 0;
}

/* ==================== 插补步进 ==================== */

int plc_interp_step(Interpolator *interp, float *jointPos, uint8_t *axisCount)
{
  if (!interp || !jointPos) return -1;
  if (!interp->hasSegment) return -1;
  if (interp->mode != INTERP_MODE_RUNNING) return 0;

  float dt = interp->cycleTime;

  /* 更新进度 */
  float v = plc_interp_sCurveVelocity(&interp->currentSegment, interp->currentS);
  interp->currentVelocity = v;

  float ds = v * dt / (interp->segmentLength + 1e-10f);
  interp->currentS += ds;
  interp->segmentProgress += v * dt;
  interp->elapsedTime += dt;
  interp->stepCount++;

  /* 检查是否结束 */
  if (interp->currentS >= 1.0f) {
    interp->currentS = 1.0f;
    interp->hasSegment = false;
    interp->mode = INTERP_MODE_IDLE;
  }

  /* 插补当前 S 位置 */
  uint8_t count = 0;
  for (int i = 0; i < 9; i++) {
    if (interp->currentSegment.axisMask & (1 << i)) {
      /* 线性插补: pos = start + (end - start) * s */
      /* 简化: 使用目标位置和方向向量 */
      jointPos[i] = interp->currentSegment.direction[i]
                    * interp->segmentLength * interp->currentS;
      count = i + 1;
    } else {
      jointPos[i] = 0;
    }
  }
  if (axisCount) *axisCount = count;

  /* 保存输出 */
  for (int i = 0; i < 9; i++) {
    interp->outputPosition[i] = jointPos[i];
  }

  return interp->hasSegment ? 0 : 1;  /* 0=进行中, 1=段结束 */
}

/* ==================== 控制 ==================== */

void plc_interp_start(Interpolator *interp)
{
  if (interp) interp->mode = INTERP_MODE_RUNNING;
}

void plc_interp_hold(Interpolator *interp)
{
  if (interp) interp->mode = INTERP_MODE_HOLD;
}

void plc_interp_resume(Interpolator *interp)
{
  if (interp) interp->mode = INTERP_MODE_RUNNING;
}

void plc_interp_stop(Interpolator *interp)
{
  if (interp) {
    interp->mode = INTERP_MODE_STOP;
    interp->hasSegment = false;
  }
}

void plc_interp_reset(Interpolator *interp)
{
  if (interp) {
    interp->mode = INTERP_MODE_IDLE;
    interp->hasSegment = false;
    interp->segmentProgress = 0;
    interp->currentS = 0;
    interp->stepCount = 0;
  }
}

/* ==================== 状态查询 ==================== */

bool plc_interp_isIdle(const Interpolator *interp)
{
  return interp ? (interp->mode == INTERP_MODE_IDLE) : true;
}

bool plc_interp_isRunning(const Interpolator *interp)
{
  return interp ? (interp->mode == INTERP_MODE_RUNNING) : false;
}

float plc_interp_getProgress(const Interpolator *interp)
{
  return interp ? interp->currentS : 0;
}

float plc_interp_getVelocity(const Interpolator *interp)
{
  return interp ? interp->currentVelocity : 0;
}

InterpolatorMode plc_interp_getMode(const Interpolator *interp)
{
  return interp ? interp->mode : INTERP_MODE_IDLE;
}
