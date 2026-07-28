#ifndef PLC_MULTIBODY_H
#define PLC_MULTIBODY_H

#include "plc_physics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_MAX_JOINTS  32
#define PLC_MAX_LINKS   64
#define PLC_MAX_DOF     32

/* ==================== 关节类型 ==================== */
typedef enum {
  JOINT_REVOLUTE,
  JOINT_PRISMATIC,
  JOINT_FIXED,
  JOINT_SPHERICAL,
  JOINT_CONTINUOUS,
  JOINT_PLANAR,
  JOINT_FLOATING,
} JointType;

/* ==================== 关节 ==================== */
typedef struct {
  JointType type;
  int parentLink;
  int childLink;
  plcVec3 pivotParent;
  plcVec3 pivotChild;
  plcVec3 axis;               /* 关节轴线 (局部坐标系) */
  plcVec3 q;                  /* 广义坐标 (位置/角度) */
  plcVec3 qd;                 /* 广义速度 */
  plcVec3 qdd;                /* 广义加速度 */
  float qMin, qMax;           /* 关节限位 */
  float torqueMax;
  float friction;
  float damping;
  float stiffness;
  bool hasLimits;
} plcJoint;

/* ==================== 连杆 ==================== */
typedef struct {
  plcRigidBody body;
  plcJoint joints[PLC_MAX_JOINTS];
  int jointCount;
  plcMat4 localTransform;
  plcMat4 worldTransform;
  plcVec3 comLocal;           /* 质心在局部坐标系 */
  int parent;
  int childCount;
  int children[8];
  float mass;
  plcMat3 inertiaLocal;
} plcLink;

/* ==================== 多体系统 ==================== */
typedef struct {
  plcLink links[PLC_MAX_LINKS];
  int linkCount;
  plcJoint joints[PLC_MAX_JOINTS];
  int jointCount;
  plcVec3 gravity;
  int rootLink;
  /* RNEA/ABA 临时存储 */
  float *hA;
  float *cA;
  float *tau;
  float *qddResult;
} plcMultiBody;

/* ==================== API ==================== */
void plc_mb_init(plcMultiBody *mb, plcVec3 gravity);
int plc_mb_addLink(plcMultiBody *mb, int parentIdx, plcVec3 com, float mass);
int plc_mb_addJoint(plcMultiBody *mb, int parent, int child, JointType type, plcVec3 axis, plcVec3 pivot);
void plc_mb_build(plcMultiBody *mb);

/* 正运动学: 计算所有连杆的世界变换 */
void plc_mb_forwardKinematics(plcMultiBody *mb);

/* 逆动力学: RNEA (Recursive Newton-Euler Algorithm) */
void plc_mb_rnea(plcMultiBody *mb, const float *q, const float *qd, const float *qdd, float *tauOut);

/* 正动力学: ABA (Articulated Body Algorithm) */
void plc_mb_aba(plcMultiBody *mb, const float *q, const float *qd, const float *tau, float *qddOut);

/* 质量矩阵 */
void plc_mb_massMatrix(plcMultiBody *mb, const float *q, float *M);

/* 科里奥利和重力项 */
void plc_mb_coriolisGravity(plcMultiBody *mb, const float *q, const float *qd, float *c);

/* 雅可比矩阵 */
void plc_mb_jacobian(plcMultiBody *mb, int linkIdx, plcVec3 point, const float *q, float *J);

/* 积分一步 (半隐式欧拉) */
void plc_mb_step(plcMultiBody *mb, float dt);

/* 关节限位检查 */
int plc_mb_checkLimits(plcMultiBody *mb);

#ifdef __cplusplus
}
#endif

#endif /* PLC_MULTIBODY_H */
