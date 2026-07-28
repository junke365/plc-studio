#ifndef PLC_MOTION_H
#define PLC_MOTION_H

/* ==================== CNC 运动控制系统主头文件 ====================
 *
 * 包含所有运动控制子系统:
 *   - Motor HAL     (plc_motorhal.h)  电机硬件抽象层
 *   - Axis          (plc_axis.h)     轴控制
 *   - Group         (plc_group.h)    轴组/坐标系
 *   - Kinematics    (plc_kinematics.h) 运动学正逆解
 *   - GCode Parser  (plc_gcodeparser.h) RS-274 G代码解释器
 *   - Planner       (plc_planner.h)  轨迹规划器 (前瞻/S曲线)
 *   - Interpolator  (plc_interpolator.h) 实时插补器
 */

#include "plc_motorhal.h"
#include "plc_axis.h"
#include "plc_group.h"
#include "plc_kinematics.h"
#include "plc_gcodeparser.h"
#include "plc_planner.h"
#include "plc_interpolator.h"

/* ==================== CNC 运行模式 ==================== */
typedef enum {
  CNC_MODE_IDLE = 0,
  CNC_MODE_MANUAL,            /* 手动/点动 */
  CNC_MODE_MDI,               /* MDI (Manual Data Input) */
  CNC_MODE_AUTO,              /* 自动运行 */
  CNC_MODE_SINGLE_BLOCK,      /* 单步运行 */
  CNC_MODE_HOMING,            /* 回零 */
} CncMode;

/* ==================== CNC 状态 ==================== */
typedef enum {
  CNC_STATUS_NONE = 0,
  CNC_STATUS_IDLE,
  CNC_STATUS_RUNNING,
  CNC_STATUS_PAUSED,
  CNC_STATUS_HOLD,             /* 进给保持 */
  CNC_STATUS_STOP,
  CNC_STATUS_FAULT,
  CNC_STATUS_ESTOP,            /* 急停 */
} CncStatus;

/* ==================== CNC 系统配置 ==================== */
typedef struct {
  uint32_t axisCount;              /* 轴数 */
  uint32_t groupCount;             /* 轴组数 */
  float servoCycleSec;             /* 伺服周期 (秒) */
  float maxFeedRate;               /* 最大进给率 (mm/min) */
  float rapidRate;                 /* 快移速度 (mm/min) */
  float defaultAccel;              /* 默认加速度 (mm/s²) */
  float defaultJerk;               /* 默认加加速度 (mm/s³) */
  float junctionDeviation;         /* 拐角偏差 (mm, G64) */
} CncConfig;

/* ==================== CNC 系统 ==================== */
typedef struct {
  /* 配置 */
  CncConfig config;

  /* 轴 */
  PlcAxis axes[9];                 /* 最大 9 轴 */
  uint32_t axisCount;

  /* 轴组 */
  PlcGroup groups[4];              /* 最大 4 组 */
  uint32_t groupCount;
  int activeGroup;                 /* 当前激活组索引 */

  /* 运动学 */
  Kinematics kins;

  /* G-Code 解析器 */
  GCodeParserState gcodeState;

  /* 规划器 */
  Planner planner;

  /* 插补器 */
  Interpolator interpolator;

  /* 模式/状态 */
  CncMode mode;
  CncStatus status;

  /* G-Code 文件运行 */
  const char **gcodeLines;         /* 内存中 G-code 行 */
  uint32_t gcodeLineCount;
  uint32_t currentLine;

  /* MDI 缓冲区 */
  char mdiBuffer[256];

  /* 进给倍率 */
  float feedOverride;              /* 0.0 - 2.0 */
  float rapidOverride;             /* 0.0 - 1.0 */
  float spindleOverride;           /* 0.0 - 2.0 */

  /* 急停 */
  bool estop;

  /* 用户数据 */
  void *userData;
} CncSystem;

/* ==================== CNC API ==================== */

/* 初始化 */
int plc_cnc_init(CncSystem *cnc, const CncConfig *cfg);
int plc_cnc_deinit(CncSystem *cnc);
int plc_cnc_addAxis(CncSystem *cnc, uint8_t id, const char *name, const MotorConfig *motorCfg);

/* 模式控制 */
int plc_cnc_setMode(CncSystem *cnc, CncMode mode);
CncMode plc_cnc_getMode(const CncSystem *cnc);

/* 状态控制 */
int plc_cnc_start(CncSystem *cnc);
int plc_cnc_stop(CncSystem *cnc);
int plc_cnc_pause(CncSystem *cnc);
int plc_cnc_resume(CncSystem *cnc);
int plc_cnc_estop(CncSystem *cnc);
int plc_cnc_clearEstop(CncSystem *cnc);

/* 进给控制 */
void plc_cnc_setFeedOverride(CncSystem *cnc, float override);
void plc_cnc_setRapidOverride(CncSystem *cnc, float override);
void plc_cnc_setSpindleOverride(CncSystem *cnc, float override);

/* G-Code 运行 */
int plc_cnc_loadGCode(CncSystem *cnc, const char **lines, uint32_t count);
int plc_cnc_loadGCodeFile(CncSystem *cnc, const char *filename);
int plc_cnc_mdi(CncSystem *cnc, const char *command);

/* 手动/点动 */
int plc_cnc_jog(CncSystem *cnc, uint8_t axisIdx, float velocity);
int plc_cnc_jogStop(CncSystem *cnc, uint8_t axisIdx);

/* 回零 */
int plc_cnc_homeAll(CncSystem *cnc);

/* 主轴 */
int plc_cnc_spindleOn(CncSystem *cnc, int direction, float speed);
int plc_cnc_spindleOff(CncSystem *cnc);

/* 冷却 */
int plc_cnc_coolantMistOn(CncSystem *cnc);
int plc_cnc_coolantMistOff(CncSystem *cnc);
int plc_cnc_coolantFloodOn(CncSystem *cnc);
int plc_cnc_coolantFloodOff(CncSystem *cnc);

/* 周期性更新 (伺服循环中调用) */
int plc_cnc_update(CncSystem *cnc, float dtSec);

/* 状态查询 */
CncStatus plc_cnc_getStatus(const CncSystem *cnc);
const char *plc_cnc_statusStr(CncStatus status);
const char *plc_cnc_modeStr(CncMode mode);

/* 错误 */
int plc_cnc_lastError(CncSystem *cnc);
const char *plc_cnc_errorStr(int err);

/* ==================== 错误码 ==================== */
#define CNC_OK                  0
#define CNC_ERR_GENERAL         -1
#define CNC_ERR_INVALID_PARAM   -2
#define CNC_ERR_NOT_INIT        -3
#define CNC_ERR_BUSY            -4
#define CNC_ERR_TIMEOUT         -5
#define CNC_ERR_LIMIT           -6
#define CNC_ERR_FOLLOWING       -7
#define CNC_ERR_HOMING          -8
#define CNC_ERR_GCODE           -9
#define CNC_ERR_PLANNER_FULL    -10
#define CNC_ERR_ESTOP           -11
#define CNC_ERR_NO_AXIS         -12
#define CNC_ERR_NO_FILE         -13

#endif /* PLC_MOTION_H */
