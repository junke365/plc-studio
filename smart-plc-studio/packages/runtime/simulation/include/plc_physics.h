#ifndef PLC_PHYSICS_H
#define PLC_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define PLC_PI          3.14159265358979323846f
#define PLC_DEG2RAD(d)  ((d) * PLC_PI / 180.0f)
#define PLC_RAD2DEG(r)  ((r) * 180.0f / PLC_PI)
#define PLC_EPS         1e-8f
#define PLC_MAX_BODIES  64
#define PLC_MAX_CONTACTS 128

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 基本类型 ==================== */
typedef struct { float x, y, z; } plcVec3;
typedef struct { float x, y, z, w; } plcQuat;
typedef struct { float m[16]; } plcMat4;
typedef struct { float m[9]; } plcMat3;

/* 3D 向量操作 */
static inline plcVec3 plc_vec3(float x, float y, float z)
{ plcVec3 v = {x, y, z}; return v; }
static inline plcVec3 plc_vec3_add(plcVec3 a, plcVec3 b)
{ return plc_vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline plcVec3 plc_vec3_sub(plcVec3 a, plcVec3 b)
{ return plc_vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline plcVec3 plc_vec3_scale(plcVec3 v, float s)
{ return plc_vec3(v.x * s, v.y * s, v.z * s); }
static inline float plc_vec3_dot(plcVec3 a, plcVec3 b)
{ return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline plcVec3 plc_vec3_cross(plcVec3 a, plcVec3 b)
{ return plc_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float plc_vec3_len(plcVec3 v)
{ return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
static inline plcVec3 plc_vec3_normalize(plcVec3 v)
{ float l = plc_vec3_len(v); if (l < PLC_EPS) return plc_vec3(0,0,0);
  return plc_vec3(v.x/l, v.y/l, v.z/l); }

/* 四元数操作 */
plcQuat plc_quat_identity(void);
plcQuat plc_quat_axisAngle(plcVec3 axis, float angle);
plcQuat plc_quat_mul(plcQuat a, plcQuat b);
plcVec3 plc_quat_rotate(plcQuat q, plcVec3 v);
plcQuat plc_quat_fromEuler(float roll, float pitch, float yaw);
void plc_quat_toEuler(plcQuat q, float *roll, float *pitch, float *yaw);

/* 4x4 矩阵操作 */
plcMat4 plc_mat4_identity(void);
plcMat4 plc_mat4_translate(float x, float y, float z);
plcMat4 plc_mat4_rotateX(float angle);
plcMat4 plc_mat4_rotateY(float angle);
plcMat4 plc_mat4_rotateZ(float angle);
plcMat4 plc_mat4_mul(plcMat4 a, plcMat4 b);
plcVec3 plc_mat4_transform(plcMat4 m, plcVec3 v);
plcMat4 plc_mat4_inverse(plcMat4 m);

/* ==================== 刚体 ==================== */
typedef struct {
  plcVec3 pos;
  plcQuat orient;
  plcVec3 linVel;
  plcVec3 angVel;
  float mass;
  plcMat4 inertia;            /* 局部惯性张量 */
  plcMat4 invInertia;         /* 局部惯性张量逆 */
  plcMat4 inertiaWorld;       /* 世界惯性张量 */
  plcMat4 invInertiaWorld;
  plcVec3 force;
  plcVec3 torque;
  bool dynamic;               /* true=动力学, false=运动学/静态 */
} plcRigidBody;

void plc_rb_init(plcRigidBody *body, float mass, plcMat4 inertia);
void plc_rb_setPos(plcRigidBody *body, plcVec3 pos, plcQuat orient);
void plc_rb_applyForce(plcRigidBody *body, plcVec3 f, plcVec3 point);
void plc_rb_applyForceWorld(plcRigidBody *body, plcVec3 f, plcVec3 point);
void plc_rb_updateInertia(plcRigidBody *body);
void plc_rb_clearForces(plcRigidBody *body);
void plc_rb_integrate(plcRigidBody *body, float dt);

/* ==================== 约束 ==================== */
typedef enum {
  PLC_CONSTRAINT_CONTACT,
  PLC_CONSTRAINT_BALL,
  PLC_CONSTRAINT_HINGE,
  PLC_CONSTRAINT_SLIDER,
  PLC_CONSTRAINT_FIXED,
} plcConstraintType;

typedef struct {
  plcConstraintType type;
  int bodyA, bodyB;           /* 刚体索引, -1 = 世界 */
  plcVec3 pivotA, pivotB;    /* 约束点在局部坐标系的位置 */
  plcVec3 axisA, axisB;       /* 约束轴在局部坐标系 */
  float damping;
  /* 接触约束参数 */
  float penetration;
  plcVec3 normal;
  plcVec3 contactPt;
  float restitution;
  float friction;
} plcConstraint;

/* ==================== 碰撞 ==================== */
typedef struct {
  int bodyA, bodyB;
  plcVec3 point;
  plcVec3 normal;
  float penetration;
} plcContact;

/* ==================== 物理世界 ==================== */
typedef struct {
  plcRigidBody bodies[PLC_MAX_BODIES];
  int bodyCount;
  plcConstraint constraints[PLC_MAX_CONTACTS];
  int constraintCount;
  plcContact contacts[PLC_MAX_CONTACTS];
  int contactCount;
  plcVec3 gravity;
  int solverIterations;
  float dt;
} plcPhysicsWorld;

void plc_physworld_init(plcPhysicsWorld *world);
int plc_physworld_addBody(plcPhysicsWorld *world, plcRigidBody *body);
int plc_physworld_addConstraint(plcPhysicsWorld *world, plcConstraint *c);
void plc_physworld_step(plcPhysicsWorld *world, float dt);
void plc_physworld_clearContacts(plcPhysicsWorld *world);

/* SOFA 桥接: 导出/导入状态 */
int plc_physworld_exportState(const plcPhysicsWorld *world, float *pos, float *vel, int maxLen);
int plc_physworld_importState(plcPhysicsWorld *world, const float *pos, const float *vel, int len);

#ifdef __cplusplus
}
#endif

#endif /* PLC_PHYSICS_H */
