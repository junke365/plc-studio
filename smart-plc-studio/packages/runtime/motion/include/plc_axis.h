#ifndef PLC_AXIS_H
#define PLC_AXIS_H

#include "plc_motorhal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 轴限位状态 ==================== */
typedef enum {
  AXIS_LIMIT_NONE = 0,
  AXIS_LIMIT_POS_HARD,
  AXIS_LIMIT_NEG_HARD,
  AXIS_LIMIT_POS_SOFT,
  AXIS_LIMIT_NEG_SOFT,
  AXIS_LIMIT_BOTH,
} AxisLimitState;

/* ==================== 轴回零状态 ==================== */
typedef enum {
  HOMING_IDLE = 0,
  HOMING_START,
  HOMING_MOVE_TO_SWITCH,    /* 快速移动到限位开关 */
  HOMING_BACKOFF,            /* 回退离开关 */
  HOMING_MOVE_TO_INDEX,      /* 慢速找 Z 脉冲/index */
  HOMING_SET_REF,            /* 设定参考点 */
  HOMING_DONE,
  HOMING_FAILED,
} HomingState;

/* ==================== 回零模式 ==================== */
typedef enum {
  HOMING_MODE_NONE = 0,
  HOMING_MODE_LIMIT,         /* 找限位开关 */
  HOMING_MODE_HOME_SWITCH,   /* 找 Home 开关 */
  HOMING_MODE_INDEX,         /* 找编码器 Z 脉冲 */
  HOMING_MODE_LIMIT_INDEX,   /* 限位开关 + Z 脉冲 */
  HOMING_MODE_HOME_INDEX,    /* Home 开关 + Z 脉冲 */
  HOMING_MODE_DIRECT,        /* 直接设定当前位置为原点 */
} HomingMode;

/* ==================== 轴数据结构 ==================== */
typedef struct PlcAxis PlcAxis;

struct PlcAxis {
  uint8_t id;                 /* 轴 ID (0-31) */
  char name[16];              /* 轴名 (X, Y, Z, A, B, C, U, V, W) */

  /* 电机驱动 */
  MotorHal motor;

  /* 位置 */
  float commandPosition;      /* 指令位置 (mm/deg) */
  float actualPosition;       /* 实际位置 (mm/deg) */
  float commandVelocity;      /* 指令速度 */
  float actualVelocity;       /* 实际速度 */
  float targetPosition;       /* 目标位置 (规划终点) */

  /* 偏移 */
  float toolOffset;           /* 刀具长度偏移 */
  float g92Offset;            /* G92 坐标系偏移 */
  float homeOffset;           /* 回零后原点偏移 */

  /* 限位 */
  float softLimitPos;         /* 正向软限位 */
  float softLimitNeg;         /* 负向软限位 */
  bool limitPosHard;          /* 正限位开关输入 */
  bool limitNegHard;          /* 负限位开关输入 */
  bool homeSwitch;            /* Home 开关输入 */
  AxisLimitState limitState;

  /* 回零 */
  HomingState homingState;
  HomingMode homingMode;
  float homingVelocityFast;
  float homingVelocitySlow;
  float homingBackoff;
  int8_t homingDirection;     /* ±1 */
  bool homed;                 /* 是否已回零 */

  /* 跟随误差 */
  float followingError;
  float followingErrorMax;
  bool followingErrorFault;

  /* 状态 */
  bool enabled;
  bool fault;
  bool inPosition;            /* 到达目标位置 */
  float inPositionTolerance;  /* 到位判定公差 */

  /* 回调 */
  void (*onLimit)(PlcAxis *axis, AxisLimitState state);
  void (*onHomeComplete)(PlcAxis *axis, bool success);
};

/* ==================== 轴 API ==================== */
int plc_axis_init(PlcAxis *axis, uint8_t id, const char *name, const MotorConfig *motorCfg);
int plc_axis_deinit(PlcAxis *axis);

/* 使能/禁能 */
int plc_axis_enable(PlcAxis *axis, bool on);

/* 运动指令 */
int plc_axis_moveAbs(PlcAxis *axis, float position, float vel, float accel);
int plc_axis_moveRel(PlcAxis *axis, float distance, float vel, float accel);
int plc_axis_moveVel(PlcAxis *axis, float velocity, float accel);
int plc_axis_stop(PlcAxis *axis);
int plc_axis_abort(PlcAxis *axis); /* 急停 */

/* 回零 */
int plc_axis_home(PlcAxis *axis, HomingMode mode);

/* 设置/获取位置 */
int plc_axis_setPos(PlcAxis *axis, float pos);
float plc_axis_getPos(PlcAxis *axis);
float plc_axis_getVel(PlcAxis *axis);

/* 偏移管理 */
void plc_axis_setG92(PlcAxis *axis, float offset);
void plc_axis_setToolOffset(PlcAxis *axis, float offset);
void plc_axis_clearG92(PlcAxis *axis);

/* 限位 */
int plc_axis_checkLimits(PlcAxis *axis);
void plc_axis_setSoftLimits(PlcAxis *axis, float neg, float pos);

/* 周期性更新 (伺服循环中调用) */
int plc_axis_update(PlcAxis *axis, float dtSec);

/* 状态查询 */
bool plc_axis_isHomed(PlcAxis *axis);
bool plc_axis_isEnabled(PlcAxis *axis);
bool plc_axis_isFault(PlcAxis *axis);
bool plc_axis_inPosition(PlcAxis *axis);
const char *plc_axis_homingStateStr(HomingState state);

#ifdef __cplusplus
}
#endif

#endif /* PLC_AXIS_H */
