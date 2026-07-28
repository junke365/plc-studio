#include "plc_motorhal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ==================== 虚拟仿真电机驱动 ====================
 *
 * 连接到仿真器中的虚拟轴，模拟伺服电机行为。
 * 支持: 位置模式、速度模式、扭矩模式
 * 不需要真实硬件，位置/速度通过仿真器积分更新。
 */

typedef struct {
  float positionMm;
  float velocityMmS;
  float torqueNm;
  float inertia;              /* 虚拟惯量 */
  float friction;             /* 虚拟摩擦 */
  float maxTorque;
  float positionKp;
  float velocityKp;
  float velocityKi;
  float velIntegral;
  float lastVelocity;
  bool enableSim;             /* 是否启用仿真 */
} SimMotorPriv;

static int sim_init(MotorHal *motor);
static int sim_deinit(MotorHal *motor);
static int sim_enable(MotorHal *motor, bool on);
static int sim_setPos(MotorHal *motor, float posMm);
static int sim_setVel(MotorHal *motor, float velMmS);
static int sim_setTorque(MotorHal *motor, float torque);
static int sim_home(MotorHal *motor);
static int sim_stop(MotorHal *motor);
static int sim_update(MotorHal *motor);
static int sim_setPid(MotorHal *motor, float kp, float ki, float kd);

static int sim_init(MotorHal *motor)
{
  SimMotorPriv *priv = (SimMotorPriv *)calloc(1, sizeof(SimMotorPriv));
  if (!priv) return -1;

  priv->inertia = 0.001f;
  priv->friction = 0.01f;
  priv->maxTorque = 1.0f;
  priv->positionKp = 50.0f;
  priv->velocityKp = 10.0f;
  priv->velocityKi = 0.1f;
  priv->enableSim = true;

  motor->priv = priv;

  motor->init   = sim_init;
  motor->enable = sim_enable;
  motor->setPos = sim_setPos;
  motor->setVel = sim_setVel;
  motor->setTorque = sim_setTorque;
  motor->home   = sim_home;
  motor->stop   = sim_stop;
  motor->update = sim_update;
  motor->setPid = sim_setPid;

  motor->status = MOTOR_STATUS_ENABLED;
  return 0;
}

static int sim_deinit(MotorHal *motor)
{
  if (motor && motor->priv) {
    free(motor->priv);
    motor->priv = NULL;
  }
  return 0;
}

static int sim_enable(MotorHal *motor, bool on)
{
  if (!motor) return -1;
  motor->status = on ? MOTOR_STATUS_ENABLED : MOTOR_STATUS_DISABLED;
  return 0;
}

static int sim_setPos(MotorHal *motor, float posMm)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  motor->commandPos = posMm;
  return 0;
}

static int sim_setVel(MotorHal *motor, float velMmS)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  motor->commandVel = velMmS;
  priv->velocityMmS = velMmS;
  return 0;
}

static int sim_setTorque(MotorHal *motor, float torque)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  priv->torqueNm = torque;
  return 0;
}

static int sim_home(MotorHal *motor)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  priv->positionMm = 0;
  motor->actualPos = 0;
  motor->commandPos = 0;
  return 0;
}

static int sim_stop(MotorHal *motor)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  motor->commandVel = 0;
  priv->velocityMmS = 0;
  return 0;
}

static int sim_update(MotorHal *motor)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;

  float dt = 0.001f;

  if (priv->enableSim) {
    /* 位置环: 计算速度指令 */
    float posError = motor->commandPos - priv->positionMm;
    float velCmd = posError * priv->positionKp;

    /* 速度限幅 */
    float maxVel = motor->config.maxVelocity;
    if (maxVel > 0) {
      if (velCmd > maxVel) velCmd = maxVel;
      if (velCmd < -maxVel) velCmd = -maxVel;
    }

    /* 速度环: PI */
    float velError = velCmd - priv->velocityMmS;
    priv->velIntegral += velError * dt;
    if (priv->velIntegral > 100) priv->velIntegral = 100;
    if (priv->velIntegral < -100) priv->velIntegral = -100;
    float torqueCmd = velError * priv->velocityKp + priv->velIntegral * priv->velocityKi;

    /* 扭矩限幅 */
    if (torqueCmd > priv->maxTorque) torqueCmd = priv->maxTorque;
    if (torqueCmd < -priv->maxTorque) torqueCmd = -priv->maxTorque;

    /* 动力学: 加速度 = (扭矩 - 摩擦*速度) / 惯量 */
    float accel = (torqueCmd - priv->friction * priv->velocityMmS) / priv->inertia;

    /* 积分 */
    priv->velocityMmS += accel * dt;
    priv->positionMm += priv->velocityMmS * dt;
  }

  /* 更新反馈 */
  motor->actualPos = priv->positionMm;
  motor->actualVel = priv->velocityMmS;
  motor->followingError = motor->commandPos - motor->actualPos;

  return 0;
}

static int sim_setPid(MotorHal *motor, float kp, float ki, float kd)
{
  SimMotorPriv *priv = (SimMotorPriv *)motor->priv;
  if (!priv) return -1;
  priv->positionKp = kp;
  priv->velocityKp = ki;
  (void)kd;
  return 0;
}

__attribute__((constructor))
static void register_sim(void)
{
  plc_motorhal_registerDriver(MOTOR_DRV_NONE, sim_init);
}
