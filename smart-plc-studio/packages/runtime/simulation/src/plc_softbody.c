#include "plc_softbody.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

static float plcMin3(float a, float b, float c)
{
  float r = (a < b) ? a : b;
  return (r < c) ? r : c;
}

static float plcMax3(float a, float b, float c)
{
  float r = (a > b) ? a : b;
  return (r > c) ? r : c;
}

void plc_soft_init(plcSoftBody *sb, SoftBodyModel model, float density)
{
  memset(sb, 0, sizeof(plcSoftBody));
  sb->density = density;
  sb->damping = 0.01f;
  sb->aabbMin = plc_vec3(1e10f, 1e10f, 1e10f);
  sb->aabbMax = plc_vec3(-1e10f, -1e10f, -1e10f);
}

int plc_soft_addNode(plcSoftBody *sb, plcVec3 pos, float mass, bool pinned)
{
  if (sb->nodeCount >= PLC_MAX_NODES) return -1;
  int idx = sb->nodeCount++;
  sb->nodes[idx].pos = pos;
  sb->nodes[idx].pos0 = pos;
  sb->nodes[idx].vel = plc_vec3(0, 0, 0);
  sb->nodes[idx].force = plc_vec3(0, 0, 0);
  sb->nodes[idx].mass = mass;
  sb->nodes[idx].invMass = (mass > 0) ? 1.0f / mass : 0;
  sb->nodes[idx].pinned = pinned;
  return idx;
}

int plc_soft_addSpring(plcSoftBody *sb, int nodeA, int nodeB, float stiffness, float damping)
{
  if (sb->springCount >= PLC_MAX_SPRINGS) return -1;
  int idx = sb->springCount++;
  sb->springs[idx].nodeA = nodeA;
  sb->springs[idx].nodeB = nodeB;
  plcVec3 diff = plc_vec3_sub(sb->nodes[nodeB].pos, sb->nodes[nodeA].pos);
  sb->springs[idx].restLength = plc_vec3_len(diff);
  sb->springs[idx].stiffness = stiffness;
  sb->springs[idx].damping = damping;
  return idx;
}

void plc_soft_addSpringGrid(plcSoftBody *sb, float stiffness, float damping)
{
  (void)sb;
  (void)stiffness;
  (void)damping;
}

void plc_soft_addSpringTet(plcSoftBody *sb, float stiffness, float damping)
{
  (void)sb;
  (void)stiffness;
  (void)damping;
}

int plc_soft_addTet(plcSoftBody *sb, int n0, int n1, int n2, int n3,
                     float young, float poisson)
{
  if (sb->tetCount >= PLC_MAX_TETRAHEDRA) return -1;
  int idx = sb->tetCount++;
  sb->tets[idx].nodes[0] = n0;
  sb->tets[idx].nodes[1] = n1;
  sb->tets[idx].nodes[2] = n2;
  sb->tets[idx].nodes[3] = n3;
  sb->tets[idx].youngModulus = young;
  sb->tets[idx].poissonRatio = poisson;
  sb->tets[idx].restVolume = 1.0f;
  return idx;
}

void plc_soft_createBox(plcSoftBody *sb, plcVec3 center, plcVec3 size,
                         int segX, int segY, int segZ, SoftBodyModel model, float density)
{
  (void)model;
  float dx = size.x / segX;
  float dy = size.y / segY;
  float dz = size.z / segZ;
  plcVec3 start;
  start.x = center.x - size.x * 0.5f;
  start.y = center.y - size.y * 0.5f;
  start.z = center.z - size.z * 0.5f;

  for (int iz = 0; iz <= segZ; iz++)
    for (int iy = 0; iy <= segY; iy++)
      for (int ix = 0; ix <= segX; ix++) {
        plcVec3 p;
        p.x = start.x + ix * dx;
        p.y = start.y + iy * dy;
        p.z = start.z + iz * dz;
        float cellVol = dx * dy * dz;
        float nodeMass = density * cellVol;
        bool pinned = (iz == 0);
        plc_soft_addNode(sb, p, nodeMass, pinned);
      }

  for (int iz = 0; iz < segZ; iz++)
    for (int iy = 0; iy < segY; iy++)
      for (int ix = 0; ix < segX; ix++) {
        int idx = iz * (segY + 1) * (segX + 1) + iy * (segX + 1) + ix;
        int r = (segX + 1);
        int c = (segY + 1) * r;
        float k = 1000.0f;
        float d = 1.0f;
        plc_soft_addSpring(sb, idx, idx + 1, k, d);
        plc_soft_addSpring(sb, idx, idx + r, k, d);
        plc_soft_addSpring(sb, idx, idx + c, k, d);
        plc_soft_addSpring(sb, idx + 1, idx + r + 1, k, d);
        plc_soft_addSpring(sb, idx + r, idx + r + 1, k, d);
        plc_soft_addSpring(sb, idx + c, idx + 1 + c, k, d);
        plc_soft_addSpring(sb, idx + 1, idx + 1 + c, k, d);
        plc_soft_addSpring(sb, idx + r + c, idx + r + c + 1, k, d);
        plc_soft_addSpring(sb, idx + r + c, idx + r + 1 + c, k, d);
        plc_soft_addSpring(sb, idx + r + c, idx + r + c + r, k, d);
      }
}

void plc_soft_createSphere(plcSoftBody *sb, plcVec3 center, float radius,
                            int rings, int sectors, SoftBodyModel model, float density)
{
  (void)sb;
  (void)center;
  (void)radius;
  (void)rings;
  (void)sectors;
  (void)model;
  (void)density;
}

void plc_soft_updateAABB(plcSoftBody *sb)
{
  sb->aabbMin = plc_vec3(1e10f, 1e10f, 1e10f);
  sb->aabbMax = plc_vec3(-1e10f, -1e10f, -1e10f);
  for (int i = 0; i < sb->nodeCount; i++) {
    plcVec3 p = sb->nodes[i].pos;
    sb->aabbMin.x = plcMin3(sb->aabbMin.x, p.x, p.x);
    sb->aabbMin.y = plcMin3(sb->aabbMin.y, p.y, p.y);
    sb->aabbMin.z = plcMin3(sb->aabbMin.z, p.z, p.z);
    sb->aabbMax.x = plcMax3(sb->aabbMax.x, p.x, p.x);
    sb->aabbMax.y = plcMax3(sb->aabbMax.y, p.y, p.y);
    sb->aabbMax.z = plcMax3(sb->aabbMax.z, p.z, p.z);
  }
}

void plc_soft_computeSpringForces(plcSoftBody *sb)
{
  for (int i = 0; i < sb->springCount; i++) {
    plcSpring *sp = &sb->springs[i];
    plcSoftNode *nA = &sb->nodes[sp->nodeA];
    plcSoftNode *nB = &sb->nodes[sp->nodeB];
    plcVec3 diff = plc_vec3_sub(nB->pos, nA->pos);
    float len = plc_vec3_len(diff);
    if (len < 1e-8f) continue;
    plcVec3 dir = plc_vec3_scale(diff, 1.0f / len);
    float springForce = sp->stiffness * (len - sp->restLength);
    plcVec3 relVel = plc_vec3_sub(nB->vel, nA->vel);
    float dampingForce = sp->damping * plc_vec3_dot(relVel, dir);
    float totalForce = springForce + dampingForce;
    plcVec3 f = plc_vec3_scale(dir, totalForce);
    nA->force.x += f.x; nA->force.y += f.y; nA->force.z += f.z;
    nB->force.x -= f.x; nB->force.y -= f.y; nB->force.z -= f.z;
  }
}

void plc_soft_computeFemLinear(plcSoftBody *sb)
{
  (void)sb;
}

void plc_soft_computeFemCorotational(plcSoftBody *sb)
{
  (void)sb;
}

void plc_soft_step(plcSoftBody *sb, float dt)
{
  for (int i = 0; i < sb->nodeCount; i++) {
    plcSoftNode *n = &sb->nodes[i];
    if (n->pinned) continue;
    n->vel.x += n->force.x * n->invMass * dt;
    n->vel.y += n->force.y * n->invMass * dt;
    n->vel.z += n->force.z * n->invMass * dt;
    n->vel.x *= (1.0f - sb->damping);
    n->vel.y *= (1.0f - sb->damping);
    n->vel.z *= (1.0f - sb->damping);
    n->pos.x += n->vel.x * dt;
    n->pos.y += n->vel.y * dt;
    n->pos.z += n->vel.z * dt;
    n->force = plc_vec3(0, 0, 0);
  }
  plc_soft_updateAABB(sb);
}

void plc_soft_reset(plcSoftBody *sb)
{
  for (int i = 0; i < sb->nodeCount; i++) {
    sb->nodes[i].pos = sb->nodes[i].pos0;
    sb->nodes[i].vel = plc_vec3(0, 0, 0);
    sb->nodes[i].force = plc_vec3(0, 0, 0);
  }
}

void plc_soft_applyForce(plcSoftBody *sb, plcVec3 force, float radius, plcVec3 center)
{
  for (int i = 0; i < sb->nodeCount; i++) {
    plcVec3 diff = plc_vec3_sub(sb->nodes[i].pos, center);
    float dist = plc_vec3_len(diff);
    if (dist < radius) {
      float influence = 1.0f - dist / radius;
      sb->nodes[i].force.x += force.x * influence;
      sb->nodes[i].force.y += force.y * influence;
      sb->nodes[i].force.z += force.z * influence;
    }
  }
}

int plc_soft_collideRigid(plcSoftBody *sb, plcRigidBody *rb, plcContact *contacts, int maxContacts)
{
  (void)sb;
  (void)rb;
  (void)contacts;
  (void)maxContacts;
  return 0;
}

int plc_soft_collidePlane(plcSoftBody *sb, plcVec3 planeNormal, float planeD,
                           plcContact *contacts, int maxContacts)
{
  int count = 0;
  for (int i = 0; i < sb->nodeCount && count < maxContacts; i++) {
    float d = plc_vec3_dot(sb->nodes[i].pos, planeNormal) + planeD;
    if (d < 0) {
      contacts[count].bodyA = i;
      contacts[count].bodyB = -1;
      contacts[count].point = sb->nodes[i].pos;
      contacts[count].normal = planeNormal;
      contacts[count].penetration = -d;
      count++;
      sb->nodes[i].pos.x -= planeNormal.x * d;
      sb->nodes[i].pos.y -= planeNormal.y * d;
      sb->nodes[i].pos.z -= planeNormal.z * d;
      float vn = plc_vec3_dot(sb->nodes[i].vel, planeNormal);
      if (vn < 0) {
        sb->nodes[i].vel.x -= vn * planeNormal.x;
        sb->nodes[i].vel.y -= vn * planeNormal.y;
        sb->nodes[i].vel.z -= vn * planeNormal.z;
      }
    }
  }
  return count;
}

int plc_soft_collideSoft(plcSoftBody *sbA, plcSoftBody *sbB, plcContact *contacts, int maxContacts)
{
  (void)sbA;
  (void)sbB;
  (void)contacts;
  (void)maxContacts;
  return 0;
}
