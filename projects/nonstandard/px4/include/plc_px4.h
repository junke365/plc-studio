#ifndef PLC_PX4_H
#define PLC_PX4_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PX4_MAX_SENSORS      32
#define PX4_MAX_ACTUATORS    16
#define PX4_MAX_MIXER       32
#define PX4_UORB_MSG_MAX    256

/* ==================== uORB 消息类型 ==================== */
typedef enum {
  UORB_SENSOR_ACCEL,
  UORB_SENSOR_GYRO,
  UORB_SENSOR_MAG,
  UORB_SENSOR_BARO,
  UORB_SENSOR_GPS,
  UORB_ACTUATOR_OUTPUT,
  UORB_VEHICLE_ATTITUDE,
  UORB_VEHICLE_LOCAL_POS,
  UORB_VEHICLE_GLOBAL_POS,
  UORB_VEHICLE_RATE_SETPOINT,
  UORB_VEHICLE_ATTITUDE_SETPOINT,
  UORB_VEHICLE_THRUST_SETPOINT,
  UORB_RC_CHANNELS,
  UORB_BATTERY_STATUS,
} Px4UorbTopic;

/* ==================== uORB 消息 ==================== */
typedef struct {
  Px4UorbTopic topic;
  uint64_t timestamp;
  union {
    struct { float x, y, z; } accel;
    struct { float x, y, z; } gyro;
    struct { float roll, pitch, yaw; } attitude;
    struct { float x, y, z; float vx, vy, vz; } localPos;
    struct { double lat, lon; float alt; } globalPos;
    struct { float roll, pitch, yaw, thrust; } attSetpoint;
    struct { float rates[3]; } rateSetpoint;
    struct { float output[PX4_MAX_ACTUATORS]; int count; } actuator;
    struct { float channels[16]; int count; } rc;
    struct { float voltage; float current; float remaining; } battery;
    uint8_t raw[PX4_UORB_MSG_MAX];
  } data;
} Px4UorbMsg;

/* ==================== 混控器 (Mixer) ==================== */
typedef struct {
  float rollScale;
  float pitchScale;
  float yawScale;
  float thrustScale;
  float idleSpeed;
  float reversYaw;
  float actMatrix[PX4_MAX_ACTUATORS][4]; /* [n][roll, pitch, yaw, thrust] */
  int actuatorCount;
} Px4Mixer;

/* ==================== 姿态控制器 ==================== */
typedef struct {
  /* PID 参数 */
  float rollP, rollI, rollD;
  float pitchP, pitchI, pitchD;
  float yawP, yawI, yawD;
  /* 状态 */
  float rollRateInteg, pitchRateInteg, yawRateInteg;
  float rollRate, pitchRate, yawRate;
  float rollAngle, pitchAngle, yawAngle;
  /* 限幅 */
  float rateLimit[3];
  float outputLimit;
  /* 时间 */
  float dt;
} Px4AttitudeCtrl;

/* ==================== PX4 控制器状态 ==================== */
typedef struct {
  Px4AttitudeCtrl attCtrl;
  Px4Mixer mixer;
  float actuators[PX4_MAX_ACTUATORS];
  int actuatorCount;
  bool armed;
  float throttle;
  /* 传感器数据 */
  Px4UorbMsg sensors[PX4_MAX_SENSORS];
  int sensorCount;
  /* 通信 */
  void *uorbSubs[PX4_MAX_SENSORS];  /* 订阅 */
  int subCount;
} Px4Controller;

/* ==================== API ==================== */

/* 初始化 */
void plc_px4_init(Px4Controller *px4, int actuatorCount, float dt);

/* uORB 消息发布/订阅 */
int plc_px4_publish(Px4Controller *px4, Px4UorbMsg *msg);
int plc_px4_subscribe(Px4Controller *px4, Px4UorbTopic topic, void *callback);
int plc_px4_getMsg(Px4Controller *px4, Px4UorbTopic topic, Px4UorbMsg *out);

/* 姿态控制 */
void plc_px4_attCtrlSetpoint(Px4Controller *px4, float roll, float pitch, float yaw, float thrust);
void plc_px4_attCtrlUpdate(Px4Controller *px4);
void plc_px4_attCtrlSetPid(Px4Controller *px4, float roll[3], float pitch[3], float yaw[3]);

/* 混控器 */
void plc_px4_mixerInit(Px4Mixer *mixer, int actuators);
void plc_px4_mixerUpdate(Px4Mixer *mixer, float roll, float pitch, float yaw, float thrust, float *output);

/* 状态管理 */
void plc_px4_arm(Px4Controller *px4, bool arm);
bool plc_px4_isArmed(const Px4Controller *px4);

/* 仿真集成 */
void plc_px4_step(Px4Controller *px4, float dt);

/* ==================== PX4 原生桥接 ==================== */
typedef struct {
  int (*init)(int argc, char *argv[]);
  int (*run)(void);
  int (*stop)(void);
  int (*sendExternal)(const uint8_t *data, int len);
  int (*recvExternal)(uint8_t *data, int maxLen);
} Px4NativeBridge;

int plc_px4_bridgeInit(Px4NativeBridge *bridge, const char *px4Path);

#ifdef __cplusplus
}
#endif

#endif /* PLC_PX4_H */
