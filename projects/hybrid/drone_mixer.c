#include "plc_px4.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  Px4Controller px4;
  float setpointRoll, setpointPitch, setpointYaw, setpointThrust;
  float actualRoll, actualPitch, actualYaw;
  int stepCount;
} HybridDrone;

HybridDrone *hybrid_drone_create(float dt)
{
  HybridDrone *hd = (HybridDrone *)calloc(1, sizeof(HybridDrone));
  if (!hd) return NULL;
  plc_px4_init(&hd->px4, 4, dt);
  plc_px4_arm(&hd->px4, true);
  return hd;
}

void hybrid_drone_setAttitude(HybridDrone *hd, float roll, float pitch, float yaw, float thrust)
{
  if (!hd) return;
  hd->setpointRoll = roll;
  hd->setpointPitch = pitch;
  hd->setpointYaw = yaw;
  hd->setpointThrust = thrust;
  plc_px4_attCtrlSetpoint(&hd->px4, roll, pitch, yaw, thrust);
}

int hybrid_drone_step(HybridDrone *hd)
{
  if (!hd) return -1;
  hd->stepCount++;
  plc_px4_step(&hd->px4, 0.004f);
  return hd->px4.actuatorCount;
}

const float *hybrid_drone_getActuators(HybridDrone *hd)
{
  return hd ? hd->px4.actuators : NULL;
}

void hybrid_drone_destroy(HybridDrone *hd)
{
  if (hd) {
    plc_px4_arm(&hd->px4, false);
    free(hd);
  }
}
