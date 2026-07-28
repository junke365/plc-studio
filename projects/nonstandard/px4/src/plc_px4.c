#include "plc_px4.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define PX4_MAX_CALLBACKS 16

typedef struct {
  Px4UorbTopic topic;
  void (*callback)(Px4UorbMsg *msg, void *userData);
  void *userData;
} Px4SubCallback;

void plc_px4_init(Px4Controller *px4, int actuatorCount, float dt)
{
  memset(px4, 0, sizeof(Px4Controller));
  px4->actuatorCount = (actuatorCount > PX4_MAX_ACTUATORS) ? PX4_MAX_ACTUATORS : actuatorCount;
  px4->dt = dt;
  px4->throttle = 0;
  px4->attCtrl.rollP = 6.0f; px4->attCtrl.rollI = 0.1f; px4->attCtrl.rollD = 0.3f;
  px4->attCtrl.pitchP = 6.0f; px4->attCtrl.pitchI = 0.1f; px4->attCtrl.pitchD = 0.3f;
  px4->attCtrl.yawP = 4.0f; px4->attCtrl.yawI = 0.05f; px4->attCtrl.yawD = 0.2f;
  px4->attCtrl.rateLimit[0] = 200; px4->attCtrl.rateLimit[1] = 200; px4->attCtrl.rateLimit[2] = 150;
  px4->attCtrl.outputLimit = 1.0f;
  plc_px4_mixerInit(&px4->mixer, actuatorCount);
}

int plc_px4_publish(Px4Controller *px4, Px4UorbMsg *msg)
{
  if (!px4 || !msg) return -1;
  for (int i = 0; i < px4->sensorCount; i++) {
    if (px4->sensors[i].topic == msg->topic) {
      px4->sensors[i] = *msg;
      px4->sensors[i].timestamp = msg->timestamp;
      goto notify;
    }
  }
  if (px4->sensorCount >= PX4_MAX_SENSORS) return -1;
  px4->sensors[px4->sensorCount++] = *msg;

notify:
  for (int i = 0; i < px4->subCount; i++) {
    Px4SubCallback *cb = (Px4SubCallback *)px4->uorbSubs[i];
    if (cb && cb->topic == msg->topic && cb->callback) {
      cb->callback(msg, cb->userData);
    }
  }
  return 0;
}

int plc_px4_subscribe(Px4Controller *px4, Px4UorbTopic topic, void *callback)
{
  if (!px4 || !callback) return -1;
  if (px4->subCount >= PX4_MAX_SENSORS) return -1;
  Px4SubCallback *cb = (Px4SubCallback *)calloc(1, sizeof(Px4SubCallback));
  if (!cb) return -1;
  cb->topic = topic;
  cb->callback = (void (*)(Px4UorbMsg *, void *))callback;
  cb->userData = px4;
  px4->uorbSubs[px4->subCount++] = cb;
  return 0;
}

int plc_px4_getMsg(Px4Controller *px4, Px4UorbTopic topic, Px4UorbMsg *out)
{
  if (!px4 || !out) return -1;
  for (int i = 0; i < px4->sensorCount; i++) {
    if (px4->sensors[i].topic == topic) {
      *out = px4->sensors[i];
      return 0;
    }
  }
  return -1;
}

void plc_px4_attCtrlSetpoint(Px4Controller *px4, float roll, float pitch, float yaw, float thrust)
{
  if (!px4) return;
  px4->attCtrl.rollAngle = roll;
  px4->attCtrl.pitchAngle = pitch;
  px4->attCtrl.yawAngle = yaw;
  px4->throttle = thrust;
}

void plc_px4_attCtrlUpdate(Px4Controller *px4)
{
  if (!px4 || !px4->armed) return;
  Px4AttitudeCtrl *ctrl = &px4->attCtrl;
  float dt = px4->dt;

  Px4UorbMsg attMsg;
  if (plc_px4_getMsg(px4, UORB_VEHICLE_ATTITUDE, &attMsg) == 0) {
    float fbRoll = attMsg.data.attitude.roll;
    float fbPitch = attMsg.data.attitude.pitch;
    float fbYaw = attMsg.data.attitude.yaw;

    float errRoll = ctrl->rollAngle - fbRoll;
    float errPitch = ctrl->pitchAngle - fbPitch;
    float errYaw = ctrl->yawAngle - fbYaw;

    Px4UorbMsg gyroMsg;
    float gyroX = 0, gyroY = 0, gyroZ = 0;
    if (plc_px4_getMsg(px4, UORB_SENSOR_GYRO, &gyroMsg) == 0) {
      gyroX = gyroMsg.data.gyro.x;
      gyroY = gyroMsg.data.gyro.y;
      gyroZ = gyroMsg.data.gyro.z;
    }

    ctrl->rollRateInteg  += errRoll * ctrl->rollI * dt;
    ctrl->pitchRateInteg += errPitch * ctrl->pitchI * dt;
    ctrl->yawRateInteg   += errYaw * ctrl->yawI * dt;

    float iLimit = 0.5f;
    if (ctrl->rollRateInteg > iLimit) ctrl->rollRateInteg = iLimit;
    if (ctrl->rollRateInteg < -iLimit) ctrl->rollRateInteg = -iLimit;
    if (ctrl->pitchRateInteg > iLimit) ctrl->pitchRateInteg = iLimit;
    if (ctrl->pitchRateInteg < -iLimit) ctrl->pitchRateInteg = -iLimit;
    if (ctrl->yawRateInteg > iLimit) ctrl->yawRateInteg = iLimit;
    if (ctrl->yawRateInteg < -iLimit) ctrl->yawRateInteg = -iLimit;

    ctrl->rollRate  = errRoll * ctrl->rollP + ctrl->rollRateInteg - gyroX * ctrl->rollD;
    ctrl->pitchRate = errPitch * ctrl->pitchP + ctrl->pitchRateInteg - gyroY * ctrl->pitchD;
    ctrl->yawRate   = errYaw * ctrl->yawP + ctrl->yawRateInteg - gyroZ * ctrl->yawD;

    if (ctrl->rollRate > ctrl->rateLimit[0]) ctrl->rollRate = ctrl->rateLimit[0];
    if (ctrl->rollRate < -ctrl->rateLimit[0]) ctrl->rollRate = -ctrl->rateLimit[0];
    if (ctrl->pitchRate > ctrl->rateLimit[1]) ctrl->pitchRate = ctrl->rateLimit[1];
    if (ctrl->pitchRate < -ctrl->rateLimit[1]) ctrl->pitchRate = -ctrl->rateLimit[1];
    if (ctrl->yawRate > ctrl->rateLimit[2]) ctrl->yawRate = ctrl->rateLimit[2];
    if (ctrl->yawRate < -ctrl->rateLimit[2]) ctrl->yawRate = -ctrl->rateLimit[2];
  }

  plc_px4_mixerUpdate(&px4->mixer, ctrl->rollRate, ctrl->pitchRate, ctrl->yawRate, px4->throttle, px4->actuators);
}

void plc_px4_attCtrlSetPid(Px4Controller *px4, float roll[3], float pitch[3], float yaw[3])
{
  if (!px4) return;
  Px4AttitudeCtrl *c = &px4->attCtrl;
  c->rollP = roll[0]; c->rollI = roll[1]; c->rollD = roll[2];
  c->pitchP = pitch[0]; c->pitchI = pitch[1]; c->pitchD = pitch[2];
  c->yawP = yaw[0]; c->yawI = yaw[1]; c->yawD = yaw[2];
}

void plc_px4_mixerInit(Px4Mixer *mixer, int actuators)
{
  memset(mixer, 0, sizeof(Px4Mixer));
  mixer->actuatorCount = actuators;
  mixer->rollScale = 1.0f;
  mixer->pitchScale = 1.0f;
  mixer->yawScale = 1.0f;
  mixer->thrustScale = 1.0f;
  mixer->idleSpeed = 0.1f;
  if (actuators >= 4) {
    float m[4][4] = {
      { 0.5f,  0.5f,  0.5f,  0.5f},
      { 0.5f, -0.5f, -0.5f,  0.5f},
      { 0.5f,  0.5f, -0.5f, -0.5f},
      { 0.5f, -0.5f,  0.5f, -0.5f},
    };
    for (int i = 0; i < actuators && i < 4; i++)
      for (int j = 0; j < 4; j++)
        mixer->actMatrix[i][j] = m[i][j];
  }
}

void plc_px4_mixerUpdate(Px4Mixer *mixer, float roll, float pitch, float yaw, float thrust, float *output)
{
  for (int i = 0; i < mixer->actuatorCount; i++) {
    float v = mixer->actMatrix[i][0] * roll * mixer->rollScale
            + mixer->actMatrix[i][1] * pitch * mixer->pitchScale
            + mixer->actMatrix[i][2] * yaw * mixer->yawScale
            + mixer->actMatrix[i][3] * thrust * mixer->thrustScale;
    if (v < mixer->idleSpeed) v = mixer->idleSpeed;
    if (v > 1.0f) v = 1.0f;
    output[i] = v;
  }
}

void plc_px4_arm(Px4Controller *px4, bool arm)
{
  if (!px4) return;
  px4->armed = arm;
  if (!arm) {
    for (int i = 0; i < px4->actuatorCount; i++)
      px4->actuators[i] = 0;
    memset(&px4->attCtrl, 0, sizeof(Px4AttitudeCtrl));
  }
}

bool plc_px4_isArmed(const Px4Controller *px4)
{
  return px4 ? px4->armed : false;
}

static void vehicleDynamics6Dof(Px4Controller *px4, float dt)
{
  float roll = 0, pitch = 0, yaw = 0;
  Px4UorbMsg attMsg;
  if (plc_px4_getMsg(px4, UORB_VEHICLE_ATTITUDE, &attMsg) == 0) {
    roll = attMsg.data.attitude.roll;
    pitch = attMsg.data.attitude.pitch;
    yaw = attMsg.data.attitude.yaw;
  }

  Px4UorbMsg rateMsg;
  float wx = 0, wy = 0, wz = 0;
  if (plc_px4_getMsg(px4, UORB_VEHICLE_RATE_SETPOINT, &rateMsg) == 0) {
    wx = rateMsg.data.rateSetpoint.rates[0];
    wy = rateMsg.data.rateSetpoint.rates[1];
    wz = rateMsg.data.rateSetpoint.rates[2];
  }
  wx = px4->attCtrl.rollRate;
  wy = px4->attCtrl.pitchRate;
  wz = px4->attCtrl.yawRate;

  float cr = cosf(roll), sr = sinf(roll);
  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), sy = sinf(yaw);
  float p = wx + (wy * sr + wz * cr) * sp / cp;
  float q = wy * cr - wz * sr;
  float r = (wy * sr + wz * cr) / cp;

  roll += p * dt;
  pitch += q * dt;
  yaw += r * dt;

  Px4UorbMsg attOut;
  memset(&attOut, 0, sizeof(attOut));
  attOut.topic = UORB_VEHICLE_ATTITUDE;
  attOut.timestamp = (uint64_t)(dt * 1e9f);
  attOut.data.attitude.roll = roll;
  attOut.data.attitude.pitch = pitch;
  attOut.data.attitude.yaw = yaw;
  plc_px4_publish(px4, &attOut);

  Px4UorbMsg gyroOut;
  memset(&gyroOut, 0, sizeof(gyroOut));
  gyroOut.topic = UORB_SENSOR_GYRO;
  gyroOut.timestamp = attOut.timestamp;
  gyroOut.data.gyro.x = wx;
  gyroOut.data.gyro.y = wy;
  gyroOut.data.gyro.z = wz;
  plc_px4_publish(px4, &gyroOut);

  Px4UorbMsg actOut;
  memset(&actOut, 0, sizeof(actOut));
  actOut.topic = UORB_ACTUATOR_OUTPUT;
  actOut.data.actuator.count = px4->actuatorCount;
  for (int i = 0; i < px4->actuatorCount; i++)
    actOut.data.actuator.output[i] = px4->actuators[i];
  plc_px4_publish(px4, &actOut);
}

void plc_px4_step(Px4Controller *px4, float dt)
{
  if (!px4) return;
  px4->dt = dt;
  plc_px4_attCtrlUpdate(px4);
  vehicleDynamics6Dof(px4, dt);
}

int plc_px4_bridgeInit(Px4NativeBridge *bridge, const char *px4Path)
{
  if (!bridge || !px4Path) return -1;
  bridge->init = NULL;
  bridge->run = NULL;
  bridge->stop = NULL;
  bridge->sendExternal = NULL;
  bridge->recvExternal = NULL;
  bridge->handle = NULL;
  return 0;
}
