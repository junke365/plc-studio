#include "plc_multibody.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ==================== 辅助：列主序 3x3 矩阵乘向量 ==================== */
static inline plcVec3 mat3_mul_vec3(const plcMat3 *m, plcVec3 v)
{
  plcVec3 r;
  r.x = m->m[0] * v.x + m->m[3] * v.y + m->m[6] * v.z;
  r.y = m->m[1] * v.x + m->m[4] * v.y + m->m[7] * v.z;
  r.z = m->m[2] * v.x + m->m[5] * v.y + m->m[8] * v.z;
  return r;
}

/* ==================== 辅助：获取 DOF 数量 ==================== */
static int get_dof(const plcMultiBody *mb)
{
  int n = 0;
  for (int i = 0; i < mb->jointCount; i++)
    if (mb->joints[i].type != JOINT_FIXED) n++;
  return n;
}

/* ==================== 辅助：将 q/qd/qdd 写入关节并返回非固定关节的 DOF 索引映射 ==================== */
static int write_joint_states(plcMultiBody *mb, const float *q, const float *qd, const float *qdd)
{
  int d = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type != JOINT_FIXED) {
      mb->joints[i].q.x   = q   ? q[d]   : 0.0f;
      mb->joints[i].qd.x  = qd  ? qd[d]  : 0.0f;
      mb->joints[i].qdd.x = qdd ? qdd[d] : 0.0f;
      d++;
    }
  }
  return d;
}

/* ==================== 辅助：建立从根到叶的遍历顺序 ==================== */
static int build_order(const plcMultiBody *mb, int *order)
{
  int queue[PLC_MAX_LINKS], head = 0, tail = 0, len = 0;
  if (mb->rootLink < 0) return 0;
  queue[tail++] = mb->rootLink;
  while (head < tail) {
    int idx = queue[head++];
    order[len++] = idx;
    const plcLink *link = &mb->links[idx];
    for (int c = 0; c < link->childCount; c++)
      queue[tail++] = link->children[c];
  }
  return len;
}

/* ==================== 辅助：查找进入每个非根连杆的关节 ==================== */
static void find_link_joints(const plcMultiBody *mb, int *linkJoint)
{
  for (int i = 0; i < mb->linkCount; i++) linkJoint[i] = -1;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type == JOINT_FIXED) continue;
    int child = mb->joints[i].childLink;
    if (child >= 0 && child < mb->linkCount)
      linkJoint[child] = i;
  }
}

/* ==================== 辅助：关节索引 → DOF 索引 ==================== */
static int joint_to_dof(const plcMultiBody *mb, int jIdx)
{
  int d = 0;
  for (int i = 0; i < jIdx; i++) {
    if (mb->joints[i].type != JOINT_FIXED) d++;
  }
  return d;
}

/* ==================== 辅助：高斯消元解 Ax = b（A 行主序 n×n，b 被覆写为 x） ==================== */
static int solve_linear(float *A, float *b, int n)
{
  float *aug = (float *)malloc(n * (n + 1) * sizeof(float));
  if (!aug) return -1;

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++)
      aug[i * (n + 1) + j] = A[i * n + j];
    aug[i * (n + 1) + n] = b[i];
  }

  for (int col = 0; col < n; col++) {
    int best = col;
    for (int row = col + 1; row < n; row++) {
      if (fabsf(aug[row * (n + 1) + col]) > fabsf(aug[best * (n + 1) + col]))
        best = row;
    }
    float piv = aug[best * (n + 1) + col];
    if (fabsf(piv) < 1e-15f) continue;
    if (best != col) {
      for (int j = col; j <= n; j++) {
        float tmp = aug[col * (n + 1) + j];
        aug[col * (n + 1) + j] = aug[best * (n + 1) + j];
        aug[best * (n + 1) + j] = tmp;
      }
    }
    for (int row = col + 1; row < n; row++) {
      float factor = aug[row * (n + 1) + col] / piv;
      for (int j = col; j <= n; j++)
        aug[row * (n + 1) + j] -= factor * aug[col * (n + 1) + j];
    }
  }

  for (int i = n - 1; i >= 0; i--) {
    float sum = aug[i * (n + 1) + n];
    for (int j = i + 1; j < n; j++)
      sum -= aug[i * (n + 1) + j] * b[j];
    float piv = aug[i * (n + 1) + i];
    b[i] = (fabsf(piv) < 1e-15f) ? 0.0f : sum / piv;
  }

  free(aug);
  return 0;
}

/* ================================================================ */
/*                      已实现的公共 API                              */
/* ================================================================ */

void plc_mb_init(plcMultiBody *mb, plcVec3 gravity)
{
  memset(mb, 0, sizeof(plcMultiBody));
  mb->gravity = gravity;
  mb->rootLink = -1;
}

int plc_mb_addLink(plcMultiBody *mb, int parentIdx, plcVec3 com, float mass)
{
  if (mb->linkCount >= PLC_MAX_LINKS) return -1;
  int idx = mb->linkCount++;
  plcLink *link = &mb->links[idx];
  memset(link, 0, sizeof(plcLink));
  link->comLocal = com;
  link->mass = mass;
  link->body.mass = mass;
  link->body.dynamic = (mass > 0);
  link->parent = parentIdx;
  if (parentIdx >= 0) {
    plcLink *parent = &mb->links[parentIdx];
    if (parent->childCount < 8)
      parent->children[parent->childCount++] = idx;
  } else {
    mb->rootLink = idx;
  }
  return idx;
}

int plc_mb_addJoint(plcMultiBody *mb, int parent, int child, JointType type, plcVec3 axis, plcVec3 pivot)
{
  if (mb->jointCount >= PLC_MAX_JOINTS) return -1;
  int idx = mb->jointCount++;
  plcJoint *j = &mb->joints[idx];
  memset(j, 0, sizeof(plcJoint));
  j->type = type;
  j->parentLink = parent;
  j->childLink = child;
  j->axis = axis;
  j->pivotParent = pivot;
  j->qMin = -3.14159f;
  j->qMax = 3.14159f;
  j->hasLimits = true;
  return idx;
}

void plc_mb_build(plcMultiBody *mb)
{
  int dof = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type != JOINT_FIXED)
      dof++;
  }
  mb->hA = (float *)calloc(dof * 4, sizeof(float));
  mb->cA = (float *)calloc(dof * 4, sizeof(float));
  mb->tau = (float *)calloc(dof * 4, sizeof(float));
  mb->qddResult = (float *)calloc(dof * 4, sizeof(float));
}

void plc_mb_forwardKinematics(plcMultiBody *mb)
{
  for (int i = 0; i < mb->linkCount; i++) {
    plcLink *link = &mb->links[i];
    if (link->parent < 0) {
      link->worldTransform = plc_mat4_identity();
      link->worldTransform = plc_mat4_translate(link->comLocal.x, link->comLocal.y, link->comLocal.z);
    } else {
      plcLink *parent = &mb->links[link->parent];
      plcMat4 local = plc_mat4_identity();
      local = plc_mat4_translate(link->comLocal.x, link->comLocal.y, link->comLocal.z);
      link->worldTransform = plc_mat4_mul(parent->worldTransform, local);
    }
    link->body.pos.x = link->worldTransform.m[12];
    link->body.pos.y = link->worldTransform.m[13];
    link->body.pos.z = link->worldTransform.m[14];
  }
}

/* ================================================================ */
/*  逆动力学：递归牛顿-欧拉算法（RNEA）                                */
/*  正向递推：从根到叶计算 ω, α, a                                    */
/*  反向递推：从叶到根计算 F, T，投影到关节轴得到力矩                   */
/* ================================================================ */
void plc_mb_rnea(plcMultiBody *mb, const float *q, const float *qd, const float *qdd, float *tauOut)
{
  int nJ = mb->jointCount;
  if (nJ == 0) return;

  /* 将 q/qd/qdd 写入关节 */
  write_joint_states(mb, q, qd, qdd);

  /* 每个连杆的"进入关节"映射 */
  int linkJoint[PLC_MAX_LINKS];
  find_link_joints(mb, linkJoint);

  /* BFS 遍历顺序 */
  int order[PLC_MAX_LINKS];
  int orderLen = build_order(mb, order);
  if (orderLen == 0) return;

  /* ========== 正向递推：运动学 ========== */
  plcVec3 omega[PLC_MAX_LINKS];
  plcVec3 alpha[PLC_MAX_LINKS];
  plcVec3 acc[PLC_MAX_LINKS];

  for (int oi = 0; oi < orderLen; oi++) {
    int idx = order[oi];
    plcLink *link = &mb->links[idx];

    if (idx == mb->rootLink) {
      /* 根连杆：角速度/角加速度为零，线加速度 = -gravity（补偿重力） */
      omega[idx] = plc_vec3(0, 0, 0);
      alpha[idx] = plc_vec3(0, 0, 0);
      acc[idx] = plc_vec3_scale(mb->gravity, -1.0f);
    } else {
      int jIdx = linkJoint[idx];
      if (jIdx < 0) continue;
      plcJoint *joint = &mb->joints[jIdx];
      int parent = joint->parentLink;
      if (parent < 0) continue;

      plcVec3 axis = joint->axis;
      float qd_i = joint->qd.x;
      float qdd_i = joint->qdd.x;

      plcVec3 wParent = omega[parent];
      plcVec3 aParent = alpha[parent];

      /* ω_i = ω_parent + qd_i * axis */
      omega[idx] = plc_vec3_add(wParent, plc_vec3_scale(axis, qd_i));

      /* α_i = α_parent + qdd_i * axis + qd_i * (ω_parent × axis) */
      plcVec3 crossWA = plc_vec3_cross(wParent, axis);
      alpha[idx] = plc_vec3_add(aParent, plc_vec3_scale(axis, qdd_i));
      alpha[idx] = plc_vec3_add(alpha[idx], plc_vec3_scale(crossWA, qd_i));

      /* a_i = a_parent + α_i × r_i + ω_i × (ω_i × r_i) */
      plcVec3 r = link->comLocal;
      plcVec3 aCrossR = plc_vec3_cross(alpha[idx], r);
      plcVec3 wCrossR = plc_vec3_cross(omega[idx], r);
      plcVec3 wCrossWCrossR = plc_vec3_cross(omega[idx], wCrossR);
      acc[idx] = plc_vec3_add(acc[parent], aCrossR);
      acc[idx] = plc_vec3_add(acc[idx], wCrossWCrossR);
    }
  }

  /* ========== 反向递推：动力学 ========== */
  plcVec3 f[PLC_MAX_LINKS];
  plcVec3 n[PLC_MAX_LINKS];
  memset(f, 0, sizeof(f));
  memset(n, 0, sizeof(n));

  /* 清零 tauOut */
  int dof = get_dof(mb);
  for (int d = 0; d < dof; d++) tauOut[d] = 0.0f;

  for (int oi = orderLen - 1; oi >= 0; oi--) {
    int idx = order[oi];
    plcLink *link = &mb->links[idx];

    /* F_i = m_i * a_i */
    f[idx] = plc_vec3_scale(acc[idx], link->mass);

    /* T_i = I_i * α_i + ω_i × (I_i * ω_i) */
    plcVec3 IAlpha = mat3_mul_vec3(&link->inertiaLocal, alpha[idx]);
    plcVec3 IOmega = mat3_mul_vec3(&link->inertiaLocal, omega[idx]);
    n[idx] = plc_vec3_add(IAlpha, plc_vec3_cross(omega[idx], IOmega));

    /* 累加子连杆的力和力矩 */
    for (int c = 0; c < link->childCount; c++) {
      int childIdx = link->children[c];
      f[idx] = plc_vec3_add(f[idx], f[childIdx]);
      /* n += n_child + comLocal_child × F_child */
      plcVec3 rCrossF = plc_vec3_cross(mb->links[childIdx].comLocal, f[childIdx]);
      n[idx] = plc_vec3_add(n[idx], n[childIdx]);
      n[idx] = plc_vec3_add(n[idx], rCrossF);
    }

    /* 如果不是根连杆，计算进入关节的力矩 */
    int jIdx = linkJoint[idx];
    if (jIdx >= 0) {
      plcJoint *joint = &mb->joints[jIdx];
      if (joint->type != JOINT_FIXED) {
        int dIdx = joint_to_dof(mb, jIdx);
        plcVec3 axis = joint->axis;
        if (joint->type == JOINT_REVOLUTE)
          tauOut[dIdx] = plc_vec3_dot(n[idx], axis);
        else /* JOINT_PRISMATIC */
          tauOut[dIdx] = plc_vec3_dot(f[idx], axis);
      }
    }
  }
}

/* ================================================================ */
/*  质量矩阵：通过 RNEA 逐列计算                                     */
/*  M[:,j] = RNEA(q, 0, e_j) - RNEA(q, 0, 0)                       */
/* ================================================================ */
void plc_mb_massMatrix(plcMultiBody *mb, const float *q, float *M)
{
  int dof = get_dof(mb);
  if (dof == 0) return;

  /* 偏置：RNEA(q, 0, 0) */
  float *tau0 = (float *)calloc(dof, sizeof(float));
  float *qddZero = (float *)calloc(dof, sizeof(float));
  float *qdZero = (float *)calloc(dof, sizeof(float));
  plc_mb_rnea(mb, q, qdZero, qddZero, tau0);

  float *qddEj = (float *)calloc(dof, sizeof(float));
  float *tauJ = (float *)calloc(dof, sizeof(float));

  for (int j = 0; j < dof; j++) {
    memset(qddEj, 0, dof * sizeof(float));
    qddEj[j] = 1.0f;
    plc_mb_rnea(mb, q, qdZero, qddEj, tauJ);
    for (int i = 0; i < dof; i++)
      M[i * dof + j] = tauJ[i] - tau0[i];
  }

  free(tau0);
  free(qddZero);
  free(qdZero);
  free(qddEj);
  free(tauJ);
}

/* ================================================================ */
/*  科里奥利和重力项：c = RNEA(q, qd, 0)                            */
/* ================================================================ */
void plc_mb_coriolisGravity(plcMultiBody *mb, const float *q, const float *qd, float *c)
{
  int dof = get_dof(mb);
  if (dof == 0) return;

  float *qddZero = (float *)calloc(dof, sizeof(float));
  plc_mb_rnea(mb, q, qd, qddZero, c);
  free(qddZero);
}

/* ================================================================ */
/*  正动力学：通过质量矩阵方法简化实现                                */
/*  1. c = RNEA(q, qd, 0)                                           */
/*  2. 计算质量矩阵 M                                                */
/*  3. 解 M * qdd = tau - c                                         */
/* ================================================================ */
void plc_mb_aba(plcMultiBody *mb, const float *q, const float *qd, const float *tau, float *qddOut)
{
  int dof = get_dof(mb);
  if (dof == 0) return;

  /* 计算偏置力矩 c = RNEA(q, qd, 0) */
  float *c = (float *)calloc(dof, sizeof(float));
  plc_mb_coriolisGravity(mb, q, qd, c);

  /* 计算质量矩阵 M（dof × dof，行主序） */
  float *M = (float *)malloc(dof * dof * sizeof(float));
  plc_mb_massMatrix(mb, q, M);

  /* 组装右侧 b = tau - c */
  for (int i = 0; i < dof; i++)
    qddOut[i] = tau ? tau[i] - c[i] : -c[i];

  /* 解 M * qdd = b */
  solve_linear(M, qddOut, dof);

  /* 结果已存入 qddOut */
  free(c);
  free(M);

  /* 同时更新关节的 qdd */
  int d = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type != JOINT_FIXED) {
      mb->joints[i].qdd.x = qddOut[d++];
    }
  }
}

/* ================================================================ */
/*  几何雅可比矩阵：6×DOF                                           */
/*  前 3 行 = 线速度雅可比，后 3 行 = 角速度雅可比                   */
/* ================================================================ */
void plc_mb_jacobian(plcMultiBody *mb, int linkIdx, plcVec3 point, const float *q, float *J)
{
  int dof = get_dof(mb);
  if (dof == 0) return;

  /* 更新关节位置并重算正向运动学 */
  write_joint_states(mb, q, NULL, NULL);
  plc_mb_forwardKinematics(mb);

  /* 清零 J */
  memset(J, 0, 6 * dof * sizeof(float));

  /* 目标连杆的位置 + 局部点偏移 */
  plcVec3 linkPos = mb->links[linkIdx].body.pos;
  plcVec3 worldPoint = plc_vec3_add(linkPos, point);

  /* 从目标连杆向上回溯到根，对路径上每个关节计算贡献 */
  int current = linkIdx;
  while (current != mb->rootLink && current >= 0) {
    int jIdx = -1;
    for (int j = 0; j < mb->jointCount; j++) {
      if (mb->joints[j].childLink == current) {
        jIdx = j;
        break;
      }
    }
    if (jIdx < 0) break;

    plcJoint *joint = &mb->joints[jIdx];
    if (joint->type != JOINT_FIXED) {
      int dIdx = joint_to_dof(mb, jIdx);
      int parent = joint->parentLink;
      plcVec3 parentPos = (parent >= 0) ? mb->links[parent].body.pos : plc_vec3(0, 0, 0);
      plcVec3 r = plc_vec3_sub(worldPoint, parentPos);
      plcVec3 axis = joint->axis;

      if (joint->type == JOINT_REVOLUTE) {
        plcVec3 v = plc_vec3_cross(axis, r);
        J[0 * dof + dIdx] = v.x;
        J[1 * dof + dIdx] = v.y;
        J[2 * dof + dIdx] = v.z;
        J[3 * dof + dIdx] = axis.x;
        J[4 * dof + dIdx] = axis.y;
        J[5 * dof + dIdx] = axis.z;
      } else if (joint->type == JOINT_PRISMATIC) {
        J[0 * dof + dIdx] = axis.x;
        J[1 * dof + dIdx] = axis.y;
        J[2 * dof + dIdx] = axis.z;
        /* 角速度部分为 0 */
      }
    }
    current = joint->parentLink;
  }
}

/* ================================================================ */
/*  半隐式欧拉积分                                                   */
/*  q += qd * dt;  计算 qdd;  qd += qdd * dt                        */
/* ================================================================ */
void plc_mb_step(plcMultiBody *mb, float dt)
{
  int dof = get_dof(mb);
  if (dof == 0) return;

  /* 组装当前 q, qd 数组 */
  float *qArr = (float *)malloc(dof * sizeof(float));
  float *qdArr = (float *)malloc(dof * sizeof(float));
  float *qddArr = (float *)malloc(dof * sizeof(float));

  int d = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type != JOINT_FIXED) {
      qArr[d] = mb->joints[i].q.x;
      qdArr[d] = mb->joints[i].qd.x;
      d++;
    }
  }

  /* 半隐式：先用速度更新位置 */
  for (int i = 0; i < dof; i++)
    qArr[i] += qdArr[i] * dt;

  /* 用 ABA 计算加速度（外力矩 = 0） */
  float *tauZero = (float *)calloc(dof, sizeof(float));
  plc_mb_aba(mb, qArr, qdArr, tauZero, qddArr);
  free(tauZero);

  /* 更新速度 */
  d = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    if (mb->joints[i].type != JOINT_FIXED) {
      mb->joints[i].q.x = qArr[d];
      mb->joints[i].qd.x += qddArr[d] * dt;
      mb->joints[i].qdd.x = qddArr[d];
      d++;
    }
  }

  /* 更新正向运动学 */
  plc_mb_forwardKinematics(mb);

  free(qArr);
  free(qdArr);
  free(qddArr);
}

/* ================================================================ */
/*  关节限位检查：返回超出限位的关节数量                              */
/* ================================================================ */
int plc_mb_checkLimits(plcMultiBody *mb)
{
  int count = 0;
  for (int i = 0; i < mb->jointCount; i++) {
    plcJoint *j = &mb->joints[i];
    if (j->hasLimits && j->type != JOINT_FIXED) {
      if (j->q.x < j->qMin || j->q.x > j->qMax)
        count++;
    }
  }
  return count;
}
