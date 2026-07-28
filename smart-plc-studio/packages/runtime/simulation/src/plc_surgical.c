#include "plc_surgical.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static const float mtmDhA[7] = {0, 0, 0, 0, 0, 0, 0};
static const float mtmDhAlpha[7] = {-PLC_PI/2, PLC_PI/2, -PLC_PI/2, PLC_PI/2, -PLC_PI/2, PLC_PI/2, 0};
static const float mtmDhD[7] = {0, 0, 0.3f, 0, 0.25f, 0, 0.15f};
static const float mtmDhTheta[7] = {0, -PLC_PI/2, 0, 0, 0, 0, 0};

static const float mtmJointMin[7] = {-PLC_PI*0.8f, -PLC_PI*0.5f, -PLC_PI*0.9f, -PLC_PI*0.7f, -PLC_PI*0.9f, -PLC_PI*0.5f, -PLC_PI*0.9f};
static const float mtmJointMax[7] = { PLC_PI*0.8f,  PLC_PI*0.5f,  PLC_PI*0.9f,  PLC_PI*0.7f,  PLC_PI*0.9f,  PLC_PI*0.5f,  PLC_PI*0.9f};

static const float psmDhA[7] = {0, 0, 0, 0, 0, 0, 0};
static const float psmDhAlpha[7] = {-PLC_PI/2, PLC_PI/2, 0, -PLC_PI/2, PLC_PI/2, -PLC_PI/2, 0};
static const float psmDhD[7] = {0, 0, 0.4f, 0, 0, 0, 0.02f};
static const float psmDhTheta[7] = {0, -PLC_PI/2, 0, 0, 0, 0, 0};

static const float psmJointMin[7] = {-PLC_PI*0.5f, -PLC_PI*0.4f, 0.0f, -PLC_PI*0.9f, -PLC_PI*0.5f, -PLC_PI*0.5f, 0.0f};
static const float psmJointMax[7] = { PLC_PI*0.5f,  PLC_PI*0.4f, 0.4f,  PLC_PI*0.9f,  PLC_PI*0.5f,  PLC_PI*0.5f, 0.02f};

static plcMat4 dhTransform(float a, float alpha, float d, float theta)
{
  float ca = cosf(alpha), sa = sinf(alpha);
  float ct = cosf(theta), st = sinf(theta);
  plcMat4 m;
  memset(&m, 0, sizeof(m));
  m.m[0] = ct;       m.m[1] = -st * ca;  m.m[2] = st * sa;   m.m[3] = a * ct;
  m.m[4] = st;       m.m[5] = ct * ca;   m.m[6] = -ct * sa;  m.m[7] = a * st;
  m.m[8] = 0;        m.m[9] = sa;        m.m[10] = ca;       m.m[11] = d;
  m.m[15] = 1;
  return m;
}

static void mat4ToQuat(const plcMat4 *m, plcQuat *q)
{
  float trace = m->m[0] + m->m[5] + m->m[10];
  if (trace > 0) {
    float s = 0.5f / sqrtf(trace + 1.0f);
    q->w = 0.25f / s;
    q->x = (m->m[9] - m->m[6]) * s;
    q->y = (m->m[2] - m->m[8]) * s;
    q->z = (m->m[4] - m->m[1]) * s;
  } else if (m->m[0] > m->m[5] && m->m[0] > m->m[10]) {
    float s = 2.0f * sqrtf(1.0f + m->m[0] - m->m[5] - m->m[10]);
    q->w = (m->m[9] - m->m[6]) / s;
    q->x = 0.25f * s;
    q->y = (m->m[4] + m->m[1]) / s;
    q->z = (m->m[2] + m->m[8]) / s;
  } else if (m->m[5] > m->m[10]) {
    float s = 2.0f * sqrtf(1.0f + m->m[5] - m->m[0] - m->m[10]);
    q->w = (m->m[2] - m->m[8]) / s;
    q->x = (m->m[4] + m->m[1]) / s;
    q->y = 0.25f * s;
    q->z = (m->m[9] + m->m[6]) / s;
  } else {
    float s = 2.0f * sqrtf(1.0f + m->m[10] - m->m[0] - m->m[5]);
    q->w = (m->m[4] - m->m[1]) / s;
    q->x = (m->m[2] + m->m[8]) / s;
    q->y = (m->m[9] + m->m[6]) / s;
    q->z = 0.25f * s;
  }
}

static int solveLinear6(const float A[36], const float b[6], float x[6])
{
  float a[36];
  float bb[6];
  memcpy(a, A, 36 * sizeof(float));
  memcpy(bb, b, 6 * sizeof(float));

  for (int col = 0; col < 6; col++) {
    int pivot = col;
    float maxVal = fabsf(a[col * 6 + col]);
    for (int row = col + 1; row < 6; row++) {
      float v = fabsf(a[row * 6 + col]);
      if (v > maxVal) { maxVal = v; pivot = row; }
    }
    if (maxVal < 1e-12f) return -1;
    if (pivot != col) {
      for (int c = col; c < 6; c++) {
        float tmp = a[col * 6 + c];
        a[col * 6 + c] = a[pivot * 6 + c];
        a[pivot * 6 + c] = tmp;
      }
      float tmp = bb[col]; bb[col] = bb[pivot]; bb[pivot] = tmp;
    }
    float invPivot = 1.0f / a[col * 6 + col];
    for (int c = col; c < 6; c++) a[col * 6 + c] *= invPivot;
    bb[col] *= invPivot;
    for (int row = col + 1; row < 6; row++) {
      float factor = a[row * 6 + col];
      if (factor != 0) {
        for (int c = col; c < 6; c++)
          a[row * 6 + c] -= factor * a[col * 6 + c];
        bb[row] -= factor * bb[col];
      }
    }
  }
  for (int i = 5; i >= 0; i--) {
    x[i] = bb[i];
    for (int j = i + 1; j < 6; j++)
      x[i] -= a[i * 6 + j] * x[j];
  }
  return 0;
}

void plc_surgical_loadMtmParams(MtmParams *params)
{
  params->l0 = 0.15f;
  params->l1 = 0.30f;
  params->l2 = 0.25f;
  params->l3 = 0.12f;
  params->l4 = 0.02f;
}

void plc_surgical_loadPsmParams(PsmParams *params)
{
  params->d1 = 0.0f;
  params->d2 = 0.4f;
  params->l1 = 0.15f;
  params->l2 = 0.02f;
  params->theta1 = 0.0f;
}

int plc_surgical_init(SurgicalRobot *robot, SurgicalRobotType type)
{
  memset(robot, 0, sizeof(SurgicalRobot));
  robot->type = type;

  switch (type) {
  case SURGICAL_MTM:
    robot->dof = 7;
    memcpy(robot->dhA, mtmDhA, sizeof(mtmDhA));
    memcpy(robot->dhAlpha, mtmDhAlpha, sizeof(mtmDhAlpha));
    memcpy(robot->dhD, mtmDhD, sizeof(mtmDhD));
    memcpy(robot->dhTheta, mtmDhTheta, sizeof(mtmDhTheta));
    memcpy(robot->jointFrames, mtmDhTheta, sizeof(mtmDhTheta));
    plc_surgical_loadMtmParams(&robot->params.mtm);
    break;
  case SURGICAL_PSM:
    robot->dof = 7;
    memcpy(robot->dhA, psmDhA, sizeof(psmDhA));
    memcpy(robot->dhAlpha, psmDhAlpha, sizeof(psmDhAlpha));
    memcpy(robot->dhD, psmDhD, sizeof(psmDhD));
    memcpy(robot->dhTheta, psmDhTheta, sizeof(psmDhTheta));
    memcpy(robot->jointFrames, psmDhTheta, sizeof(psmDhTheta));
    plc_surgical_loadPsmParams(&robot->params.psm);
    break;
  default:
    return -1;
  }

  robot->toolOffset = plc_vec3(0, 0, 0);
  robot->toolOrient = plc_quat_identity();
  return 0;
}

int plc_surgical_forward(SurgicalRobot *robot, const float *jointAngles, plcVec3 *pos, plcQuat *orient)
{
  if (!robot || !jointAngles) return -1;

  plcMat4 T = plc_mat4_identity();

  for (int i = 0; i < robot->dof; i++) {
    float theta = robot->dhTheta[i] + jointAngles[i];
    plcMat4 Ai = dhTransform(robot->dhA[i], robot->dhAlpha[i], robot->dhD[i], theta);
    T = plc_mat4_mul(T, Ai);
    robot->jointFrames[i] = T;
  }

  plcMat4 toolT = plc_mat4_identity();
  toolT = plc_mat4_translate(robot->toolOffset.x, robot->toolOffset.y, robot->toolOffset.z);
  plcQuat qo = robot->toolOrient;
  plcMat4 rotM = plc_mat4_identity();
  rotM.m[0] = 1 - 2 * (qo.y * qo.y + qo.z * qo.z);
  rotM.m[1] = 2 * (qo.x * qo.y - qo.w * qo.z);
  rotM.m[2] = 2 * (qo.x * qo.z + qo.w * qo.y);
  rotM.m[4] = 2 * (qo.x * qo.y + qo.w * qo.z);
  rotM.m[5] = 1 - 2 * (qo.x * qo.x + qo.z * qo.z);
  rotM.m[6] = 2 * (qo.y * qo.z - qo.w * qo.x);
  rotM.m[8] = 2 * (qo.x * qo.z - qo.w * qo.y);
  rotM.m[9] = 2 * (qo.y * qo.z + qo.w * qo.x);
  rotM.m[10] = 1 - 2 * (qo.x * qo.x + qo.y * qo.y);
  T = plc_mat4_mul(T, toolT);
  rotM.m[12] = T.m[12]; rotM.m[13] = T.m[13]; rotM.m[14] = T.m[14];
  T = rotM;

  if (pos) {
    pos->x = T.m[12];
    pos->y = T.m[13];
    pos->z = T.m[14];
  }

  if (orient) {
    plcMat4 rotOnly;
    rotOnly.m[0] = T.m[0]; rotOnly.m[4] = T.m[4]; rotOnly.m[8]  = T.m[8];  rotOnly.m[12] = 0;
    rotOnly.m[1] = T.m[1]; rotOnly.m[5] = T.m[5]; rotOnly.m[9]  = T.m[9];  rotOnly.m[13] = 0;
    rotOnly.m[2] = T.m[2]; rotOnly.m[6] = T.m[6]; rotOnly.m[10] = T.m[10]; rotOnly.m[14] = 0;
    rotOnly.m[3] = 0;      rotOnly.m[7] = 0;      rotOnly.m[11] = 0;       rotOnly.m[15] = 1;
    mat4ToQuat(&rotOnly, orient);
  }

  robot->tcpPosition = *pos;
  robot->tcpTransform = T;
  memcpy(robot->qCurrent, jointAngles, robot->dof * sizeof(float));
  return 0;
}

void plc_surgical_jacobian(SurgicalRobot *robot, const float *q, float *J)
{
  plcVec3 tcpPos;
  plcQuat tcpOrient;
  plc_surgical_forward(robot, q, &tcpPos, &tcpOrient);

  memset(J, 0, 6 * robot->dof * sizeof(float));

  for (int i = 0; i < robot->dof; i++) {
    plcVec3 z_i, p_i;
    z_i.x = robot->jointFrames[i].m[8];
    z_i.y = robot->jointFrames[i].m[9];
    z_i.z = robot->jointFrames[i].m[10];
    p_i.x = robot->jointFrames[i].m[12];
    p_i.y = robot->jointFrames[i].m[13];
    p_i.z = robot->jointFrames[i].m[14];

    float jointType = robot->dhAlpha[i];
    int isPrismatic = (fabsf(jointType - 0) < PLC_EPS && fabsf(robot->dhA[i] - 0) < PLC_EPS &&
                      fabsf(robot->dhD[i]) > PLC_EPS) ? 1 : 0;

    if (isPrismatic) {
      J[6 * i + 0] = z_i.x;
      J[6 * i + 1] = z_i.y;
      J[6 * i + 2] = z_i.z;
      J[6 * i + 3] = 0;
      J[6 * i + 4] = 0;
      J[6 * i + 5] = 0;
    } else {
      plcVec3 diff = plc_vec3_sub(tcpPos, p_i);
      plcVec3 v = plc_vec3_cross(z_i, diff);
      J[6 * i + 0] = v.x;
      J[6 * i + 1] = v.y;
      J[6 * i + 2] = v.z;
      J[6 * i + 3] = z_i.x;
      J[6 * i + 4] = z_i.y;
      J[6 * i + 5] = z_i.z;
    }
  }
}

int plc_surgical_inverse(SurgicalRobot *robot, plcVec3 targetPos, plcQuat targetOrient,
                          float *jointAngles, int maxSolutions)
{
  if (!robot || !jointAngles) return 0;

  float q[8];
  memcpy(q, robot->qCurrent, robot->dof * sizeof(float));

  int maxIter = 150;
  float lambda = 1.0f;
  float lambdaMin = 1e-6f;
  float posTol = 1e-5f;
  float orientTol = 1e-4f;

  for (int iter = 0; iter < maxIter; iter++) {
    plcVec3 curPos;
    plcQuat curOrient;
    plc_surgical_forward(robot, q, &curPos, &curOrient);

    plcVec3 posErr;
    posErr.x = targetPos.x - curPos.x;
    posErr.y = targetPos.y - curPos.y;
    posErr.z = targetPos.z - curPos.z;

    plcQuat qInv;
    qInv.x = -curOrient.x;
    qInv.y = -curOrient.y;
    qInv.z = -curOrient.z;
    qInv.w = curOrient.w;
    float norm = sqrtf(qInv.x * qInv.x + qInv.y * qInv.y + qInv.z * qInv.z + qInv.w * qInv.w);
    if (norm > 0) { qInv.x /= norm; qInv.y /= norm; qInv.z /= norm; qInv.w /= norm; }

    plcQuat qErr = plc_quat_mul(targetOrient, qInv);

    plcVec3 orientErr;
    float qErrNorm = sqrtf(qErr.x * qErr.x + qErr.y * qErr.y + qErr.z * qErr.z);
    if (qErrNorm > 1e-10f && fabsf(qErr.w) < 1.0f) {
      float angle = 2.0f * atan2f(qErrNorm, fabsf(qErr.w));
      if (qErr.w < 0) angle = -angle;
      float scale = angle / qErrNorm;
      orientErr.x = qErr.x * scale;
      orientErr.y = qErr.y * scale;
      orientErr.z = qErr.z * scale;
    } else {
      orientErr.x = 2.0f * qErr.x;
      orientErr.y = 2.0f * qErr.y;
      orientErr.z = 2.0f * qErr.z;
    }

    float posErrNorm = sqrtf(posErr.x * posErr.x + posErr.y * posErr.y + posErr.z * posErr.z);
    float orientErrNorm = sqrtf(orientErr.x * orientErr.x + orientErr.y * orientErr.y + orientErr.z * orientErr.z);

    if (posErrNorm < posTol && orientErrNorm < orientTol) {
      memcpy(jointAngles, q, robot->dof * sizeof(float));
      return 1;
    }

    float J[6 * 8];
    plc_surgical_jacobian(robot, q, J);

    float JJt[36];
    memset(JJt, 0, sizeof(JJt));
    for (int r = 0; r < 6; r++)
      for (int c = 0; c < 6; c++)
        for (int k = 0; k < robot->dof; k++)
          JJt[r * 6 + c] += J[r + 6 * k] * J[c + 6 * k];

    float lambdaSq = lambda * lambda;
    for (int i = 0; i < 6; i++)
      JJt[i * 6 + i] += lambdaSq;

    float error[6] = {posErr.x, posErr.y, posErr.z, orientErr.x, orientErr.y, orientErr.z};
    float x[6];

    if (solveLinear6(JJt, error, x) != 0) {
      lambda *= 2.0f;
      continue;
    }

    float dq[8] = {0};
    for (int k = 0; k < robot->dof; k++)
      for (int r = 0; r < 6; r++)
        dq[k] += J[r + 6 * k] * x[r];

    float dqNorm = 0;
    for (int k = 0; k < robot->dof; k++) dqNorm += dq[k] * dq[k];
    dqNorm = sqrtf(dqNorm);
    float maxStep = 0.3f;
    if (dqNorm > maxStep) {
      float scale = maxStep / dqNorm;
      for (int k = 0; k < robot->dof; k++) dq[k] *= scale;
    }

    for (int k = 0; k < robot->dof; k++)
      q[k] += dq[k];

    lambda *= 0.95f;
    if (lambda < lambdaMin) lambda = lambdaMin;
  }

  for (int i = 0; i < robot->dof; i++) {
    if (isnan(q[i]) || isinf(q[i])) return 0;
  }

  plcVec3 finalPos;
  plcQuat finalOrient;
  plc_surgical_forward(robot, q, &finalPos, &finalOrient);
  plcVec3 pe;
  pe.x = targetPos.x - finalPos.x;
  pe.y = targetPos.y - finalPos.y;
  pe.z = targetPos.z - finalPos.z;
  float fe = sqrtf(pe.x * pe.x + pe.y * pe.y + pe.z * pe.z);
  if (fe < 1e-3f) {
    memcpy(jointAngles, q, robot->dof * sizeof(float));
    return 1;
  }
  return 0;
}

float plc_surgical_singularity(SurgicalRobot *robot, const float *q)
{
  float J[6 * 8];
  plc_surgical_jacobian(robot, q, J);

  float JJt[36];
  memset(JJt, 0, sizeof(JJt));
  for (int r = 0; r < 6; r++)
    for (int c = 0; c < 6; c++)
      for (int k = 0; k < robot->dof; k++)
        JJt[r * 6 + c] += J[r + 6 * k] * J[c + 6 * k];

  float a[36];
  memcpy(a, JJt, sizeof(JJt));
  float det = 1.0f;
  for (int col = 0; col < 6; col++) {
    int pivot = col;
    float maxVal = fabsf(a[col * 6 + col]);
    for (int row = col + 1; row < 6; row++) {
      float v = fabsf(a[row * 6 + col]);
      if (v > maxVal) { maxVal = v; pivot = row; }
    }
    if (maxVal < 1e-12f) return 0;
    if (pivot != col) {
      for (int c = col; c < 6; c++) {
        float tmp = a[col * 6 + c];
        a[col * 6 + c] = a[pivot * 6 + c];
        a[pivot * 6 + c] = tmp;
      }
      det = -det;
    }
    float pv = a[col * 6 + col];
    det *= pv;
    float invPv = 1.0f / pv;
    for (int row = col + 1; row < 6; row++) {
      float factor = a[row * 6 + col] * invPv;
      if (factor != 0)
        for (int c = col; c < 6; c++)
          a[row * 6 + c] -= factor * a[col * 6 + c];
    }
  }

  robot->jacobianDet = det;
  return det;
}

int plc_surgical_inWorkspace(SurgicalRobot *robot, plcVec3 pos)
{
  float dist = sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
  switch (robot->type) {
  case SURGICAL_MTM:
    return (dist > 0.1f && dist < 0.8f) ? 1 : 0;
  case SURGICAL_PSM:
    return (pos.y > -0.1f && dist > 0.1f && dist < 0.7f) ? 1 : 0;
  default:
    return 1;
  }
}

int plc_surgical_checkJoints(SurgicalRobot *robot, const float *q)
{
  const float *jMin, *jMax;
  switch (robot->type) {
  case SURGICAL_MTM: jMin = mtmJointMin; jMax = mtmJointMax; break;
  case SURGICAL_PSM: jMin = psmJointMin; jMax = psmJointMax; break;
  default: return 1;
  }
  for (int i = 0; i < robot->dof; i++) {
    if (q[i] < jMin[i] || q[i] > jMax[i]) return 0;
  }
  return 1;
}

void plc_surgical_jointInterp(SurgicalRobot *robot, const float *qStart, const float *qEnd,
                               float t, float *qOut)
{
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  for (int i = 0; i < robot->dof; i++) {
    float diff = qEnd[i] - qStart[i];
    if (diff > PLC_PI) diff -= 2 * PLC_PI;
    else if (diff < -PLC_PI) diff += 2 * PLC_PI;
    qOut[i] = qStart[i] + diff * t;
  }
}

void plc_surgical_cartesianInterp(SurgicalRobot *robot, plcVec3 pStart, plcQuat qStart,
                                   plcVec3 pEnd, plcQuat qEnd, float t,
                                   plcVec3 *pOut, plcQuat *qOut)
{
  (void)robot;
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  pOut->x = pStart.x + (pEnd.x - pStart.x) * t;
  pOut->y = pStart.y + (pEnd.y - pStart.y) * t;
  pOut->z = pStart.z + (pEnd.z - pStart.z) * t;
  float dot = qStart.x * qEnd.x + qStart.y * qEnd.y + qStart.z * qEnd.z + qStart.w * qEnd.w;
  if (dot < 0) {
    qEnd.x = -qEnd.x; qEnd.y = -qEnd.y; qEnd.z = -qEnd.z; qEnd.w = -qEnd.w;
    dot = -dot;
  }
  float s = 1 - t;
  qOut->x = s * qStart.x + t * qEnd.x;
  qOut->y = s * qStart.y + t * qEnd.y;
  qOut->z = s * qStart.z + t * qEnd.z;
  qOut->w = s * qStart.w + t * qEnd.w;
  float len = sqrtf(qOut->x * qOut->x + qOut->y * qOut->y + qOut->z * qOut->z + qOut->w * qOut->w);
  if (len > 0) {
    qOut->x /= len; qOut->y /= len; qOut->z /= len; qOut->w /= len;
  }
}

void plc_surgical_update(SurgicalRobot *robot)
{
  if (!robot) return;
  plc_surgical_forward(robot, robot->qCurrent, &robot->tcpPosition, NULL);
  plc_surgical_singularity(robot, robot->qCurrent);
}

void plc_surgical_psm_setCannulaAngle(SurgicalRobot *robot, float angle)
{
  robot->params.psm.theta1 = angle;
  robot->dhTheta[1] = -PLC_PI/2 + angle;
}

void plc_surgical_psm_grip(SurgicalRobot *robot, float openRatio)
{
  if (!robot || robot->type != SURGICAL_PSM) return;
  if (openRatio < 0) openRatio = 0;
  if (openRatio > 1) openRatio = 1;
  float gripAngle = openRatio * 0.5f;
  robot->dhTheta[6] = gripAngle;
  robot->qCurrent[6] = gripAngle;
}

void plc_surgical_setToolOffset(SurgicalRobot *robot, plcVec3 offset, plcQuat orient)
{
  robot->toolOffset = offset;
  robot->toolOrient = orient;
}
