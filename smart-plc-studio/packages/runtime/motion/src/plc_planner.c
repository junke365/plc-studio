#include "plc_planner.h"
#include "plc_motion.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) MAX((lo), MIN((hi), (x)))

/* ==================== 几何工具 ==================== */

float plc_plan_length(const float *start, const float *end, uint8_t axisMask)
{
  float len = 0;
  for (int i = 0; i < 9; i++) {
    if (axisMask & (1 << i)) {
      float d = end[i] - start[i];
      len += d * d;
    }
  }
  return sqrtf(len);
}

float plc_plan_arcLength(float radius, float startAngle, float endAngle)
{
  float theta = fabsf(endAngle - startAngle);
  /* 保证最短弧 */
  if (theta > M_PI) theta = 2 * M_PI - theta;
  return radius * theta;
}

float plc_plan_junctionVelocity(const float *dirA, const float *dirB,
                                float maxAccel, float junctionDeviation)
{
  if (!dirA || !dirB) return 0;

  /* 计算方向向量的点积 */
  float dot = 0;
  for (int i = 0; i < 3; i++) {
    dot += dirA[i] * dirB[i];
  }

  /* 夹角余弦。dot=1 同向, dot=-1 反向 */
  float cosTheta = CLAMP(dot, -1.0f, 1.0f);

  if (cosTheta >= 0.99f) {
    /* 几乎共线，无拐角速度限制 */
    return 1e10f;
  }

  /* 根据拐角偏差计算最大拐角速度
   * v_junction = sqrt(2 * accel * junctionDeviation / (1 - cos(theta)))
   * 这是 G2 的拐角速度公式 */
  float denom = 1.0f - cosTheta;
  if (denom < 1e-10f) denom = 1e-10f;

  return sqrtf(2.0f * maxAccel * junctionDeviation / denom);
}

static void compute_direction(const float *from, const float *to,
                              uint8_t axisMask, float *dir)
{
  float len = plc_plan_length(from, to, axisMask);
  if (len < 1e-10f) {
    memset(dir, 0, sizeof(float) * 9);
    return;
  }
  for (int i = 0; i < 9; i++) {
    dir[i] = (to[i] - from[i]) / len;
  }
}

/* ==================== S 曲线剖面时间 ==================== */

float plc_plan_profileTime(float length, float vEntry, float vPlan,
                           float vExit, float accel, float jerk,
                           float *outTacc, float *outTdec)
{
  /* 梯形/S 曲线速度规划
   * 限制: 加速和减速段受 jerk 和 accel 限制 */
  float vMax = MIN(vPlan, sqrtf(length * accel + (vEntry*vEntry + vExit*vExit) / 2.0f));

  /* 加速段时间 */
  float ta = 0;
  float td = 0;

  if (vMax > vEntry) {
    ta = (vMax - vEntry) / accel;
    /* 考虑 jerk 限制增加的时间 */
    float jTime = accel / jerk;
    if (jTime > ta * 0.5f) jTime = ta * 0.5f;
    ta += jTime;
  }

  if (vMax > vExit) {
    td = (vMax - vExit) / accel;
    float jTime = accel / jerk;
    if (jTime > td * 0.5f) jTime = td * 0.5f;
    td += jTime;
  }

  /* 匀速段时间 */
  float accelDist = (vEntry + vMax) * ta * 0.5f;
  float decelDist = (vMax + vExit) * td * 0.5f;
  float cruiseDist = length - accelDist - decelDist;

  float tc = 0;
  if (cruiseDist > 0) {
    tc = cruiseDist / vMax;
  } else {
    /* 没有匀速段，重新计算能达到的最大速度 */
    vMax = sqrtf(length * accel + (vEntry*vEntry + vExit*vExit) / 2.0f);
    if (vMax > vPlan) vMax = vPlan;

    float ratio = accelDist / (accelDist + decelDist + 1e-10f);
    ta = ratio * (length * 2.0f / (vEntry + vMax));
    td = (1.0f - ratio) * (length * 2.0f / (vMax + vExit));
    tc = 0;
  }

  if (outTacc) *outTacc = ta;
  if (outTdec) *outTdec = td;

  return ta + tc + td;
}

/* ==================== 规划器 ==================== */

int plc_planner_init(Planner *planner, const PlannerConfig *cfg)
{
  if (!planner || !cfg) return -1;

  memset(planner, 0, sizeof(Planner));
  planner->config = *cfg;

  if (cfg->bufferSize == 0) planner->config.bufferSize = 32;

  planner->buffer = (PlannerSegment *)calloc(planner->config.bufferSize, sizeof(PlannerSegment));
  if (!planner->buffer) return -1;

  planner->bufferSize = planner->config.bufferSize;
  planner->head = 0;
  planner->tail = 0;
  planner->count = 0;
  planner->isEmpty = true;
  planner->isRunning = false;

  return 0;
}

int plc_planner_deinit(Planner *planner)
{
  if (!planner) return -1;
  if (planner->buffer) {
    free(planner->buffer);
    planner->buffer = NULL;
  }
  memset(planner, 0, sizeof(Planner));
  return 0;
}

void plc_planner_setMaxVelocity(Planner *planner, float vMax)
{
  if (planner) planner->config.maxVelocity = vMax;
}

void plc_planner_setAcceleration(Planner *planner, float accel)
{
  if (planner) planner->config.defaultAcceleration = accel;
}

void plc_planner_setJerk(Planner *planner, float jerk)
{
  if (planner) planner->config.defaultJerk = jerk;
}

void plc_planner_setJunctionDeviation(Planner *planner, float dev)
{
  if (planner) planner->config.junctionDeviation = dev;
}

void plc_planner_setTermCondition(Planner *planner, TermCondition cond)
{
  if (planner) planner->config.termCond = cond;
}

/* ==================== 规划运动 ==================== */

static PlannerSegment *next_slot(Planner *planner)
{
  if (planner->count >= planner->bufferSize) return NULL;
  PlannerSegment *seg = &planner->buffer[planner->head];
  planner->head = (planner->head + 1) % planner->bufferSize;
  planner->count++;
  planner->isEmpty = false;
  return seg;
}

int plc_planner_planLinear(Planner *planner, const float *target,
                           float feedRate, float accel, float jerk)
{
  if (!planner || !target) return -1;

  PlannerSegment *seg = next_slot(planner);
  if (!seg) return -CNC_ERR_PLANNER_FULL;

  memset(seg, 0, sizeof(PlannerSegment));
  seg->type = SEGMENT_LINEAR;
  seg->segmentId = planner->plannedCount++;

  /* 计算终点 */
  uint8_t axisMask = 0;
  for (int i = 0; i < 9; i++) {
    if (fabsf(target[i] - planner->currentPosition[i]) > 1e-8f) {
      axisMask |= (1 << i);
    }
    seg->target[i] = target[i];
  }
  seg->axisMask = axisMask;

  /* 计算路径长度和方向 */
  seg->length = plc_plan_length(planner->currentPosition, target, axisMask);
  compute_direction(planner->currentPosition, target, axisMask, seg->direction);

  /* 速度规划参数 */
  seg->acceleration = (accel > 0) ? accel : planner->config.defaultAcceleration;
  seg->jerk = (jerk > 0) ? jerk : planner->config.defaultJerk;
  seg->plannedVelocity = (feedRate > 0) ? feedRate : planner->config.maxVelocity;

  /* 限制全局最大速度 */
  if (seg->plannedVelocity > planner->config.maxVelocity) {
    seg->plannedVelocity = planner->config.maxVelocity;
  }

  /* 入口/出口速度 (由前瞻决定) */
  seg->entryVelocity = planner->currentVelocity;
  seg->exitVelocity = 0;
  seg->junctionVelocity = 1e10f;

  /* 更新规划器当前位置 (用于下一段) */
  for (int i = 0; i < 9; i++) {
    planner->currentPosition[i] = target[i];
  }

  return 0;
}

int plc_planner_planArc(Planner *planner, const float *target,
                        const float *center, float radius, int dir,
                        float feedRate, float accel, float jerk)
{
  if (!planner || !target || !center) return -1;

  PlannerSegment *seg = next_slot(planner);
  if (!seg) return -CNC_ERR_PLANNER_FULL;

  memset(seg, 0, sizeof(PlannerSegment));
  seg->type = (dir > 0) ? SEGMENT_CIRCULAR_CCW : SEGMENT_CIRCULAR_CW;
  seg->segmentId = planner->plannedCount++;

  uint8_t axisMask = 0;
  for (int i = 0; i < 9; i++) {
    if (fabsf(target[i] - planner->currentPosition[i]) > 1e-8f) {
      axisMask |= (1 << i);
    }
    seg->target[i] = target[i];
  }
  seg->axisMask = axisMask;
  seg->center[0] = center[0];
  seg->center[1] = center[1];
  seg->center[2] = center[2];
  seg->radius = radius;
  seg->dir = dir;

  /* 弧长估算 */
  float dx = target[0] - (planner->currentPosition[0] + center[0]);
  float dy = target[1] - (planner->currentPosition[1] + center[1]);
  float startAngle = atan2f(-center[1], -center[0]);  /* 相对中心 */
  float endAngle   = atan2f(dy, dx);
  seg->length = plc_plan_arcLength(radius, startAngle, endAngle);

  seg->acceleration = (accel > 0) ? accel : planner->config.defaultAcceleration;
  seg->jerk = (jerk > 0) ? jerk : planner->config.defaultJerk;
  seg->plannedVelocity = (feedRate > 0) ? feedRate : planner->config.maxVelocity;
  if (seg->plannedVelocity > planner->config.maxVelocity) {
    seg->plannedVelocity = planner->config.maxVelocity;
  }

  seg->entryVelocity = planner->currentVelocity;
  seg->exitVelocity = 0;
  seg->junctionVelocity = 1e10f;

  for (int i = 0; i < 9; i++) {
    planner->currentPosition[i] = target[i];
  }

  return 0;
}

/* ==================== 前瞻规划 ==================== */

int plc_planner_lookAhead(Planner *planner)
{
  if (!planner || planner->count < 2) return 0;

  if (!planner->config.enableLookAhead) {
    /* 无前瞻，简单梯形规划每段 */
    for (uint32_t i = 0; i < planner->count; i++) {
      uint32_t idx = (planner->tail + i) % planner->bufferSize;
      PlannerSegment *seg = &planner->buffer[idx];

      float ta = 0, td = 0;
      seg->exitVelocity = 0;  /* 假设每段都停 */
      seg->cruiseVelocity = seg->plannedVelocity;

      float totalTime = plc_plan_profileTime(
        seg->length, seg->entryVelocity, seg->plannedVelocity, seg->exitVelocity,
        seg->acceleration, seg->jerk, &ta, &td);

      seg->accelerateUntil = (totalTime > 0) ? (ta / totalTime) : 0;
      seg->decelerateAfter = (totalTime > 0) ? (1.0f - td / totalTime) : 1.0f;

      if (seg->accelerateUntil > seg->decelerateAfter) {
        /* 三角形剖面 (无匀速段) */
        seg->cruiseVelocity = seg->plannedVelocity;
      }
    }
    return 0;
  }

  /* 前瞻: 从后往前反向传播速度限制 */
  /* 从最后一段开始，向前计算出口速度限制 */

  /* 1. 计算拐角速度 (需要方向向量) */
  for (uint32_t i = 0; i < planner->count - 1; i++) {
    uint32_t idx0 = (planner->tail + i) % planner->bufferSize;
    uint32_t idx1 = (planner->tail + i + 1) % planner->bufferSize;
    PlannerSegment *seg0 = &planner->buffer[idx0];
    PlannerSegment *seg1 = &planner->buffer[idx1];

    seg0->junctionCos = 0;
    for (int j = 0; j < 3; j++) {
      seg0->junctionCos += seg0->direction[j] * seg1->direction[j];
    }
    seg0->junctionCos = CLAMP(seg0->junctionCos, -1.0f, 1.0f);

    seg0->junctionVelocity = plc_plan_junctionVelocity(
      seg0->direction, seg1->direction,
      MIN(seg0->acceleration, seg1->acceleration),
      planner->config.junctionDeviation);
  }

  /* 最后一段出口速度为 0 */
  if (planner->count > 0) {
    uint32_t lastIdx = (planner->tail + planner->count - 1) % planner->bufferSize;
    planner->buffer[lastIdx].exitVelocity = 0;
    planner->buffer[lastIdx].junctionVelocity = 0;
  }

  /* 2. 反向传播: 每段的出口速度受下段入口速度限制 */
  for (uint32_t i = planner->count - 1; i > 0; i--) {
    uint32_t idx0 = (planner->tail + i - 1) % planner->bufferSize;
    uint32_t idx1 = (planner->tail + i) % planner->bufferSize;
    PlannerSegment *seg0 = &planner->buffer[idx0];
    PlannerSegment *seg1 = &planner->buffer[idx1];

    seg1->entryVelocity = seg0->junctionVelocity;

    /* 检查能否在段内从 entryVelocity 减到 exitVelocity */
    float stopDist = (seg1->entryVelocity * seg1->entryVelocity) / (2.0f * seg1->acceleration);
    if (stopDist < seg1->length) {
      /* 可以在段内停下 */
    } else {
      /* 需要更低的入口速度 */
      float vMax = sqrtf(2.0f * seg1->acceleration * seg1->length);
      if (seg1->entryVelocity > vMax) {
        seg1->entryVelocity = vMax;
        seg0->exitVelocity = vMax;
      }
    }

    seg0->exitVelocity = MIN(seg1->entryVelocity, seg0->junctionVelocity);
  }

  /* 3. 正向计算剖面 */
  for (uint32_t i = 0; i < planner->count; i++) {
    uint32_t idx = (planner->tail + i) % planner->bufferSize;
    PlannerSegment *seg = &planner->buffer[idx];

    if (i > 0) {
      uint32_t prevIdx = (planner->tail + i - 1) % planner->bufferSize;
      seg->entryVelocity = planner->buffer[prevIdx].exitVelocity;
    }

    seg->cruiseVelocity = MIN(seg->plannedVelocity,
      sqrtf(seg->entryVelocity*seg->entryVelocity + 2*seg->acceleration*seg->length));
    seg->cruiseVelocity = MIN(seg->cruiseVelocity, seg->plannedVelocity);

    /* 检查能否加速到巡航速度再减到出口速度 */
    float accelDist = (seg->cruiseVelocity*seg->cruiseVelocity - seg->entryVelocity*seg->entryVelocity) / (2*seg->acceleration);
    float decelDist = (seg->cruiseVelocity*seg->cruiseVelocity - seg->exitVelocity*seg->exitVelocity) / (2*seg->acceleration);
    float totalDist = accelDist + decelDist;

    if (totalDist > seg->length) {
      /* 三角形剖面 */
      float vPeak = sqrtf((seg->length*seg->acceleration) + (seg->entryVelocity*seg->entryVelocity + seg->exitVelocity*seg->exitVelocity)/2.0f);
      seg->cruiseVelocity = MIN(vPeak, seg->plannedVelocity);
      accelDist = (seg->cruiseVelocity*seg->cruiseVelocity - seg->entryVelocity*seg->entryVelocity) / (2*seg->acceleration);
      decelDist = (seg->cruiseVelocity*seg->cruiseVelocity - seg->exitVelocity*seg->exitVelocity) / (2*seg->acceleration);
    }

    seg->accelerateUntil = (seg->length > 0) ? (accelDist / seg->length) : 0;
    seg->decelerateAfter = (seg->length > 0) ? (1.0f - decelDist / seg->length) : 1.0f;
  }

  planner->plannedCount += planner->count;
  return 0;
}

/* ==================== 段管理 ==================== */

int plc_planner_getNext(Planner *planner, PlannerSegment *seg)
{
  if (!planner || !seg) return -1;
  if (planner->count == 0) return -1;

  *seg = planner->buffer[planner->tail];
  planner->tail = (planner->tail + 1) % planner->bufferSize;
  planner->count--;
  if (planner->count == 0) planner->isEmpty = true;
  planner->executedCount++;

  return 0;
}

int plc_planner_clear(Planner *planner)
{
  if (!planner) return -1;
  planner->head = planner->tail;
  planner->count = 0;
  planner->isEmpty = true;
  return 0;
}

uint32_t plc_planner_available(const Planner *planner)
{
  return planner ? (planner->bufferSize - planner->count) : 0;
}

uint32_t plc_planner_queued(const Planner *planner)
{
  return planner ? planner->count : 0;
}

bool plc_planner_isEmpty(const Planner *planner)
{
  return planner ? planner->isEmpty : true;
}

bool plc_planner_isFull(const Planner *planner)
{
  return planner ? (planner->count >= planner->bufferSize) : true;
}

/* ==================== 段插补 ==================== */

void plc_planner_interpolate(const PlannerSegment *seg, float s, float *pos)
{
  if (!seg || !pos) return;

  s = CLAMP(s, 0.0f, 1.0f);

  switch (seg->type) {
  case SEGMENT_LINEAR:
    for (int i = 0; i < 9; i++) {
      if (seg->axisMask & (1 << i)) {
        pos[i] = (seg->target[i] - (seg->target[i] - seg->direction[i] * seg->length * s));
      }
    }
    break;

  case SEGMENT_CIRCULAR_CW:
  case SEGMENT_CIRCULAR_CCW: {
    float angle = s * (seg->length / seg->radius) * seg->dir;
    float c = cosf(angle);
    float sn = sinf(angle);
    /* 绕中心旋转 */
    float rx = -seg->center[0] * c - seg->center[1] * sn * seg->dir;
    float ry = -seg->center[0] * sn * seg->dir + seg->center[1] * c;
    pos[0] = rx + seg->target[0];
    pos[1] = ry + seg->target[1];
    pos[2] = seg->target[2] * s;  /* Z 线性插补 */
    break;
  }
  }
}
