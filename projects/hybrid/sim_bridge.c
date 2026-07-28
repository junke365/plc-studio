#include "plc_simulation.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  SimulationSystem sim;
  int robotIdx;
  float stateBuffer[256];
  int stateLen;
} HybridSim;

HybridSim *hybrid_sim_create(float dt)
{
  HybridSim *hs = (HybridSim *)calloc(1, sizeof(HybridSim));
  if (!hs) return NULL;

  SimConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.dt = dt;
  cfg.gravity = 9.81f;
  cfg.solverIterations = 10;
  cfg.enableCollision = true;
  cfg.enableSoftBody = true;
  cfg.enableMultibody = true;

  plc_sim_init(&hs->sim, &cfg);
  plc_sim_start(&hs->sim);
  return hs;
}

int hybrid_sim_addRobot(HybridSim *hs, SurgicalRobotType type)
{
  if (!hs) return -1;
  SurgicalRobot robot;
  if (plc_surgical_init(&robot, type) != 0) return -1;
  hs->robotIdx = plc_sim_addSurgicalRobot(&hs->sim, &robot);
  return hs->robotIdx;
}

int hybrid_sim_step(HybridSim *hs)
{
  if (!hs) return -1;
  plc_sim_step(&hs->sim);
  return 0;
}

int hybrid_sim_export(HybridSim *hs)
{
  if (!hs) return -1;
  hs->stateLen = plc_sim_exportState(&hs->sim, hs->stateBuffer, 256);
  return hs->stateLen;
}

int hybrid_sim_import(HybridSim *hs, const float *buf, int len)
{
  if (!hs || !buf) return -1;
  return plc_sim_importState(&hs->sim, buf, len);
}

void hybrid_sim_destroy(HybridSim *hs)
{
  if (hs) {
    plc_sim_stop(&hs->sim);
    plc_sim_deinit(&hs->sim);
    free(hs);
  }
}
