#ifndef PLC_SURGICAL_H
#define PLC_SURGICAL_H

#include "plc_multibody.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== dVRK 手术机器人类型 ==================== */
typedef enum {
  SURGICAL_NONE = 0,
  SURGICAL_MTM,               /* Master Tool Manipulator (7-DOF) */
  SURGICAL_PSM,               /* Patient Side Manipulator (7-DOF) */
  SURGICAL_ECM,               /* Endoscopic Camera Manipulator (4-DOF) */
} SurgicalRobotType;

/* ==================== MTM 运动学参数 (da Vinci Master Tool Manipulator) ====================
 *
 * 7-DOF: 肩关节外展/屈曲 (2) + 上臂旋转 (1) + 肘关节屈曲 (1) +
 *         前臂旋转 (1) + 腕关节屈曲/旋转 (2)
 */
typedef struct {
  float l0;    /* 基座高度 */
  float l1;    /* 上臂长度 */
  float l2;    /* 前臂长度 */
  float l3;    /* 手腕长度 */
  float l4;    /* 工具长度 */
} MtmParams;

/* ==================== PSM 运动学参数 (Patient Side Manipulator) ====================
 *
 * 7-DOF: 偏航 (1) + 俯仰 (1) + 插入 (1) + 滚转 (1) +
 *         腕关节俯仰/偏航 (2) + 夹持 (1)
 */
typedef struct {
  float d1;    /* 基座偏移 */
  float d2;    /* 套管长度 */
  float l1;    /* 远端中心到腕关节距离 */
  float l2;    /* 腕关节到工具端距离 */
  float theta1; /* 套管角度 */
} PsmParams;

/* ==================== 手术机器人 ==================== */
typedef struct {
  SurgicalRobotType type;
  plcMultiBody mb;
  /* DH 参数表 */
  float dhA[8];        /* 连杆长度 a_i */
  float dhAlpha[8];    /* 连杆扭转 α_i */
  float dhD[8];        /* 连杆偏移 d_i */
  float dhTheta[8];    /* 关节角 θ_i */
  int dof;             /* 自由度 */
  /* 配置参数 */
  union {
    MtmParams mtm;
    PsmParams psm;
  } params;
  /* 运动学缓存 */
  plcMat4 tcpTransform;    /* 工具中心点变换 */
  plcVec3 tcpPosition;     /* TCP 位置 */
  plcMat4 jacobian[6];     /* 几何雅可比 */
  float jacobianDet;       /* 雅可比行列式 (奇异性检测) */
  plcMat4 jointFrames[8];  /* 各关节坐标系变换缓存 */
  plcVec3 toolOffset;      /* 工具偏移 */
  plcQuat toolOrient;      /* 工具姿态偏移 */
  float qCurrent[8];       /* 当前关节角缓存 */
} SurgicalRobot;

/* ==================== API ==================== */

/* 初始化手术机器人 */
int plc_surgical_init(SurgicalRobot *robot, SurgicalRobotType type);

/* 加载默认参数 */
void plc_surgical_loadMtmParams(MtmParams *params);
void plc_surgical_loadPsmParams(PsmParams *params);

/* 正运动学: 关节角 → TCP 位姿 */
int plc_surgical_forward(SurgicalRobot *robot, const float *jointAngles, plcVec3 *pos, plcQuat *orient);

/* 逆运动学: TCP 位姿 → 关节角 (返回解的数量) */
int plc_surgical_inverse(SurgicalRobot *robot, plcVec3 targetPos, plcQuat targetOrient,
                          float *jointAngles, int maxSolutions);

/* 计算几何雅可比矩阵 (6×DOF) */
void plc_surgical_jacobian(SurgicalRobot *robot, const float *q, float *J);

/* 奇异性检查 */
float plc_surgical_singularity(SurgicalRobot *robot, const float *q);

/* 工作空间检查 */
int plc_surgical_inWorkspace(SurgicalRobot *robot, plcVec3 pos);

/* 关节限位检查 */
int plc_surgical_checkJoints(SurgicalRobot *robot, const float *q);

/* 关节空间插值 (梯形速度) */
void plc_surgical_jointInterp(SurgicalRobot *robot, const float *qStart, const float *qEnd,
                               float t, float *qOut);

/* 笛卡尔空间直线插值 */
void plc_surgical_cartesianInterp(SurgicalRobot *robot, plcVec3 pStart, plcQuat qStart,
                                   plcVec3 pEnd, plcQuat qEnd, float t,
                                   plcVec3 *pOut, plcQuat *qOut);

/* 更新机器人状态 (从多体系统) */
void plc_surgical_update(SurgicalRobot *robot);

/* ==================== PSM 特定工具 ==================== */

/* 设置 PSM 套管插入角度 */
void plc_surgical_psm_setCannulaAngle(SurgicalRobot *robot, float angle);

/* 夹持控制 (PSM 第7轴) */
void plc_surgical_psm_grip(SurgicalRobot *robot, float openRatio);

/* ==================== 工具坐标系 ==================== */

/* 设置工具偏移 (从最后一个关节到 TCP) */
void plc_surgical_setToolOffset(SurgicalRobot *robot, plcVec3 offset, plcQuat orient);

#ifdef __cplusplus
}
#endif

#endif /* PLC_SURGICAL_H */
