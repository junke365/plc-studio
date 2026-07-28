#include "plc_motorhal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* 驱动注册表 */
#define MAX_DRIVERS 8

static MotorDriverInitFn s_driverRegistry[MAX_DRIVERS];
static int s_driverCount = 0;

int plc_motorhal_registerDriver(MotorDriverType type, MotorDriverInitFn initFn)
{
  if (type < 0 || type >= MAX_DRIVERS) return -1;
  s_driverRegistry[type] = initFn;
  if (type >= (MotorDriverType)s_driverCount) s_driverCount = (int)type + 1;
  return 0;
}

int plc_motorhal_init(MotorHal *motor, const MotorConfig *cfg)
{
  if (!motor || !cfg) return -1;

  memset(motor, 0, sizeof(MotorHal));
  motor->config = *cfg;
  motor->status = MOTOR_STATUS_DISABLED;
  motor->commandPos = MOTOR_POS_INVALID;

  /* 查找并调用驱动初始化函数 */
  MotorDriverInitFn initFn = NULL;
  if (cfg->drvType >= 0 && cfg->drvType < MAX_DRIVERS) {
    initFn = s_driverRegistry[cfg->drvType];
  }

  if (initFn) {
    return initFn(motor);
  }

  /* 没有注册的驱动，使用简单的默认行为 */
  return 0;
}

int plc_motorhal_deinit(MotorHal *motor)
{
  if (!motor) return -1;
  if (motor->deinit) return motor->deinit(motor);
  if (motor->priv) free(motor->priv);
  memset(motor, 0, sizeof(MotorHal));
  return 0;
}

/* ==================== EtherCAT CiA402 驱动 ==================== */

typedef struct {
  uint16_t slaveId;
  uint16_t vendorId;
  uint32_t profileNo;
  float positionMm;
  float velocityMmS;
  /* EtherCAT 通信句柄 (由上层协议栈提供) */
  void *ecHandle;
} EthercatPriv;

/* 前向声明 */
static int ethercat_enable(MotorHal *motor, bool on);
static int ethercat_stop(MotorHal *motor);
static int ethercat_setPos(MotorHal *motor, float posMm);
static int ethercat_setVel(MotorHal *motor, float velMmS);

static int ethercat_init(MotorHal *motor)
{
  EthercatPriv *priv = (EthercatPriv *)calloc(1, sizeof(EthercatPriv));
  if (!priv) return -1;

  priv->slaveId = motor->config.drv.ethercat.slaveId;
  priv->vendorId = motor->config.drv.ethercat.vendorId;
  priv->profileNo = motor->config.drv.ethercat.profileNo;
  priv->positionMm = 0;
  priv->velocityMmS = 0;

  motor->priv = priv;
  motor->enable = ethercat_enable;
  motor->setPos = ethercat_setPos;
  motor->setVel = ethercat_setVel;
  motor->stop   = ethercat_stop;
  motor->status = MOTOR_STATUS_ENABLED;
  return 0;
}

static int ethercat_enable(MotorHal *motor, bool on)
{
  motor->status = on ? MOTOR_STATUS_ENABLED : MOTOR_STATUS_DISABLED;
  (void)motor;
  return 0;
}

static int ethercat_stop(MotorHal *motor)
{
  motor->commandVel = 0;
  return 0;
}

static int ethercat_setPos(MotorHal *motor, float posMm)
{
  motor->commandPos = posMm;
  /* 通过 PDO 写入目标位置到 EtherCAT 驱动器 */
  return 0;
}

static int ethercat_setVel(MotorHal *motor, float velMmS)
{
  motor->commandVel = velMmS;
  return 0;
}

/* ==================== CANopen DS402 驱动 ==================== */

typedef struct {
  uint8_t nodeId;
  uint16_t cobIdBase;
  float positionMm;
  float velocityMmS;
  int16_t targetTorque;
} CanopenPriv;

/* 前向声明 */
static int canopen_setPos(MotorHal *motor, float posMm);
static int canopen_setVel(MotorHal *motor, float velMmS);

static int canopen_enable(MotorHal *motor, bool on)
{
  motor->status = on ? MOTOR_STATUS_ENABLED : MOTOR_STATUS_DISABLED;
  (void)motor;
  return 0;
}

static int canopen_stop(MotorHal *motor)
{
  motor->commandVel = 0;
  return 0;
}

static int canopen_init(MotorHal *motor)
{
  CanopenPriv *priv = (CanopenPriv *)calloc(1, sizeof(CanopenPriv));
  if (!priv) return -1;

  priv->nodeId = motor->config.drv.canopen.nodeId;
  motor->priv = priv;
  motor->enable = canopen_enable;
  motor->setPos = canopen_setPos;
  motor->setVel = canopen_setVel;
  motor->stop   = canopen_stop;
  motor->status = MOTOR_STATUS_ENABLED;
  return 0;
}

static int canopen_setPos(MotorHal *motor, float posMm)
{
  motor->commandPos = posMm;
  /* SDO 写目标位置对象 (0x607A) */
  return 0;
}

static int canopen_setVel(MotorHal *motor, float velMmS)
{
  motor->commandVel = velMmS;
  return 0;
}

/* ==================== Modbus 驱动 ==================== */

typedef struct {
  uint8_t slaveAddr;
  uint16_t regControl;
  uint16_t regStatus;
  uint16_t regSpeed;
  uint16_t regPos;
  float positionMm;
} ModbusPriv;

/* 前向声明 */
static int modbus_setPos(MotorHal *motor, float posMm);
static int modbus_setVel(MotorHal *motor, float velMmS);

static int modbus_enable(MotorHal *motor, bool on)
{
  motor->status = on ? MOTOR_STATUS_ENABLED : MOTOR_STATUS_DISABLED;
  (void)motor;
  return 0;
}

static int modbus_stop(MotorHal *motor)
{
  motor->commandVel = 0;
  return 0;
}

static int modbus_init(MotorHal *motor)
{
  ModbusPriv *priv = (ModbusPriv *)calloc(1, sizeof(ModbusPriv));
  if (!priv) return -1;

  priv->slaveAddr = motor->config.drv.modbus.slaveAddr;
  priv->regControl = motor->config.drv.modbus.regControl;
  priv->regStatus = motor->config.drv.modbus.regStatus;
  priv->regSpeed = motor->config.drv.modbus.regSpeed;
  priv->regPos = motor->config.drv.modbus.regPos;

  motor->priv = priv;
  motor->enable = modbus_enable;
  motor->setPos = modbus_setPos;
  motor->setVel = modbus_setVel;
  motor->stop   = modbus_stop;
  motor->status = MOTOR_STATUS_ENABLED;
  return 0;
}

static int modbus_setPos(MotorHal *motor, float posMm)
{
  motor->commandPos = posMm;
  /* Modbus 写保持寄存器到变频器/伺服 */
  return 0;
}

static int modbus_setVel(MotorHal *motor, float velMmS)
{
  motor->commandVel = velMmS;
  return 0;
}

/* ==================== 驱动注册 (每个驱动通过独立的 constructor 注册) ==================== */

/* EtherCAT CiA402 存根 */
__attribute__((constructor))
static void register_ethercat_stub(void)
{
  plc_motorhal_registerDriver(MOTOR_DRV_ETHERCAT, ethercat_init);
}

/* CANopen DS402 存根 */
__attribute__((constructor))
static void register_canopen_stub(void)
{
  static int reg = 0;
  if (reg) return;
  reg = 1;
  plc_motorhal_registerDriver(MOTOR_DRV_CANOPEN, canopen_init);
}

/* Modbus 存根 */
__attribute__((constructor))
static void register_modbus_stub(void)
{
  static int reg = 0;
  if (reg) return;
  reg = 1;
  plc_motorhal_registerDriver(MOTOR_DRV_MODBUS, modbus_init);
}
