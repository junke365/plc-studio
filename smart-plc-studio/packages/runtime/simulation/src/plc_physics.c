#include "plc_physics.h"
#include <string.h>
#include <math.h>

plcQuat plc_quat_identity(void)
{
  plcQuat q = {0, 0, 0, 1};
  return q;
}

plcQuat plc_quat_axisAngle(plcVec3 axis, float angle)
{
  float ha = angle * 0.5f;
  float s = sinf(ha);
  axis = plc_vec3_normalize(axis);
  plcQuat q;
  q.x = axis.x * s;
  q.y = axis.y * s;
  q.z = axis.z * s;
  q.w = cosf(ha);
  return q;
}

plcQuat plc_quat_mul(plcQuat a, plcQuat b)
{
  plcQuat q;
  q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  return q;
}

plcVec3 plc_quat_rotate(plcQuat q, plcVec3 v)
{
  plcVec3 u = {q.x, q.y, q.z};
  float s = q.w;
  float udot = plc_vec3_dot(u, v);
  plcVec3 ucross = plc_vec3_cross(u, v);
  plcVec3 result;
  result.x = 2.0f * udot * u.x + (s * s - plc_vec3_dot(u, u)) * v.x + 2.0f * s * ucross.x;
  result.y = 2.0f * udot * u.y + (s * s - plc_vec3_dot(u, u)) * v.y + 2.0f * s * ucross.y;
  result.z = 2.0f * udot * u.z + (s * s - plc_vec3_dot(u, u)) * v.z + 2.0f * s * ucross.z;
  return result;
}

plcQuat plc_quat_fromEuler(float roll, float pitch, float yaw)
{
  float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
  float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
  float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
  plcQuat q;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;
  q.w = cr * cp * cy + sr * sp * sy;
  return q;
}

void plc_quat_toEuler(plcQuat q, float *roll, float *pitch, float *yaw)
{
  float sqx = q.x * q.x, sqy = q.y * q.y, sqz = q.z * q.z;
  *roll = atan2f(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (sqx + sqy));
  *pitch = asinf(2.0f * (q.w * q.y - q.z * q.x));
  *yaw = atan2f(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (sqy + sqz));
}

/* 4x4 矩阵 */
plcMat4 plc_mat4_identity(void)
{
  plcMat4 m;
  memset(m.m, 0, sizeof(m.m));
  m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
  return m;
}

plcMat4 plc_mat4_translate(float x, float y, float z)
{
  plcMat4 m = plc_mat4_identity();
  m.m[12] = x; m.m[13] = y; m.m[14] = z;
  return m;
}

plcMat4 plc_mat4_rotateX(float angle)
{
  float c = cosf(angle), s = sinf(angle);
  plcMat4 m = plc_mat4_identity();
  m.m[5] = c; m.m[6] = -s;
  m.m[9] = s; m.m[10] = c;
  return m;
}

plcMat4 plc_mat4_rotateY(float angle)
{
  float c = cosf(angle), s = sinf(angle);
  plcMat4 m = plc_mat4_identity();
  m.m[0] = c; m.m[2] = s;
  m.m[8] = -s; m.m[10] = c;
  return m;
}

plcMat4 plc_mat4_rotateZ(float angle)
{
  float c = cosf(angle), s = sinf(angle);
  plcMat4 m = plc_mat4_identity();
  m.m[0] = c; m.m[1] = -s;
  m.m[4] = s; m.m[5] = c;
  return m;
}

plcMat4 plc_mat4_mul(plcMat4 a, plcMat4 b)
{
  plcMat4 r;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      float sum = 0;
      for (int k = 0; k < 4; k++)
        sum += a.m[i * 4 + k] * b.m[k * 4 + j];
      r.m[i * 4 + j] = sum;
    }
  return r;
}

plcVec3 plc_mat4_transform(plcMat4 m, plcVec3 v)
{
  plcVec3 r;
  r.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12];
  r.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13];
  r.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14];
  return r;
}

plcMat4 plc_mat4_inverse(plcMat4 m)
{
  plcMat4 inv;
  float det;
  float *a = m.m;

  inv.m[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15]
           + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
  inv.m[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15]
           - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
  inv.m[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15]
           + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
  inv.m[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14]
            - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
  inv.m[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15]
           - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
  inv.m[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15]
           + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
  inv.m[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15]
           - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
  inv.m[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14]
            + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
  inv.m[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15]
           + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
  inv.m[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15]
           - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
  inv.m[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15]
            + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
  inv.m[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14]
            - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
  inv.m[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11]
           - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
  inv.m[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11]
           + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
  inv.m[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11]
            - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
  inv.m[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10]
            + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

  det = a[0] * inv.m[0] + a[1] * inv.m[4] + a[2] * inv.m[8] + a[3] * inv.m[12];
  if (fabsf(det) < 1e-12f) return plc_mat4_identity();
  det = 1.0f / det;
  for (int i = 0; i < 16; i++) inv.m[i] *= det;
  return inv;
}

/* ==================== 刚体 ==================== */

void plc_rb_init(plcRigidBody *body, float mass, plcMat4 inertia)
{
  memset(body, 0, sizeof(plcRigidBody));
  body->mass = mass;
  body->inertia = inertia;
  body->dynamic = (mass > 0);
  if (body->dynamic) {
    body->invInertia = plc_mat4_inverse(inertia);
  }
  body->orient = plc_quat_identity();
}

void plc_rb_setPos(plcRigidBody *body, plcVec3 pos, plcQuat orient)
{
  body->pos = pos;
  body->orient = orient;
}

void plc_rb_applyForce(plcRigidBody *body, plcVec3 f, plcVec3 point)
{
  body->force.x += f.x;
  body->force.y += f.y;
  body->force.z += f.z;
  plcVec3 r = {point.x - body->pos.x, point.y - body->pos.y, point.z - body->pos.z};
  body->torque = plc_vec3_cross(r, f);
}

void plc_rb_applyForceWorld(plcRigidBody *body, plcVec3 f, plcVec3 point)
{
  plc_rb_applyForce(body, f, point);
}

void plc_rb_updateInertia(plcRigidBody *body)
{
  if (!body->dynamic) return;
  plcMat4 R = plc_mat4_identity();
  plcQuat q = body->orient;
  plcMat4 rot = plc_mat4_rotateX(0);
  rot.m[0] = 1 - 2 * (q.y * q.y + q.z * q.z);
  rot.m[1] = 2 * (q.x * q.y - q.w * q.z);
  rot.m[2] = 2 * (q.x * q.z + q.w * q.y);
  rot.m[4] = 2 * (q.x * q.y + q.w * q.z);
  rot.m[5] = 1 - 2 * (q.x * q.x + q.z * q.z);
  rot.m[6] = 2 * (q.y * q.z - q.w * q.x);
  rot.m[8] = 2 * (q.x * q.z - q.w * q.y);
  rot.m[9] = 2 * (q.y * q.z + q.w * q.x);
  rot.m[10] = 1 - 2 * (q.x * q.x + q.y * q.y);
  body->inertiaWorld = plc_mat4_mul(rot, plc_mat4_mul(body->inertia, plc_mat4_inverse(rot)));
  body->invInertiaWorld = plc_mat4_inverse(body->inertiaWorld);
}

void plc_rb_clearForces(plcRigidBody *body)
{
  body->force.x = 0; body->force.y = 0; body->force.z = 0;
  body->torque.x = 0; body->torque.y = 0; body->torque.z = 0;
}

void plc_rb_integrate(plcRigidBody *body, float dt)
{
  if (!body->dynamic) return;

  /* 线速度 */
  plcVec3 accel;
  accel.x = body->force.x / body->mass;
  accel.y = body->force.y / body->mass;
  accel.z = body->force.z / body->mass;
  body->linVel.x += accel.x * dt;
  body->linVel.y += accel.y * dt;
  body->linVel.z += accel.z * dt;
  body->pos.x += body->linVel.x * dt;
  body->pos.y += body->linVel.y * dt;
  body->pos.z += body->linVel.z * dt;

  /* 角速度 */
  plc_rb_updateInertia(body);
  plcVec3 angAccel;
  angAccel.x = body->invInertiaWorld.m[0] * body->torque.x +
               body->invInertiaWorld.m[4] * body->torque.y +
               body->invInertiaWorld.m[8] * body->torque.z;
  angAccel.y = body->invInertiaWorld.m[1] * body->torque.x +
               body->invInertiaWorld.m[5] * body->torque.y +
               body->invInertiaWorld.m[9] * body->torque.z;
  angAccel.z = body->invInertiaWorld.m[2] * body->torque.x +
               body->invInertiaWorld.m[6] * body->torque.y +
               body->invInertiaWorld.m[10] * body->torque.z;
  body->angVel.x += angAccel.x * dt;
  body->angVel.y += angAccel.y * dt;
  body->angVel.z += angAccel.z * dt;

  plcQuat dq;
  dq.x = 0.5f * (body->orient.w * body->angVel.x - body->orient.z * body->angVel.y + body->orient.y * body->angVel.z);
  dq.y = 0.5f * (body->orient.z * body->angVel.x + body->orient.w * body->angVel.y - body->orient.x * body->angVel.z);
  dq.z = 0.5f * (-body->orient.y * body->angVel.x + body->orient.x * body->angVel.y + body->orient.w * body->angVel.z);
  dq.w = 0.5f * (-body->orient.x * body->angVel.x - body->orient.y * body->angVel.y - body->orient.z * body->angVel.z);

  body->orient.x += dq.x * dt;
  body->orient.y += dq.y * dt;
  body->orient.z += dq.z * dt;
  body->orient.w += dq.w * dt;

  float len = sqrtf(body->orient.x * body->orient.x + body->orient.y * body->orient.y +
                    body->orient.z * body->orient.z + body->orient.w * body->orient.w);
  if (len > 0) {
    body->orient.x /= len;
    body->orient.y /= len;
    body->orient.z /= len;
    body->orient.w /= len;
  }
}

/* ==================== 物理世界 ==================== */

void plc_physworld_init(plcPhysicsWorld *world)
{
  memset(world, 0, sizeof(plcPhysicsWorld));
  world->gravity.x = 0;
  world->gravity.y = -9.81f;
  world->gravity.z = 0;
  world->solverIterations = 10;
  world->dt = 0.001f;
}

int plc_physworld_addBody(plcPhysicsWorld *world, plcRigidBody *body)
{
  if (world->bodyCount >= PLC_MAX_BODIES) return -1;
  world->bodies[world->bodyCount] = *body;
  return world->bodyCount++;
}

int plc_physworld_addConstraint(plcPhysicsWorld *world, plcConstraint *c)
{
  if (world->constraintCount >= PLC_MAX_CONTACTS) return -1;
  world->constraints[world->constraintCount++] = *c;
  return 0;
}

static void solveContact(plcRigidBody *bodyA, plcRigidBody *bodyB, plcConstraint *c)
{
  (void)bodyA;
  (void)bodyB;
  (void)c;
}

static void solveConstraints(plcPhysicsWorld *world)
{
  for (int iter = 0; iter < world->solverIterations; iter++) {
    for (int i = 0; i < world->constraintCount; i++) {
      plcConstraint *c = &world->constraints[i];
      plcRigidBody *bodyA = (c->bodyA >= 0) ? &world->bodies[c->bodyA] : NULL;
      plcRigidBody *bodyB = (c->bodyB >= 0) ? &world->bodies[c->bodyB] : NULL;
      solveContact(bodyA, bodyB, c);
    }
  }
}

void plc_physworld_step(plcPhysicsWorld *world, float dt)
{
  world->dt = dt;

  /* 施加重力 */
  for (int i = 0; i < world->bodyCount; i++) {
    if (world->bodies[i].dynamic) {
      plcVec3 gravForce;
      gravForce.x = world->gravity.x * world->bodies[i].mass;
      gravForce.y = world->gravity.y * world->bodies[i].mass;
      gravForce.z = world->gravity.z * world->bodies[i].mass;
      world->bodies[i].force.x += gravForce.x;
      world->bodies[i].force.y += gravForce.y;
      world->bodies[i].force.z += gravForce.z;
    }
  }

  /* 积分 */
  for (int i = 0; i < world->bodyCount; i++) {
    plc_rb_integrate(&world->bodies[i], dt);
  }

  /* 约束求解 */
  solveConstraints(world);

  /* 清空力 */
  for (int i = 0; i < world->bodyCount; i++) {
    plc_rb_clearForces(&world->bodies[i]);
  }
}

void plc_physworld_clearContacts(plcPhysicsWorld *world)
{
  world->contactCount = 0;
}

int plc_physworld_exportState(const plcPhysicsWorld *world, float *pos, float *vel, int maxLen)
{
  int idx = 0;
  for (int i = 0; i < world->bodyCount && idx < maxLen; i++) {
    pos[idx++] = world->bodies[i].pos.x;
    if (idx >= maxLen) return idx;
    pos[idx++] = world->bodies[i].pos.y;
    if (idx >= maxLen) return idx;
    pos[idx++] = world->bodies[i].pos.z;
    if (idx >= maxLen) return idx;
  }
  return idx;
}

int plc_physworld_importState(plcPhysicsWorld *world, const float *pos, const float *vel, int len)
{
  int idx = 0;
  for (int i = 0; i < world->bodyCount && idx < len; i++) {
    world->bodies[i].pos.x = pos[idx++];
    if (idx >= len) return i;
    world->bodies[i].pos.y = pos[idx++];
    if (idx >= len) return i;
    world->bodies[i].pos.z = pos[idx++];
    if (idx >= len) return i;
  }
  return world->bodyCount;
}
