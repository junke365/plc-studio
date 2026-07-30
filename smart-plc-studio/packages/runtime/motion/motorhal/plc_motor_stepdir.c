#include "plc_motorhal.h"
#include "plc_platform.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ==================== Step/Direction 步进电机驱动 ====================
 *
 * 通过 GPIO 产生步进脉冲和方向信号。
 * 支持: 步进脉冲输出, 方向控制, 使能信号
 * 上层调用 setPos/setVel，驱动内部计算脉冲频率并输出。
 */

typedef struct {
  float pulsePerMm;           /* 脉冲/毫米 */
  float positionMm;           /* 当前位置 (mm) */
  float targetPositionMm;     /* 目标位置 (mm) */
  float velocityMmS;           /* 当前速度 (mm/s) */
  float maxPulseFreq;         /* 最大脉冲频率 (Hz) */
  float pulseFreq;            /* 当前脉冲频率 (Hz) */
  float pulseAccum;           /* 脉冲累积 (小数部分) */
  int32_t stepCounter;        /* 脉冲计数 */
  int32_t targetSteps;        /* 目标步数 */

  uint32_t stepPin;
  uint32_t dirPin;
  uint32_t enablePin;
  bool inverted;
  bool lastStepState;

  /* 软件定时器 */
  float stepTimer;            /* 步进定时器累计 */
  float stepPeriod;           /* 当前步进周期 (秒) */

  /* PID 位置环 (可选) */
  float kp, ki, kd;
  float integral;
  float lastError;
} StepDirPriv;

/* 前向声明 */
static int stepdir_init(MotorHal *motor);
static int stepdir_deinit(MotorHal *motor);
static int stepdir_enable(MotorHal *motor, bool on);
static int stepdir_setPos(MotorHal *motor, float posMm);
static int stepdir_setVel(MotorHal *motor, float velMmS);
static int stepdir_home(MotorHal *motor);
static int stepdir_stop(MotorHal *motor);
static int stepdir_update(MotorHal *motor);
static int stepdir_setPid(MotorHal *motor, float kp, float ki, float kd);

static int stepdir_init(MotorHal *motor)
{
  StepDirPriv *priv = (StepDirPriv *)calloc(1, sizeof(StepDirPriv));
  if (!priv) return -1;

  priv->pulsePerMm = motor->config.drv.stepdir.pulsePerMm;
  if (priv->pulsePerMm <= 0) priv->pulsePerMm = 80.0f;

  priv->stepPin = motor->config.drv.stepdir.stepPin;
  priv->dirPin = motor->config.drv.stepdir.dirPin;
  priv->enablePin = motor->config.drv.stepdir.enablePin;
  priv->maxPulseFreq = motor->config.drv.stepdir.maxPulseFreq;
  if (priv->maxPulseFreq <= 0) priv->maxPulseFreq = 100000;

  priv->positionMm = 0;
  priv->velocityMmS = 0;
  priv->pulseFreq = 0;
  priv->stepPeriod = 0;
  priv->stepTimer = 0;
  priv->kp = 0;
  priv->ki = 0;
  priv->kd = 0;

  motor->priv = priv;

  /* 设置函数表 */
  motor->init   = stepdir_init;
  motor->enable = stepdir_enable;
  motor->setPos = stepdir_setPos;
  motor->setVel = stepdir_setVel;
  motor->home   = stepdir_home;
  motor->stop   = stepdir_stop;
  motor->update = stepdir_update;
  motor->setPid = stepdir_setPid;

  motor->status = MOTOR_STATUS_ENABLED;
  return 0;
}

static int stepdir_deinit(MotorHal *motor)
{
  if (motor && motor->priv) {
    free(motor->priv);
    motor->priv = NULL;
  }
  return 0;
}

static int stepdir_enable(MotorHal *motor, bool on)
{
  if (!motor) return -1;
  motor->status = on ? MOTOR_STATUS_ENABLED : MOTOR_STATUS_DISABLED;
  return 0;
}

static int stepdir_setPos(MotorHal *motor, float posMm)
{
  StepDirPriv *priv = (StepDirPriv *)motor->priv;
  if (!priv) return -1;

  motor->commandPos = posMm;
  priv->targetPositionMm = posMm;

  /* 计算目标步数 */
  priv->targetSteps = (int32_t)(posMm * priv->pulsePerMm);
  priv->stepCounter = 0;

  return 0;
}

static int stepdir_setVel(MotorHal *motor, float velMmS)
{
  StepDirPriv *priv = (StepDirPriv *)motor->priv;
  if (!priv) return -1;

  motor->commandVel = velMmS;
  priv->velocityMmS = velMmS;

  /* 计算脉冲频率 */
  float freq = fabsf(velMmS) * priv->pulsePerMm;
  if (freq > priv->maxPulseFreq) freq = priv->maxPulseFreq;
  priv->pulseFreq = freq;
  priv->stepPeriod = (freq > 0) ? (1.0f / freq) : 0;

  return 0;
}

static int stepdir_home(MotorHal *motor)
{
  (void)motor;
  return 0;
}

static int stepdir_stop(MotorHal *motor)
{
  StepDirPriv *priv = (StepDirPriv *)motor->priv;
  if (!priv) return -1;

  motor->commandVel = 0;
  priv->velocityMmS = 0;
  priv->pulseFreq = 0;
  priv->stepPeriod = 0;

  return 0;
}

static int stepdir_setPid(MotorHal *motor, float kp, float ki, float kd)
{
  StepDirPriv *priv = (StepDirPriv *)motor->priv;
  if (!priv) return -1;
  priv->kp = kp;
  priv->ki = ki;
  priv->kd = kd;
  return 0;
}

static int stepdir_update(MotorHal *motor)
{
  StepDirPriv *priv = (StepDirPriv *)motor->priv;
  if (!priv) return -1;

  /* PID 位置环 (可选) */
  if (priv->kp > 0 || priv->ki > 0 || priv->kd > 0) {
    float error = motor->commandPos - priv->positionMm;
    priv->integral += error * 0.001f;  /* dt=1ms */
    float derivative = (error - priv->lastError) / 0.001f;
    float output = priv->kp * error + priv->ki * priv->integral + priv->kd * derivative;
    priv->lastError = error;

    /* PID 输出转为速度 */
    float velCmd = output;
    float maxVel = motor->config.maxVelocity;
    if (velCmd > maxVel) velCmd = maxVel;
    if (velCmd < -maxVel) velCmd = -maxVel;
    priv->velocityMmS = velCmd;

    /* 更新脉冲频率 */
    float freq = fabsf(velCmd) * priv->pulsePerMm;
    if (freq > priv->maxPulseFreq) freq = priv->maxPulseFreq;
    priv->pulseFreq = freq;
    priv->stepPeriod = (freq > 0) ? (1.0f / freq) : 0;
  }

  /* 步进脉冲输出 */
  if (priv->stepPeriod > 0) {
    priv->stepTimer += 0.001f;  /* dt=1ms */
    if (priv->stepTimer >= priv->stepPeriod) {
      priv->stepTimer -= priv->stepPeriod; /* 保持精度 */

      /* 方向 */
      int dir = (priv->velocityMmS >= 0 || motor->commandPos >= priv->positionMm) ? 1 : -1;

#ifdef PLATFORM_STM32
      /* 真实硬件：调用平台 GPIO 产生脉冲 */
      plc_hal_step_pulse(priv->stepPin, priv->dirPin, dir);
#else
      /* 仿真：软件模拟脉冲 */
      priv->lastStepState = !priv->lastStepState;
      if (priv->lastStepState) {
        priv->stepCounter++;
        priv->positionMm += (float)dir / priv->pulsePerMm;
      }
#endif
    }
  }

  /* 更新反馈 */
  motor->actualPos = priv->positionMm;
  motor->actualVel = priv->velocityMmS;
  motor->followingError = motor->commandPos - motor->actualPos;

  return 0;
}

/* 驱动注册 (由 __attribute__((constructor)) 调用) */
__attribute__((constructor))
static void register_stepdir(void)
{
  plc_motorhal_registerDriver(MOTOR_DRV_STEPDIR, stepdir_init);
}
