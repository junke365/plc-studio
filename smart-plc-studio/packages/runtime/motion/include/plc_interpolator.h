#ifndef PLC_INTERPOLATOR_H
#define PLC_INTERPOLATOR_H

#include "plc_planner.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 插补器模式 ==================== */
typedef enum {
  INTERP_MODE_IDLE = 0,
  INTERP_MODE_RUNNING,
  INTERP_MODE_HOLD,                   /* 进给保持 */
  INTERP_MODE_RESUMING,               /* 恢复中 */
  INTERP_MODE_STOP,                   /* 急停 */
} InterpolatorMode;

/* ==================== 插补器状态 ==================== */
typedef struct {
  /* 当前段 */
  PlannerSegment currentSegment;
  bool hasSegment;

  /* 段进度 */
  float segmentLength;                /* 段总长 */
  float segmentProgress;              /* 已执行距离 */
  float segmentTime;                  /* 段总时间 */
  float elapsedTime;                  /* 段已用时间 */
  float currentS;                     /* 参数化位置 [0..1] */
  float currentVelocity;              /* 当前速度 */

  /* 插补输出 */
  float outputPosition[9];            /* 插补后的位置 (关节空间) */

  /* 状态 */
  InterpolatorMode mode;
  float cycleTime;                    /* 插补周期 (秒) */
  uint32_t stepCount;                 /* 总步数 */
  bool usePositionMode;               /* 位置模式 vs 速度模式 */
} Interpolator;

/* ==================== 插补器 API ==================== */

/* 初始化 */
void plc_interp_init(Interpolator *interp, float cycleTime);

/* 加载新段 */
int plc_interp_loadSegment(Interpolator *interp, const PlannerSegment *seg);

/* 单步执行 (每伺服周期调用) */
int plc_interp_step(Interpolator *interp, float *jointPos, uint8_t *axisCount);

/* 控制 */
void plc_interp_start(Interpolator *interp);
void plc_interp_hold(Interpolator *interp);
void plc_interp_resume(Interpolator *interp);
void plc_interp_stop(Interpolator *interp);
void plc_interp_reset(Interpolator *interp);

/* 状态查询 */
bool plc_interp_isIdle(const Interpolator *interp);
bool plc_interp_isRunning(const Interpolator *interp);
float plc_interp_getProgress(const Interpolator *interp);
float plc_interp_getVelocity(const Interpolator *interp);
InterpolatorMode plc_interp_getMode(const Interpolator *interp);

/* ==================== S 曲线剖面计算 ==================== */
typedef struct {
  float accelTime;                    /* 加速时间 */
  float decelTime;                    /* 减速时间 */
  float cruiseTime;                   /* 匀速时间 */
  float totalTime;                    /* 总时间 */
  float maxReachedVel;                /* 实际达到的最大速度 */
} SCurveProfile;

int plc_interp_computeSCurve(const PlannerSegment *seg, SCurveProfile *profile);

/* 根据 S 曲线参数化位置 s [0..1] 求速度 */
float plc_interp_sCurveVelocity(const PlannerSegment *seg, float s);

#ifdef __cplusplus
}
#endif

#endif /* PLC_INTERPOLATOR_H */
