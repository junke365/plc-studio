#include "plc_kinematics.h"
#include <string.h>
#include <math.h>

/* ==================== 直角坐标 (Identity) ==================== */

int plc_kins_identity_forward(const float *joint, CartesianPose *pose, void *user)
{
  (void)user;
  if (!joint || !pose) return -1;
  pose->x = joint[0];
  pose->y = joint[1];
  pose->z = joint[2];
  pose->a = joint[3];
  pose->b = joint[4];
  pose->c = joint[5];
  return 0;
}

int plc_kins_identity_inverse(const CartesianPose *pose, float *joint, void *user)
{
  (void)user;
  if (!pose || !joint) return -1;
  joint[0] = pose->x;
  joint[1] = pose->y;
  joint[2] = pose->z;
  joint[3] = pose->a;
  joint[4] = pose->b;
  joint[5] = pose->c;
  return 0;
}

int plc_kins_identity_init(Kinematics *kins)
{
  if (!kins) return -1;
  memset(kins, 0, sizeof(Kinematics));
  kins->config.type = KINEMATICS_IDENTITY;
  kins->forward = plc_kins_identity_forward;
  kins->inverse = plc_kins_identity_inverse;
  kins->name = "Cartesian (Identity)";
  kins->valid = true;
  return 0;
}

/* ==================== CoreXY ==================== */

int plc_kins_corexy_forward(const float *joint, CartesianPose *pose, void *user)
{
  (void)user;
  if (!joint || !pose) return -1;
  /* CoreXY: X = (A+B)/2,  Y = (A-B)/2 */
  pose->x = (joint[0] + joint[1]) * 0.5f;
  pose->y = (joint[0] - joint[1]) * 0.5f;
  pose->z = joint[2];
  pose->a = 0; pose->b = 0; pose->c = 0;
  return 0;
}

int plc_kins_corexy_inverse(const CartesianPose *pose, float *joint, void *user)
{
  (void)user;
  if (!pose || !joint) return -1;
  /* CoreXY: A = X+Y,  B = X-Y */
  joint[0] = pose->x + pose->y;
  joint[1] = pose->x - pose->y;
  joint[2] = pose->z;
  joint[3] = 0; joint[4] = 0; joint[5] = 0;
  return 0;
}

int plc_kins_corexy_init(Kinematics *kins)
{
  if (!kins) return -1;
  memset(kins, 0, sizeof(Kinematics));
  kins->config.type = KINEMATICS_CORE_XY;
  kins->forward = plc_kins_corexy_forward;
  kins->inverse = plc_kins_corexy_inverse;
  kins->name = "CoreXY";
  kins->valid = true;
  return 0;
}

/* ==================== 运动学注册表 ==================== */

#define MAX_KINS_DRIVERS 8
static KinematicsInitFn s_kinsRegistry[MAX_KINS_DRIVERS];

int plc_kinematics_register(KinematicsType type, KinematicsInitFn initFn)
{
  if (type < 0 || type >= MAX_KINS_DRIVERS) return -1;
  s_kinsRegistry[type] = initFn;
  return 0;
}

int plc_kinematics_init(Kinematics *kins, const KinematicsConfig *cfg)
{
  if (!kins || !cfg) return -1;

  memset(kins, 0, sizeof(Kinematics));
  kins->config = *cfg;

  /* 尝试注册表 */
  if (cfg->type >= 0 && cfg->type < MAX_KINS_DRIVERS && s_kinsRegistry[cfg->type]) {
    return s_kinsRegistry[cfg->type](kins);
  }

  /* 内置运动学 */
  switch (cfg->type) {
  case KINEMATICS_IDENTITY:
    return plc_kins_identity_init(kins);
  case KINEMATICS_CORE_XY:
    return plc_kins_corexy_init(kins);
  default:
    /* 默认使用直角坐标 */
    return plc_kins_identity_init(kins);
  }
}

int plc_kinematics_forward(Kinematics *kins, const float *joint, CartesianPose *pose)
{
  if (!kins || !kins->forward) return -1;
  return kins->forward(joint, pose, kins->user);
}

int plc_kinematics_inverse(Kinematics *kins, const CartesianPose *pose, float *joint)
{
  if (!kins || !kins->inverse) return -1;
  return kins->inverse(pose, joint, kins->user);
}

const char *plc_kinematics_name(Kinematics *kins)
{
  return kins ? kins->name : "?";
}

/* 内置注册 */
__attribute__((constructor))
static void register_builtin_kins(void)
{
  static int registered = 0;
  if (registered) return;
  registered = 1;

  plc_kinematics_register(KINEMATICS_IDENTITY, plc_kins_identity_init);
  plc_kinematics_register(KINEMATICS_CORE_XY, plc_kins_corexy_init);
}
