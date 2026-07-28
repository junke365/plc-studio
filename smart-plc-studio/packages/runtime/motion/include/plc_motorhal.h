#ifndef PLC_MOTORHAL_H
#define PLC_MOTORHAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 电机驱动类型 ==================== */
typedef enum {
  MOTOR_DRV_NONE = 0,
  MOTOR_DRV_STEPDIR,       /* 步进/方向脉冲 */
  MOTOR_DRV_ETHERCAT,      /* EtherCAT CiA402 */
  MOTOR_DRV_CANOPEN,       /* CANopen DS402 */
  MOTOR_DRV_MODBUS,        /* Modbus 伺服/VFD */
  MOTOR_DRV_PWM_DIR,       /* PWM + 方向 (直流电机) */
} MotorDriverType;

/* ==================== 电机状态 ==================== */
typedef enum {
  MOTOR_STATUS_DISABLED = 0,
  MOTOR_STATUS_ENABLED,
  MOTOR_STATUS_FAULT,
  MOTOR_STATUS_HOMING,
  MOTOR_STATUS_STOPPED,
} MotorStatus;

/* ==================== 电机配置 ==================== */
typedef struct {
  MotorDriverType drvType;
  uint8_t axisId;             /* 轴 ID (0-31) */

  /* 单位: SI (mm, mm/s, mm/s²) */
  float maxVelocity;          /* 最大速度 */
  float maxAcceleration;      /* 最大加速度 */
  float maxJerk;              /* 最大加加速度 */
  float softLimitPos;         /* 正向软限位 */
  float softLimitNeg;         /* 负向软限位 */
  float followingErrorMax;    /* 最大跟随误差 */

  /* 回零参数 */
  float homingVelocityFast;   /* 快速找零速度 */
  float homingVelocitySlow;   /* 精确找零速度 */
  float homingBackoff;        /* 回退距离 */
  int8_t homingDirection;     /* 回零方向 (±1) */

  /* 驱动特定参数联合体 */
  union {
    struct {
      uint32_t stepPin;       /* 步进脉冲 GPIO */
      uint32_t dirPin;        /* 方向 GPIO */
      uint32_t enablePin;     /* 使能 GPIO */
      float pulsePerMm;       /* 脉冲/毫米 */
      uint32_t maxPulseFreq;  /* 最大脉冲频率 (Hz) */
    } stepdir;

    struct {
      uint16_t slaveId;       /* EtherCAT 从站 ID */
      uint16_t vendorId;      /* 制造商 ID */
      uint32_t profileNo;     /* 驱动配置文件号 (通常 402) */
    } ethercat;

    struct {
      uint8_t nodeId;         /* CANopen 节点 ID */
      uint16_t cobIdSdo;      /* SDO COB-ID 基地址 */
    } canopen;

    struct {
      uint8_t slaveAddr;      /* Modbus 从站地址 */
      uint16_t regControl;    /* 控制字寄存器 */
      uint16_t regStatus;     /* 状态字寄存器 */
      uint16_t regSpeed;      /* 速度设定寄存器 */
      uint16_t regPos;        /* 位置设定寄存器 (双字) */
    } modbus;
  } drv;
} MotorConfig;

/* ==================== 电机 HAL 操作接口 ==================== */
typedef struct MotorHal MotorHal;

struct MotorHal {
  /* 配置 */
  MotorConfig config;

  /* 运行时状态 */
  MotorStatus status;
  float commandPos;           /* 指令位置 (mm) */
  float commandVel;           /* 指令速度 (mm/s) */
  float actualPos;            /* 实际位置 (mm) */
  float actualVel;            /* 实际速度 (mm/s) */
  float followingError;       /* 跟随误差 */

  /* --- 虚函数表 (由后端实现) --- */
  int (*init)(MotorHal *motor);
  int (*deinit)(MotorHal *motor);
  int (*enable)(MotorHal *motor, bool on);
  int (*setPos)(MotorHal *motor, float posMm);    /* 绝对定位 */
  int (*setVel)(MotorHal *motor, float velMmS);   /* 速度模式 */
  int (*setTorque)(MotorHal *motor, float torque); /* 扭矩模式 */
  int (*home)(MotorHal *motor);                    /* 回零 */
  int (*stop)(MotorHal *motor);                    /* 急停 */
  int (*update)(MotorHal *motor);                  /* 周期性更新 (读反馈) */
  int (*setPid)(MotorHal *motor, float kp, float ki, float kd);

  /* 内部状态 */
  void *priv;   /* 后端私有数据 */
};

/* ==================== 全局 API ==================== */
int plc_motorhal_init(MotorHal *motor, const MotorConfig *cfg);
int plc_motorhal_deinit(MotorHal *motor);

/* 注册后端驱动 */
typedef int (*MotorDriverInitFn)(MotorHal *motor);
int plc_motorhal_registerDriver(MotorDriverType type, MotorDriverInitFn initFn);

/* 便捷宏 */
#define MOTOR_POS_INVALID  (-1e30f)

#ifdef __cplusplus
}
#endif

#endif /* PLC_MOTORHAL_H */
