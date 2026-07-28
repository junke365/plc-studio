#ifndef PLC_KINEMATICS_H
#define PLC_KINEMATICS_H

#include <stdint.h>
#include <stdbool.h>

/* 最大轴数 (与 plc_group.h 同步) */
#define PLC_KINS_MAX_AXIS 9

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 运动学类型 ==================== */
typedef enum {
  KINEMATICS_IDENTITY = 0,   /* 直角坐标 (X=J0, Y=J1, Z=J2) */
  KINEMATICS_CORE_XY,        /* CoreXY (H-bot) */
  KINEMATICS_DELTA,          /* Delta 并联 */
  KINEMATICS_SCARA,          /* SCARA */
  KINEMATICS_5AXIS_TRT,      /* 五轴双转台 */
  KINEMATICS_CUSTOM,         /* 用户自定义 */
} KinematicsType;

/* ==================== 运动学配置 ==================== */
typedef struct {
  KinematicsType type;
  float params[16];           /* 运动学参数 (臂长、偏距等) */
} KinematicsConfig;

/* ==================== 位姿数据结构 ==================== */
typedef struct {
  float x, y, z;              /* 直线轴 (mm) */
  float a, b, c;              /* 旋转轴 (deg) */
} CartesianPose;

typedef struct {
  float joint[PLC_KINS_MAX_AXIS];  /* 关节坐标 */
} JointPosition;

/* ==================== 运动学操作接口 ==================== */
typedef struct {
  KinematicsConfig config;
  bool valid;

  /* 正运动学: 关节 -> 笛卡尔 */
  int (*forward)(const float *joint, CartesianPose *pose, void *user);

  /* 逆运动学: 笛卡尔 -> 关节 */
  int (*inverse)(const CartesianPose *pose, float *joint, void *user);

  /* 运动学名 */
  const char *name;

  /* 用户数据 */
  void *user;
} Kinematics;

/* ==================== 内置运动学 ==================== */
/* 直角坐标 (Identity) - 每个关节直接映射到笛卡尔轴 */
int plc_kins_identity_init(Kinematics *kins);
int plc_kins_identity_forward(const float *joint, CartesianPose *pose, void *user);
int plc_kins_identity_inverse(const CartesianPose *pose, float *joint, void *user);

/* CoreXY 运动学 */
int plc_kins_corexy_init(Kinematics *kins);
int plc_kins_corexy_forward(const float *joint, CartesianPose *pose, void *user);
int plc_kins_corexy_inverse(const CartesianPose *pose, float *joint, void *user);

/* ==================== 运动学 API ==================== */
int plc_kinematics_init(Kinematics *kins, const KinematicsConfig *cfg);
int plc_kinematics_forward(Kinematics *kins, const float *joint, CartesianPose *pose);
int plc_kinematics_inverse(Kinematics *kins, const CartesianPose *pose, float *joint);
const char *plc_kinematics_name(Kinematics *kins);

/* 注册自定义运动学 */
typedef int (*KinematicsInitFn)(Kinematics *kins);
int plc_kinematics_register(KinematicsType type, KinematicsInitFn initFn);

#ifdef __cplusplus
}
#endif

#endif /* PLC_KINEMATICS_H */
