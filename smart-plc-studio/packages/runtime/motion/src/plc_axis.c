#include "plc_axis.h"
#include "plc_motion.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

int plc_axis_init(PlcAxis *axis, uint8_t id, const char *name, const MotorConfig *motorCfg)
{
  if (!axis || !name) return -1;

  memset(axis, 0, sizeof(PlcAxis));
  axis->id = id;
  strncpy(axis->name, name, sizeof(axis->name) - 1);

  if (motorCfg) {
    plc_motorhal_init(&axis->motor, motorCfg);
    axis->softLimitPos = motorCfg->softLimitPos;
    axis->softLimitNeg = motorCfg->softLimitNeg;
    axis->followingErrorMax = motorCfg->followingErrorMax;
    axis->homingVelocityFast = motorCfg->homingVelocityFast;
    axis->homingVelocitySlow = motorCfg->homingVelocitySlow;
    axis->homingBackoff = motorCfg->homingBackoff;
    axis->homingDirection = motorCfg->homingDirection > 0 ? 1 : -1;
  }

  axis->inPositionTolerance = 0.01f;  /* 0.01mm 默认到位精度 */
  axis->commandPosition = 0;
  axis->actualPosition = 0;
  axis->homed = false;
  axis->homingState = HOMING_IDLE;
  axis->limitState = AXIS_LIMIT_NONE;

  return 0;
}

int plc_axis_deinit(PlcAxis *axis)
{
  if (!axis) return -1;
  plc_motorhal_deinit(&axis->motor);
  return 0;
}

int plc_axis_enable(PlcAxis *axis, bool on)
{
  if (!axis) return -1;
  axis->enabled = on;
  return axis->motor.enable(&axis->motor, on);
}

int plc_axis_moveAbs(PlcAxis *axis, float position, float vel, float accel)
{
  if (!axis || !axis->enabled) return -1;
  if (axis->fault) return -1;

  /* 检查软限位 */
  if (position > axis->softLimitPos || position < axis->softLimitNeg) {
    return -CNC_ERR_LIMIT;
  }

  axis->targetPosition = position;
  axis->commandVelocity = fabsf(vel);
  return axis->motor.setPos(&axis->motor, position);
}

int plc_axis_moveRel(PlcAxis *axis, float distance, float vel, float accel)
{
  return plc_axis_moveAbs(axis, axis->commandPosition + distance, vel, accel);
}

int plc_axis_moveVel(PlcAxis *axis, float velocity, float accel)
{
  if (!axis || !axis->enabled) return -1;
  axis->commandVelocity = velocity;
  return axis->motor.setVel(&axis->motor, velocity);
}

int plc_axis_stop(PlcAxis *axis)
{
  if (!axis) return -1;
  axis->commandVelocity = 0;
  return axis->motor.stop(&axis->motor);
}

int plc_axis_abort(PlcAxis *axis)
{
  if (!axis) return -1;
  axis->commandVelocity = 0;
  axis->commandPosition = axis->actualPosition;
  if (axis->motor.stop) return axis->motor.stop(&axis->motor);
  return 0;
}

int plc_axis_home(PlcAxis *axis, HomingMode mode)
{
  if (!axis || !axis->enabled) return -1;

  axis->homingMode = mode;
  axis->homingState = HOMING_START;

  if (mode == HOMING_MODE_DIRECT) {
    axis->commandPosition = 0;
    axis->actualPosition = 0;
    axis->homed = true;
    axis->homingState = HOMING_DONE;
    if (axis->onHomeComplete) axis->onHomeComplete(axis, true);
    return 0;
  }

  return axis->motor.home(&axis->motor);
}

int plc_axis_setPos(PlcAxis *axis, float pos)
{
  if (!axis) return -1;
  axis->commandPosition = pos;
  axis->actualPosition = pos;
  return 0;
}

float plc_axis_getPos(PlcAxis *axis)
{
  if (!axis) return 0;
  return axis->actualPosition;
}

float plc_axis_getVel(PlcAxis *axis)
{
  if (!axis) return 0;
  return axis->actualVelocity;
}

void plc_axis_setG92(PlcAxis *axis, float offset)
{
  if (axis) axis->g92Offset = offset;
}

void plc_axis_setToolOffset(PlcAxis *axis, float offset)
{
  if (axis) axis->toolOffset = offset;
}

void plc_axis_clearG92(PlcAxis *axis)
{
  if (axis) axis->g92Offset = 0;
}

int plc_axis_checkLimits(PlcAxis *axis)
{
  if (!axis) return 0;

  AxisLimitState state = AXIS_LIMIT_NONE;

  if (axis->limitPosHard || axis->commandPosition >= axis->softLimitPos) {
    state = AXIS_LIMIT_POS_HARD;
  }
  if (axis->limitNegHard || axis->commandPosition <= axis->softLimitNeg) {
    state = (state == AXIS_LIMIT_POS_HARD) ? AXIS_LIMIT_BOTH : AXIS_LIMIT_NEG_HARD;
  }

  if (axis->limitState != state) {
    axis->limitState = state;
    if (axis->onLimit) axis->onLimit(axis, state);
  }

  return (state != AXIS_LIMIT_NONE) ? -CNC_ERR_LIMIT : 0;
}

void plc_axis_setSoftLimits(PlcAxis *axis, float neg, float pos)
{
  if (axis) {
    axis->softLimitNeg = neg;
    axis->softLimitPos = pos;
  }
}

int plc_axis_update(PlcAxis *axis, float dtSec)
{
  if (!axis || !axis->enabled) return -1;

  /* 更新电机驱动 (读反馈) */
  if (axis->motor.update) {
    axis->motor.update(&axis->motor);
  }

  /* 更新位置反馈 */
  axis->actualPosition = axis->motor.actualPos;
  axis->actualVelocity = axis->motor.actualVel;

  /* 计算跟随误差 */
  axis->followingError = axis->commandPosition - axis->actualPosition;

  /* 跟随误差报警 */
  if (axis->followingErrorMax > 0 &&
      fabsf(axis->followingError) > axis->followingErrorMax) {
    axis->followingErrorFault = true;
    axis->fault = true;
  }

  /* 到位判定 */
  axis->inPosition = (fabsf(axis->targetPosition - axis->actualPosition) < axis->inPositionTolerance);

  /* 检查限位 */
  plc_axis_checkLimits(axis);

  return 0;
}

bool plc_axis_isHomed(PlcAxis *axis)
{
  return axis ? axis->homed : false;
}

bool plc_axis_isEnabled(PlcAxis *axis)
{
  return axis ? axis->enabled : false;
}

bool plc_axis_isFault(PlcAxis *axis)
{
  return axis ? axis->fault : false;
}

bool plc_axis_inPosition(PlcAxis *axis)
{
  return axis ? axis->inPosition : false;
}

const char *plc_axis_homingStateStr(HomingState state)
{
  static const char *names[] = {
    "IDLE", "START", "MOVE_TO_SWITCH", "BACKOFF",
    "MOVE_TO_INDEX", "SET_REF", "DONE", "FAILED"
  };
  if (state < 0 || state >= (int)(sizeof(names)/sizeof(names[0]))) return "?";
  return names[state];
}
