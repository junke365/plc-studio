#ifndef PLC_GROUP_H
#define PLC_GROUP_H

#include "plc_axis.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 轴组配置 ==================== */
#define PLC_GROUP_MAX_AXIS   9   /* X, Y, Z, A, B, C, U, V, W */
#define PLC_GROUP_MAX_GROUPS 4

/* 轴 ID 常量 */
#define AXIS_X  0
#define AXIS_Y  1
#define AXIS_Z  2
#define AXIS_A  3
#define AXIS_B  4
#define AXIS_C  5
#define AXIS_U  6
#define AXIS_V  7
#define AXIS_W  8

/* ==================== 坐标系类型 ==================== */
typedef enum {
  COORD_MACHINE = 0,    /* 机床坐标系 */
  COORD_G54,            /* 工件坐标系 1 */
  COORD_G55,            /* 工件坐标系 2 */
  COORD_G56,            /* 工件坐标系 3 */
  COORD_G57,            /* 工件坐标系 4 */
  COORD_G58,            /* 工件坐标系 5 */
  COORD_G59,            /* 工件坐标系 6 */
  COORD_G59_1,          /* 工件坐标系 7 */
  COORD_G59_2,          /* 工件坐标系 8 */
  COORD_G59_3,          /* 工件坐标系 9 */
} CoordSystem;

#define COORD_WORK_FIRST  COORD_G54
#define COORD_WORK_LAST   COORD_G59_3

/* ==================== 轴映射 ==================== */
typedef struct {
  uint8_t logicalAxis;     /* 逻辑轴号 (0-8) */
  int8_t physicalAxis;     /* 物理轴号 (-1 表示未映射) */
  float scale;             /* 比例因子 */
  float offset;            /* 偏置 */
  bool inverted;           /* 反向 */
} AxisMapping;

/* ==================== 轴组数据结构 ==================== */
typedef struct {
  uint8_t id;                  /* 组 ID */
  char name[16];               /* 组名 */
  uint8_t axisCount;           /* 轴数 (2-9) */

  /* 轴引用 */
  PlcAxis *axes[PLC_GROUP_MAX_AXIS];
  AxisMapping mapping[PLC_GROUP_MAX_AXIS];

  /* 当前坐标系 */
  CoordSystem activeCoord;
  float coordOffsets[PLC_GROUP_MAX_AXIS][9]; /* [axis][coord] 偏移值 */

  /* G92 偏移 (每轴) */
  float g92Offset[PLC_GROUP_MAX_AXIS];

  /* 刀具偏移 */
  float toolOffset[PLC_GROUP_MAX_AXIS];

  /* 当前位置 (工作坐标系) */
  float commandPos[PLC_GROUP_MAX_AXIS];
  float actualPos[PLC_GROUP_MAX_AXIS];

  /* 状态 */
  bool enabled;
  bool allHomed;               /* 所有轴已回零 */
  bool inPosition;
} PlcGroup;

/* ==================== 轴组 API ==================== */
int plc_group_init(PlcGroup *group, uint8_t id, const char *name);
int plc_group_deinit(PlcGroup *group);

/* 轴管理 */
int plc_group_addAxis(PlcGroup *group, PlcAxis *axis, uint8_t logicalIdx, float scale, float offset);
int plc_group_removeAxis(PlcGroup *group, uint8_t logicalIdx);

/* 坐标系管理 */
void plc_group_selectCoord(PlcGroup *group, CoordSystem coord);
CoordSystem plc_group_getCoord(const PlcGroup *group);
void plc_group_setCoordOffset(PlcGroup *group, CoordSystem coord, uint8_t axisIdx, float offset);
float plc_group_getCoordOffset(const PlcGroup *group, CoordSystem coord, uint8_t axisIdx);

/* G92 */
void plc_group_setG92(PlcGroup *group, const float *offsets);
void plc_group_clearG92(PlcGroup *group);

/* 刀具偏移 */
void plc_group_setToolOffset(PlcGroup *group, const float *offsets);
void plc_group_clearToolOffset(PlcGroup *group);

/* 坐标转换 */
void plc_group_machineToWork(PlcGroup *group, const float *machine, float *work);
void plc_group_workToMachine(PlcGroup *group, const float *work, float *machine);

/* 运动指令 (工作坐标系) */
int plc_group_moveAbs(PlcGroup *group, const float *workPos, float feedRate, uint8_t axes);
int plc_group_moveRel(PlcGroup *group, const float *delta, float feedRate, uint8_t axes);
int plc_group_moveSync(PlcGroup *group);  /* 同步发送所有轴指令 */
int plc_group_stop(PlcGroup *group);
int plc_group_abort(PlcGroup *group);

/* 回零 (所有轴) */
int plc_group_homeAll(PlcGroup *group);
int plc_group_homeAxis(PlcGroup *group, uint8_t logicalIdx);

/* 周期性更新 */
int plc_group_update(PlcGroup *group, float dtSec);

/* 状态查询 */
bool plc_group_allHomed(PlcGroup *group);
bool plc_group_inPosition(PlcGroup *group);
bool plc_group_isEnabled(PlcGroup *group);

/* 工具函数 */
int plc_group_findGroupByAxis(const PlcGroup *groups, uint8_t groupCount, uint8_t axisId);
const char *plc_group_coordName(CoordSystem coord);

#ifdef __cplusplus
}
#endif

#endif /* PLC_GROUP_H */
