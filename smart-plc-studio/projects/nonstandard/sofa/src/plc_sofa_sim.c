#include "plc_sofa_sim.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int sofa_sim_init(SofaSimulator *ss, int port)
{
  memset(ss, 0, sizeof(SofaSimulator));

  SimConfig cfg;
  memset(&cfg, 0, sizeof(SimConfig));
  cfg.dt = 0.001f;
  cfg.gravity = 9.81f;
  cfg.solverIterations = 10;
  cfg.enableCollision = true;
  cfg.enableSoftBody = true;
  cfg.enableMultibody = true;

  int ret = plc_sim_init(&ss->sim, &cfg);
  if (ret != SIM_OK) return ret;

  ss->port = port;
  ss->serverFd = -1;
  ss->running = false;
  ss->clientConnected = false;
  ss->stateLen = 0;
#ifdef _WIN32
  ss->wsaInit = false;
#endif

  return SIM_OK;
}

void sofa_sim_deinit(SofaSimulator *ss)
{
  if (!ss) return;
  sofa_sim_stop(ss);
  plc_sim_deinit(&ss->sim);
  memset(ss, 0, sizeof(SofaSimulator));
}

int sofa_sim_start(SofaSimulator *ss)
{
  if (!ss) return SIM_ERR_PARAM;
  ss->running = true;
  return plc_sim_start(&ss->sim);
}

void sofa_sim_stop(SofaSimulator *ss)
{
  if (!ss) return;
  ss->running = false;
  plc_sim_stop(&ss->sim);
}

int sofa_sim_step(SofaSimulator *ss)
{
  if (!ss) return SIM_ERR_PARAM;

  int ret = plc_sim_step(&ss->sim);
  if (ret != SIM_OK) return ret;

  /* 步进后自动刷新状态缓存 */
  ss->stateLen = plc_sim_exportState(&ss->sim, ss->stateBuffer, 2048);
  if (ss->stateLen < 0) return SIM_ERR_STATE;

  return SIM_OK;
}

int sofa_sim_addSurgical(SofaSimulator *ss, SurgicalRobotType type)
{
  if (!ss) return SIM_ERR_PARAM;

  SurgicalRobot robot;
  memset(&robot, 0, sizeof(SurgicalRobot));

  int ret = plc_surgical_init(&robot, type);
  if (ret != SIM_OK) return ret;

  /* 加载默认参数 */
  if (type == SURGICAL_MTM) {
    plc_surgical_loadMtmParams(&robot.params.mtm);
  } else if (type == SURGICAL_PSM) {
    plc_surgical_loadPsmParams(&robot.params.psm);
  }

  return plc_sim_addSurgicalRobot(&ss->sim, &robot);
}

const float *sofa_sim_getState(SofaSimulator *ss, int *len)
{
  if (!ss || !len) return NULL;

  /* 刷新状态缓存 */
  int ret = plc_sim_exportState(&ss->sim, ss->stateBuffer, 2048);
  if (ret < 0) {
    *len = 0;
    return NULL;
  }
  ss->stateLen = ret;
  *len = ss->stateLen;
  return ss->stateBuffer;
}

int sofa_sim_applyState(SofaSimulator *ss, const float *buffer, int len)
{
  if (!ss || !buffer) return SIM_ERR_PARAM;
  return plc_sim_importState(&ss->sim, buffer, len);
}
