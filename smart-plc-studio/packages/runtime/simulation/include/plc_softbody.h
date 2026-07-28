#ifndef PLC_SOFTBODY_H
#define PLC_SOFTBODY_H

#include "plc_physics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_MAX_NODES       256
#define PLC_MAX_SPRINGS     512
#define PLC_MAX_TETRAHEDRA  128
#define PLC_MAX_TRIANGLES   256

/* ==================== 软体节点 ==================== */
typedef struct {
  plcVec3 pos;
  plcVec3 vel;
  plcVec3 force;
  plcVec3 pos0;              /* 初始/静止位置 */
  float mass;
  float invMass;
  bool pinned;               /* 固定节点 */
} plcSoftNode;

/* ==================== 弹簧 ==================== */
typedef struct {
  int nodeA, nodeB;
  float restLength;
  float stiffness;
  float damping;
} plcSpring;

/* ==================== 四面体 (FEM) ==================== */
typedef struct {
  int nodes[4];
  float restVolume;
  float youngModulus;
  float poissonRatio;
  float invDm[9];            /* 初始形状矩阵逆 */
} plcTetrahedron;

/* ==================== 三角面 (碰撞检测) ==================== */
typedef struct {
  int nodes[3];
  plcVec3 normal;
} plcTriangle;

/* ==================== 软体 ==================== */
typedef struct {
  plcSoftNode nodes[PLC_MAX_NODES];
  int nodeCount;
  plcSpring springs[PLC_MAX_SPRINGS];
  int springCount;
  plcTetrahedron tets[PLC_MAX_TETRAHEDRA];
  int tetCount;
  plcTriangle tris[PLC_MAX_TRIANGLES];
  int triCount;
  float density;
  float damping;
  int bodyId;                /* 关联刚体 (0 = 世界) */
  /* 碰撞 */
  plcVec3 aabbMin, aabbMax;
} plcSoftBody;

/* ==================== 软体类型 ==================== */
typedef enum {
  SOFT_MASS_SPRING,
  SOFT_FEM_COROTATIONAL,
  SOFT_FEM_LINEAR,
} SoftBodyModel;

/* ==================== API ==================== */

/* 创建软体 */
void plc_soft_init(plcSoftBody *sb, SoftBodyModel model, float density);

/* 添加节点 */
int plc_soft_addNode(plcSoftBody *sb, plcVec3 pos, float mass, bool pinned);

/* 添加弹簧连接 */
int plc_soft_addSpring(plcSoftBody *sb, int nodeA, int nodeB, float stiffness, float damping);

/* 按结构添加弹簧 (网格/四面体) */
void plc_soft_addSpringGrid(plcSoftBody *sb, float stiffness, float damping);
void plc_soft_addSpringTet(plcSoftBody *sb, float stiffness, float damping);

/* 添加四面体 */
int plc_soft_addTet(plcSoftBody *sb, int n0, int n1, int n2, int n3,
                     float young, float poisson);

/* 创建长方体软体 */
void plc_soft_createBox(plcSoftBody *sb, plcVec3 center, plcVec3 size,
                         int segX, int segY, int segZ, SoftBodyModel model, float density);

/* 创建球体软体 */
void plc_soft_createSphere(plcSoftBody *sb, plcVec3 center, float radius,
                            int rings, int sectors, SoftBodyModel model, float density);

/* 更新碰撞形状 (AABB) */
void plc_soft_updateAABB(plcSoftBody *sb);

/* 力计算: 质量-弹簧系统 */
void plc_soft_computeSpringForces(plcSoftBody *sb);

/* 力计算: FEM 线弹性 */
void plc_soft_computeFemLinear(plcSoftBody *sb);

/* 力计算: FEM 共旋转 */
void plc_soft_computeFemCorotational(plcSoftBody *sb);

/* 积分一步 */
void plc_soft_step(plcSoftBody *sb, float dt);

/* 重置到初始位置 */
void plc_soft_reset(plcSoftBody *sb);

/* 施加外力 (如重力、工具压力) */
void plc_soft_applyForce(plcSoftBody *sb, plcVec3 force, float radius, plcVec3 center);

/* ==================== 碰撞检测 ==================== */

/* 软体-刚体碰撞 */
int plc_soft_collideRigid(plcSoftBody *sb, plcRigidBody *rb, plcContact *contacts, int maxContacts);

/* 软体-平面碰撞 */
int plc_soft_collidePlane(plcSoftBody *sb, plcVec3 planeNormal, float planeD,
                           plcContact *contacts, int maxContacts);

/* 软体-软体碰撞 */
int plc_soft_collideSoft(plcSoftBody *sbA, plcSoftBody *sbB, plcContact *contacts, int maxContacts);

#ifdef __cplusplus
}
#endif

#endif /* PLC_SOFTBODY_H */
