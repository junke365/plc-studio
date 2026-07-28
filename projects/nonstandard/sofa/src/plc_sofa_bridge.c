#include "plc_sofa_bridge.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int plc_sofa_bridgeInit(SofaBridge *bridge)
{
  memset(bridge, 0, sizeof(SofaBridge));
  bridge->socket = -1;
  bridge->dt = 0.01f;
  return 0;
}

int plc_sofa_connect(SofaBridge *bridge, const char *host, int port)
{
  if (!bridge || !host) return -1;
  strncpy(bridge->host, host, sizeof(bridge->host) - 1);
  bridge->port = port;
  bridge->isConnected = true;
  return 0;
}

int plc_sofa_disconnect(SofaBridge *bridge)
{
  if (!bridge) return -1;
  bridge->isConnected = false;
  bridge->socket = -1;
  return 0;
}

bool plc_sofa_isConnected(const SofaBridge *bridge)
{
  return bridge ? bridge->isConnected : false;
}

int plc_sofa_sendState(SofaBridge *bridge, const float *pos, const float *vel, int dof)
{
  (void)bridge;
  (void)pos;
  (void)vel;
  (void)dof;
  return 0;
}

int plc_sofa_recvState(SofaBridge *bridge, float *pos, float *vel, int *dof)
{
  (void)bridge;
  (void)pos;
  (void)vel;
  (void)dof;
  return 0;
}

int plc_sofa_sendCommand(SofaBridge *bridge, const char *cmd)
{
  (void)bridge;
  (void)cmd;
  return 0;
}

int plc_sofa_start(SofaBridge *bridge)
{
  if (!bridge) return -1;
  bridge->isPaused = false;
  return 0;
}

int plc_sofa_pause(SofaBridge *bridge)
{
  if (!bridge) return -1;
  bridge->isPaused = true;
  return 0;
}

int plc_sofa_step(SofaBridge *bridge, float dt)
{
  (void)bridge;
  (void)dt;
  return 0;
}

int plc_sofa_reset(SofaBridge *bridge)
{
  (void)bridge;
  return 0;
}

int plc_sofa_getMesh(SofaBridge *bridge, int meshId, SofaMesh *out)
{
  (void)bridge;
  (void)meshId;
  (void)out;
  return 0;
}

int plc_sofa_sendMesh(SofaBridge *bridge, SofaMesh *mesh)
{
  (void)bridge;
  (void)mesh;
  return 0;
}

void plc_sofa_setCallback(SofaBridge *bridge, void (*cb)(SofaState *, void *), void *userData)
{
  if (!bridge) return;
  bridge->onStateUpdate = cb;
  bridge->userData = userData;
}

int plc_sofa_toSimulation(const SofaBridge *bridge, void *simSystem)
{
  (void)bridge;
  (void)simSystem;
  return 0;
}
