#ifndef PLC_SIMULATION_H
#define PLC_SIMULATION_H

#include "plc_physics.h"
#include "plc_multibody.h"
#include "plc_surgical.h"
#include "plc_softbody.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_SIM_MAX_ROBOTS  8
#define PLC_SIM_MAX_SOFT    4

/* ==================== 仿真模式 ==================== */
typedef enum {
  SIM_MODE_IDLE = 0,
  SIM_MODE_RUNNING,
  SIM_MODE_PAUSED,
  SIM_MODE_SINGLE_STEP,
} SimMode;

/* ==================== 仿真统计 ==================== */
typedef struct {
  float simTime;
  float realTime;
  int stepCount;
  float avgStepTimeMs;
  float maxStepTimeMs;
  float minStepTimeMs;
  float stepTimeMs;
} SimStats;

/* ==================== 仿真配置 ==================== */
typedef struct {
  float dt;                    /* 仿真步长 (秒) */
  float gravity;               /* 重力加速度 (m/s²) */
  int solverIterations;        /* 约束求解迭代次数 */
  bool enableCollision;
  bool enableSoftBody;
  bool enableMultibody;
} SimConfig;

/* ==================== 仿真系统 ==================== */
typedef struct {
  SimConfig config;
  SimMode mode;
  SimStats stats;

  /* 物理世界 */
  plcPhysicsWorld world;

  /* 多体系统 */
  plcMultiBody multibodies[PLC_SIM_MAX_ROBOTS];
  int multibodyCount;

  /* 手术机器人 */
  SurgicalRobot robots[PLC_SIM_MAX_ROBOTS];
  int robotCount;

  /* 软体 */
  plcSoftBody softBodies[PLC_SIM_MAX_SOFT];
  int softBodyCount;

  /* 时间管理 */
  float simTime;
  float timeAccum;
} SimulationSystem;

/* ==================== 仿真 API ==================== */

/* 初始化/反初始化 */
int plc_sim_init(SimulationSystem *sim, const SimConfig *cfg);
void plc_sim_deinit(SimulationSystem *sim);

/* 仿真控制 */
int plc_sim_start(SimulationSystem *sim);
int plc_sim_pause(SimulationSystem *sim);
int plc_sim_resume(SimulationSystem *sim);
int plc_sim_step(SimulationSystem *sim);
int plc_sim_stop(SimulationSystem *sim);
SimMode plc_sim_getMode(const SimulationSystem *sim);

/* 添加对象 */
int plc_sim_addRigidBody(SimulationSystem *sim, plcRigidBody *body);
int plc_sim_addMultiBody(SimulationSystem *sim, plcMultiBody *mb);
int plc_sim_addSurgicalRobot(SimulationSystem *sim, SurgicalRobot *robot);
int plc_sim_addSoftBody(SimulationSystem *sim, plcSoftBody *sb);

/* 主更新循环 (实时仿真，传入 dtSec) */
void plc_sim_update(SimulationSystem *sim, float dtSec);

/* 固定步长更新 */
void plc_sim_updateFixed(SimulationSystem *sim, float dtSec);

/* 状态查询 */
float plc_sim_getTime(const SimulationSystem *sim);
int plc_sim_getStepCount(const SimulationSystem *sim);
SimStats plc_sim_getStats(const SimulationSystem *sim);
void plc_sim_resetStats(SimulationSystem *sim);

/* 状态导入/导出 (用于 SOFA 桥接) */
int plc_sim_exportState(const SimulationSystem *sim, float *buffer, int maxLen);
int plc_sim_importState(SimulationSystem *sim, const float *buffer, int len);

/* 获取特定手术机器人 */
SurgicalRobot *plc_sim_getRobot(SimulationSystem *sim, int index);

/* 获取物理世界 */
plcPhysicsWorld *plc_sim_getWorld(SimulationSystem *sim);

/* ==================== SOFA 桥接 (可选) ==================== */

typedef struct {
  int socketFd;                 /* 内部 socket 文件描述符 */
  int (*connect)(const char *host, int port);
  int (*disconnect)(void);
  int (*sendState)(const float *data, int len);
  int (*recvState)(float *data, int maxLen);
} SimSofaBridge;

int plc_sim_sofaBridgeInit(SimSofaBridge *bridge, const char *host, int port);

/* ==================== 错误码 ==================== */
#define SIM_OK              0
#define SIM_ERR_INIT        -1
#define SIM_ERR_PARAM       -2
#define SIM_ERR_FULL        -3
#define SIM_ERR_NOT_FOUND   -4
#define SIM_ERR_STATE       -5

#ifdef __cplusplus
}
#endif

#endif /* PLC_SIMULATION_H */
