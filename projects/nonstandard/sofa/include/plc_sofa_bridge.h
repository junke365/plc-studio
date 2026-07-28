#ifndef PLC_SOFA_BRIDGE_H
#define PLC_SOFA_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOFA_MAX_NODES       1024
#define SOFA_MAX_MESHES      64
#define SOFA_MAX_DOF         1024

/* ==================== SOFA 状态导入/导出 ==================== */
typedef struct {
  int dofCount;
  float *positions;         /* [3 * dofCount] */
  float *velocities;        /* [3 * dofCount] */
  float *forces;            /* [3 * dofCount] */
  uint64_t timestamp;
} SofaState;

/* ==================== SOFA 网格 ==================== */
typedef struct {
  int id;
  float *vertices;
  int vertexCount;
  int *triangles;
  int triangleCount;
  float *tetrahedra;
  int tetraCount;
  float *restPositions;
} SofaMesh;

/* ==================== SOFA 桥接 ==================== */
typedef struct {
  /* 连接 */
  int socket;
  char host[128];
  int port;
  bool isConnected;
  /* 状态缓存 */
  SofaState currentState;
  SofaMesh meshes[SOFA_MAX_MESHES];
  int meshCount;
  /* 仿真控制 */
  float dt;
  bool isPaused;
  /* 回调 */
  void (*onStateUpdate)(SofaState *state, void *userData);
  void *userData;
} SofaBridge;

/* ==================== API ==================== */

/* 桥接初始化 */
int plc_sofa_bridgeInit(SofaBridge *bridge);
int plc_sofa_connect(SofaBridge *bridge, const char *host, int port);
int plc_sofa_disconnect(SofaBridge *bridge);
bool plc_sofa_isConnected(const SofaBridge *bridge);

/* 状态同步 */
int plc_sofa_sendState(SofaBridge *bridge, const float *pos, const float *vel, int dof);
int plc_sofa_recvState(SofaBridge *bridge, float *pos, float *vel, int *dof);
int plc_sofa_sendCommand(SofaBridge *bridge, const char *cmd);

/* 仿真控制 */
int plc_sofa_start(SofaBridge *bridge);
int plc_sofa_pause(SofaBridge *bridge);
int plc_sofa_step(SofaBridge *bridge, float dt);
int plc_sofa_reset(SofaBridge *bridge);

/* 网格数据 */
int plc_sofa_getMesh(SofaBridge *bridge, int meshId, SofaMesh *out);
int plc_sofa_sendMesh(SofaBridge *bridge, SofaMesh *mesh);

/* 回调节注册 */
void plc_sofa_setCallback(SofaBridge *bridge, void (*cb)(SofaState *, void *), void *userData);

/* 导入到 plc-simulation */
int plc_sofa_toSimulation(const SofaBridge *bridge, void *simSystem);

#ifdef __cplusplus
}
#endif

#endif /* PLC_SOFA_BRIDGE_H */
