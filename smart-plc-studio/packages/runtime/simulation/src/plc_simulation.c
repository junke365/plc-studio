#include "plc_simulation.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

int plc_sim_init(SimulationSystem *sim, const SimConfig *cfg)
{
  memset(sim, 0, sizeof(SimulationSystem));
  sim->config = *cfg;
  sim->mode = SIM_MODE_IDLE;
  sim->simTime = 0;
  sim->timeAccum = 0;
  sim->stats.maxStepTimeMs = 0;
  sim->stats.minStepTimeMs = 1e10f;

  plc_physworld_init(&sim->world);
  sim->world.gravity.y = -cfg->gravity;
  sim->world.solverIterations = cfg->solverIterations;

  return SIM_OK;
}

void plc_sim_deinit(SimulationSystem *sim)
{
  memset(sim, 0, sizeof(SimulationSystem));
}

int plc_sim_start(SimulationSystem *sim)
{
  if (!sim) return SIM_ERR_PARAM;
  sim->mode = SIM_MODE_RUNNING;
  return SIM_OK;
}

int plc_sim_pause(SimulationSystem *sim)
{
  if (!sim) return SIM_ERR_PARAM;
  sim->mode = SIM_MODE_PAUSED;
  return SIM_OK;
}

int plc_sim_resume(SimulationSystem *sim)
{
  if (!sim) return SIM_ERR_PARAM;
  if (sim->mode == SIM_MODE_PAUSED) {
    sim->mode = SIM_MODE_RUNNING;
  }
  return SIM_OK;
}

int plc_sim_step(SimulationSystem *sim)
{
  if (!sim) return SIM_ERR_PARAM;

  float dt = sim->config.dt;

  /* 更新物理世界 */
  if (sim->config.enableCollision) {
    plc_physworld_step(&sim->world, dt);
  }
  /* 更新手术机器人 */
  for (int i = 0; i < sim->robotCount; i++) {
    plc_surgical_update(&sim->robots[i]);
  }
  /* 更新软体 */
  if (sim->config.enableSoftBody) {
    for (int i = 0; i < sim->softBodyCount; i++) {
      plc_soft_computeSpringForces(&sim->softBodies[i]);
      if (sim->config.enableCollision) {
        plcVec3 planeN = plc_vec3(0, 1, 0);
        plc_soft_collidePlane(&sim->softBodies[i], planeN, 0, NULL, 0);
      }
      plc_soft_step(&sim->softBodies[i], dt);
    }
  }

  sim->simTime += dt;
  sim->stats.stepCount++;
  if (sim->mode == SIM_MODE_SINGLE_STEP) {
    sim->mode = SIM_MODE_PAUSED;
  }
  return SIM_OK;
}

int plc_sim_stop(SimulationSystem *sim)
{
  if (!sim) return SIM_ERR_PARAM;
  sim->mode = SIM_MODE_IDLE;
  sim->simTime = 0;
  return SIM_OK;
}

SimMode plc_sim_getMode(const SimulationSystem *sim)
{
  return sim ? sim->mode : SIM_MODE_IDLE;
}

int plc_sim_addRigidBody(SimulationSystem *sim, plcRigidBody *body)
{
  if (!sim || !body) return SIM_ERR_PARAM;
  return plc_physworld_addBody(&sim->world, body);
}

int plc_sim_addMultiBody(SimulationSystem *sim, plcMultiBody *mb)
{
  if (!sim || !mb) return SIM_ERR_PARAM;
  if (sim->multibodyCount >= PLC_SIM_MAX_ROBOTS) return SIM_ERR_FULL;
  sim->multibodies[sim->multibodyCount++] = *mb;
  return SIM_OK;
}

int plc_sim_addSurgicalRobot(SimulationSystem *sim, SurgicalRobot *robot)
{
  if (!sim || !robot) return SIM_ERR_PARAM;
  if (sim->robotCount >= PLC_SIM_MAX_ROBOTS) return SIM_ERR_FULL;
  sim->robots[sim->robotCount++] = *robot;
  return SIM_OK;
}

int plc_sim_addSoftBody(SimulationSystem *sim, plcSoftBody *sb)
{
  if (!sim || !sb) return SIM_ERR_PARAM;
  if (sim->softBodyCount >= PLC_SIM_MAX_SOFT) return SIM_ERR_FULL;
  sim->softBodies[sim->softBodyCount++] = *sb;
  return SIM_OK;
}

void plc_sim_update(SimulationSystem *sim, float dtSec)
{
  if (!sim || sim->mode != SIM_MODE_RUNNING) return;

  sim->stats.realTime += dtSec;
  sim->timeAccum += dtSec;

  while (sim->timeAccum >= sim->config.dt) {
    plc_sim_step(sim);
    sim->timeAccum -= sim->config.dt;
  }
}

void plc_sim_updateFixed(SimulationSystem *sim, float dtSec)
{
  if (!sim || sim->mode != SIM_MODE_RUNNING) return;
  sim->config.dt = dtSec;
  plc_sim_step(sim);
}

float plc_sim_getTime(const SimulationSystem *sim)
{
  return sim ? sim->simTime : 0;
}

int plc_sim_getStepCount(const SimulationSystem *sim)
{
  return sim ? sim->stats.stepCount : 0;
}

SimStats plc_sim_getStats(const SimulationSystem *sim)
{
  SimStats empty = {0};
  return sim ? sim->stats : empty;
}

void plc_sim_resetStats(SimulationSystem *sim)
{
  if (sim) {
    memset(&sim->stats, 0, sizeof(SimStats));
    sim->stats.minStepTimeMs = 1e10f;
  }
}

int plc_sim_exportState(const SimulationSystem *sim, float *buffer, int maxLen)
{
  if (!sim || !buffer) return SIM_ERR_PARAM;

  int bodyCount = sim->world.bodyCount;
  int robotCount = sim->robotCount;
  int softCount = sim->softBodyCount;

  /* 计算所需大小: header(1) + 刚体(13/个) + 机器人(8/个) + 软体节点(1+6*nodeCount/个) */
  int required = 1;
  required += bodyCount * 13;
  required += robotCount * 8;
  for (int i = 0; i < softCount; i++)
    required += 1 + sim->softBodies[i].nodeCount * 6;

  if (maxLen < required)
    return -required;

  /* 写入 header: bodyCount(bit0-7) | robotCount(bit8-15) | softCount(bit16-23) */
  int header = (bodyCount & 0xFF) | ((robotCount & 0xFF) << 8) | ((softCount & 0xFF) << 16);
  memcpy(&buffer[0], &header, sizeof(int));

  int idx = 1;

  /* 写入刚体: pos(3) + orient(4) + linVel(3) + angVel(3) = 13 */
  for (int i = 0; i < bodyCount; i++) {
    const plcRigidBody *b = &sim->world.bodies[i];
    buffer[idx++] = b->pos.x;    buffer[idx++] = b->pos.y;    buffer[idx++] = b->pos.z;
    buffer[idx++] = b->orient.x; buffer[idx++] = b->orient.y;
    buffer[idx++] = b->orient.z; buffer[idx++] = b->orient.w;
    buffer[idx++] = b->linVel.x; buffer[idx++] = b->linVel.y; buffer[idx++] = b->linVel.z;
    buffer[idx++] = b->angVel.x; buffer[idx++] = b->angVel.y; buffer[idx++] = b->angVel.z;
  }

  /* 写入手术机器人关节角: qCurrent[8] */
  for (int i = 0; i < robotCount; i++) {
    const SurgicalRobot *r = &sim->robots[i];
    for (int j = 0; j < 8; j++)
      buffer[idx++] = r->qCurrent[j];
  }

  /* 写入软体节点: nodeCount + pos(3) + vel(3) 每个节点 */
  for (int i = 0; i < softCount; i++) {
    const plcSoftBody *sb = &sim->softBodies[i];
    buffer[idx++] = (float)sb->nodeCount;
    for (int j = 0; j < sb->nodeCount; j++) {
      buffer[idx++] = sb->nodes[j].pos.x;
      buffer[idx++] = sb->nodes[j].pos.y;
      buffer[idx++] = sb->nodes[j].pos.z;
      buffer[idx++] = sb->nodes[j].vel.x;
      buffer[idx++] = sb->nodes[j].vel.y;
      buffer[idx++] = sb->nodes[j].vel.z;
    }
  }

  return required;
}

int plc_sim_importState(SimulationSystem *sim, const float *buffer, int len)
{
  if (!sim || !buffer || len < 1) return SIM_ERR_PARAM;

  /* 解析 header */
  int header;
  memcpy(&header, &buffer[0], sizeof(int));
  int bodyCount = header & 0xFF;
  int robotCount = (header >> 8) & 0xFF;
  int softCount = (header >> 16) & 0xFF;

  if (bodyCount > PLC_MAX_BODIES ||
      robotCount > PLC_SIM_MAX_ROBOTS ||
      softCount > PLC_SIM_MAX_SOFT)
    return SIM_ERR_STATE;

  int idx = 1;

  /* 恢复刚体 */
  sim->world.bodyCount = bodyCount;
  for (int i = 0; i < bodyCount; i++) {
    if (idx + 13 > len) return SIM_ERR_STATE;
    plcRigidBody *b = &sim->world.bodies[i];
    b->pos.x = buffer[idx++]; b->pos.y = buffer[idx++]; b->pos.z = buffer[idx++];
    b->orient.x = buffer[idx++]; b->orient.y = buffer[idx++];
    b->orient.z = buffer[idx++]; b->orient.w = buffer[idx++];
    b->linVel.x = buffer[idx++]; b->linVel.y = buffer[idx++]; b->linVel.z = buffer[idx++];
    b->angVel.x = buffer[idx++]; b->angVel.y = buffer[idx++]; b->angVel.z = buffer[idx++];
  }

  /* 恢复手术机器人关节角 */
  sim->robotCount = robotCount;
  for (int i = 0; i < robotCount; i++) {
    if (idx + 8 > len) return SIM_ERR_STATE;
    SurgicalRobot *r = &sim->robots[i];
    for (int j = 0; j < 8; j++)
      r->qCurrent[j] = buffer[idx++];
  }

  /* 恢复软体节点 */
  sim->softBodyCount = softCount;
  for (int i = 0; i < softCount; i++) {
    if (idx + 1 > len) return SIM_ERR_STATE;
    int nodeCount = (int)buffer[idx++];
    if (nodeCount > PLC_MAX_NODES) return SIM_ERR_STATE;
    plcSoftBody *sb = &sim->softBodies[i];
    sb->nodeCount = nodeCount;
    for (int j = 0; j < nodeCount; j++) {
      if (idx + 6 > len) return SIM_ERR_STATE;
      sb->nodes[j].pos.x = buffer[idx++];
      sb->nodes[j].pos.y = buffer[idx++];
      sb->nodes[j].pos.z = buffer[idx++];
      sb->nodes[j].vel.x = buffer[idx++];
      sb->nodes[j].vel.y = buffer[idx++];
      sb->nodes[j].vel.z = buffer[idx++];
    }
  }

  return idx;
}

SurgicalRobot *plc_sim_getRobot(SimulationSystem *sim, int index)
{
  if (!sim || index < 0 || index >= sim->robotCount) return NULL;
  return &sim->robots[index];
}

plcPhysicsWorld *plc_sim_getWorld(SimulationSystem *sim)
{
  return sim ? &sim->world : NULL;
}

/* ==================== SOFA 桥接内部实现 ==================== */

#ifdef _WIN32
static bool g_wsaInit = false;
#endif

static int g_sockFd = -1;

/* 循环发送所有字节 */
static int sendAllBytes(int fd, const char *data, int size)
{
  int total = 0;
  while (total < size) {
    int sent = (int)send(fd, data + total, size - total, 0);
    if (sent <= 0) return -1;
    total += sent;
  }
  return 0;
}

/* 循环接收所有字节 */
static int recvAllBytes(int fd, char *data, int size)
{
  int total = 0;
  while (total < size) {
    int n = (int)recv(fd, data + total, size - total, 0);
    if (n <= 0) return -1;
    total += n;
  }
  return 0;
}

static int sofaConnect(const char *host, int port)
{
#ifdef _WIN32
  if (!g_wsaInit) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    g_wsaInit = true;
  }
#endif

  int sock = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return -1;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)port);
  addr.sin_addr.s_addr = inet_addr(host);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return -1;
  }

  g_sockFd = sock;
  return 0;
}

static int sofaDisconnect(void)
{
  if (g_sockFd < 0) return -1;
#ifdef _WIN32
  closesocket(g_sockFd);
#else
  close(g_sockFd);
#endif
  g_sockFd = -1;
  return 0;
}

static int sofaSendState(const float *data, int len)
{
  if (g_sockFd < 0) return -1;

  /* 先发 4 字节长度（网络字节序 = 本机小端即可，本地桥接） */
  int32_t netLen = (int32_t)len;
  if (sendAllBytes(g_sockFd, (const char *)&netLen, sizeof(netLen)) != 0)
    return -1;

  /* 再发数据体 */
  if (len > 0) {
    if (sendAllBytes(g_sockFd, (const char *)data, len * (int)sizeof(float)) != 0)
      return -1;
  }

  return 0;
}

static int sofaRecvState(float *data, int maxLen)
{
  if (g_sockFd < 0) return -1;

  /* 先收 4 字节长度 */
  int32_t netLen = 0;
  if (recvAllBytes(g_sockFd, (char *)&netLen, sizeof(netLen)) != 0)
    return -1;

  if (netLen < 0 || netLen > maxLen) return -1;

  /* 心跳检测 */
  if (netLen == 0) return 0;

  /* 再收数据体 */
  if (recvAllBytes(g_sockFd, (char *)data, netLen * (int)sizeof(float)) != 0)
    return -1;

  return (int)netLen;
}

int plc_sim_sofaBridgeInit(SimSofaBridge *bridge, const char *host, int port)
{
  if (!bridge) return SIM_ERR_PARAM;
  (void)host;
  (void)port;

#ifdef _WIN32
  g_wsaInit = false;
#endif
  g_sockFd = -1;

  bridge->socketFd = -1;
  bridge->connect = sofaConnect;
  bridge->disconnect = sofaDisconnect;
  bridge->sendState = sofaSendState;
  bridge->recvState = sofaRecvState;

  return SIM_OK;
}
