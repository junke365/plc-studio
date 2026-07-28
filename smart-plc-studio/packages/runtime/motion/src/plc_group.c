#include "plc_group.h"
#include <string.h>
#include <math.h>

int plc_group_init(PlcGroup *group, uint8_t id, const char *name)
{
  if (!group || !name) return -1;

  memset(group, 0, sizeof(PlcGroup));
  group->id = id;
  strncpy(group->name, name, sizeof(group->name) - 1);
  group->axisCount = 0;
  group->activeCoord = COORD_G54;
  group->enabled = true;

  /* 初始化轴映射为未映射 */
  for (int i = 0; i < PLC_GROUP_MAX_AXIS; i++) {
    group->mapping[i].physicalAxis = -1;
    group->mapping[i].scale = 1.0f;
    group->mapping[i].inverted = false;
  }

  return 0;
}

int plc_group_deinit(PlcGroup *group)
{
  if (!group) return -1;
  memset(group, 0, sizeof(PlcGroup));
  return 0;
}

int plc_group_addAxis(PlcGroup *group, PlcAxis *axis, uint8_t logicalIdx, float scale, float offset)
{
  if (!group || !axis) return -1;
  if (logicalIdx >= PLC_GROUP_MAX_AXIS) return -1;

  if (group->axes[logicalIdx] != NULL) {
    /* 轴已存在，先移除 */
    plc_group_removeAxis(group, logicalIdx);
  }

  group->axes[logicalIdx] = axis;
  group->mapping[logicalIdx].logicalAxis = logicalIdx;
  group->mapping[logicalIdx].physicalAxis = axis->id;
  group->mapping[logicalIdx].scale = scale;
  group->mapping[logicalIdx].offset = offset;

  if (logicalIdx >= group->axisCount) {
    group->axisCount = logicalIdx + 1;
  }

  return 0;
}

int plc_group_removeAxis(PlcGroup *group, uint8_t logicalIdx)
{
  if (!group) return -1;
  if (logicalIdx >= PLC_GROUP_MAX_AXIS) return -1;

  group->axes[logicalIdx] = NULL;
  group->mapping[logicalIdx].physicalAxis = -1;

  /* 收缩 axisCount */
  while (group->axisCount > 0 && group->axes[group->axisCount - 1] == NULL) {
    group->axisCount--;
  }

  return 0;
}

void plc_group_selectCoord(PlcGroup *group, CoordSystem coord)
{
  if (group) group->activeCoord = coord;
}

CoordSystem plc_group_getCoord(const PlcGroup *group)
{
  return group ? group->activeCoord : COORD_MACHINE;
}

void plc_group_setCoordOffset(PlcGroup *group, CoordSystem coord, uint8_t axisIdx, float offset)
{
  if (!group || axisIdx >= PLC_GROUP_MAX_AXIS) return;
  if (coord < COORD_G54 || coord > COORD_G59_3) return;
  group->coordOffsets[axisIdx][coord - COORD_G54] = offset;
}

float plc_group_getCoordOffset(const PlcGroup *group, CoordSystem coord, uint8_t axisIdx)
{
  if (!group || axisIdx >= PLC_GROUP_MAX_AXIS) return 0;
  if (coord < COORD_G54 || coord > COORD_G59_3) return 0;
  return group->coordOffsets[axisIdx][coord - COORD_G54];
}

void plc_group_setG92(PlcGroup *group, const float *offsets)
{
  if (!group || !offsets) return;
  for (int i = 0; i < PLC_GROUP_MAX_AXIS; i++) {
    group->g92Offset[i] = offsets[i];
  }
}

void plc_group_clearG92(PlcGroup *group)
{
  if (group) {
    memset(group->g92Offset, 0, sizeof(group->g92Offset));
  }
}

void plc_group_setToolOffset(PlcGroup *group, const float *offsets)
{
  if (!group || !offsets) return;
  for (int i = 0; i < PLC_GROUP_MAX_AXIS; i++) {
    group->toolOffset[i] = offsets[i];
  }
}

void plc_group_clearToolOffset(PlcGroup *group)
{
  if (group) {
    memset(group->toolOffset, 0, sizeof(group->toolOffset));
  }
}

void plc_group_machineToWork(PlcGroup *group, const float *machine, float *work)
{
  if (!group || !machine || !work) return;

  int coordIdx = group->activeCoord - COORD_G54;
  if (coordIdx < 0) coordIdx = 0;

  for (int i = 0; i < PLC_GROUP_MAX_AXIS; i++) {
    /* 工作位置 = 机床位置 - 坐标系偏移 - G92偏移 - 刀具偏移 */
    float offset = group->coordOffsets[i][coordIdx]
                 + group->g92Offset[i]
                 + group->toolOffset[i];
    work[i] = machine[i] - offset;
  }
}

void plc_group_workToMachine(PlcGroup *group, const float *work, float *machine)
{
  if (!group || !work || !machine) return;

  int coordIdx = group->activeCoord - COORD_G54;
  if (coordIdx < 0) coordIdx = 0;

  for (int i = 0; i < PLC_GROUP_MAX_AXIS; i++) {
    float offset = group->coordOffsets[i][coordIdx]
                 + group->g92Offset[i]
                 + group->toolOffset[i];
    machine[i] = work[i] + offset;
  }
}

int plc_group_moveAbs(PlcGroup *group, const float *workPos, float feedRate, uint8_t axes)
{
  if (!group || !workPos) return -1;

  float machinePos[PLC_GROUP_MAX_AXIS];
  plc_group_workToMachine(group, workPos, machinePos);

  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i] && ((axes >> i) & 1)) {
      int ret = plc_axis_moveAbs(group->axes[i], machinePos[i], feedRate / 60.0f, 0);
      if (ret < 0) return ret;
    }
  }

  return 0;
}

int plc_group_moveRel(PlcGroup *group, const float *delta, float feedRate, uint8_t axes)
{
  if (!group || !delta) return -1;

  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i] && ((axes >> i) & 1)) {
      float target = group->axes[i]->commandPosition + delta[i];
      int ret = plc_axis_moveAbs(group->axes[i], target, feedRate / 60.0f, 0);
      if (ret < 0) return ret;
    }
  }

  return 0;
}

int plc_group_moveSync(PlcGroup *group)
{
  (void)group;
  return 0;
}

int plc_group_stop(PlcGroup *group)
{
  if (!group) return -1;
  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i]) plc_axis_stop(group->axes[i]);
  }
  return 0;
}

int plc_group_abort(PlcGroup *group)
{
  if (!group) return -1;
  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i]) plc_axis_abort(group->axes[i]);
  }
  return 0;
}

int plc_group_homeAll(PlcGroup *group)
{
  if (!group) return -1;
  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i]) plc_axis_home(group->axes[i], HOMING_MODE_LIMIT);
  }
  return 0;
}

int plc_group_homeAxis(PlcGroup *group, uint8_t logicalIdx)
{
  if (!group || logicalIdx >= group->axisCount) return -1;
  if (!group->axes[logicalIdx]) return -1;
  return plc_axis_home(group->axes[logicalIdx], HOMING_MODE_LIMIT);
}

int plc_group_update(PlcGroup *group, float dtSec)
{
  if (!group) return -1;

  group->allHomed = true;
  group->inPosition = true;

  for (int i = 0; i < group->axisCount; i++) {
    if (group->axes[i]) {
      plc_axis_update(group->axes[i], dtSec);

      /* 更新组位置 (工作坐标系) */
      float machinePos = group->axes[i]->actualPosition;
      float workPos = machinePos;
      int coordIdx = group->activeCoord - COORD_G54;
      if (coordIdx >= 0) {
        workPos -= group->coordOffsets[i][coordIdx]
                 + group->g92Offset[i]
                 + group->toolOffset[i];
      }
      group->actualPos[i] = workPos;

      if (!group->axes[i]->homed) group->allHomed = false;
      if (!group->axes[i]->inPosition) group->inPosition = false;
    }
  }

  return 0;
}

bool plc_group_allHomed(PlcGroup *group)
{
  return group ? group->allHomed : false;
}

bool plc_group_inPosition(PlcGroup *group)
{
  return group ? group->inPosition : false;
}

bool plc_group_isEnabled(PlcGroup *group)
{
  return group ? group->enabled : false;
}

int plc_group_findGroupByAxis(const PlcGroup *groups, uint8_t groupCount, uint8_t axisId)
{
  if (!groups) return -1;
  for (int g = 0; g < groupCount; g++) {
    for (int a = 0; a < groups[g].axisCount; a++) {
      if (groups[g].axes[a] && groups[g].axes[a]->id == axisId) {
        return g;
      }
    }
  }
  return -1;
}

const char *plc_group_coordName(CoordSystem coord)
{
  static const char *names[] = {
    "MACHINE", "G54", "G55", "G56", "G57", "G58", "G59",
    "G59.1", "G59.2", "G59.3"
  };
  if (coord < 0 || coord >= (int)(sizeof(names)/sizeof(names[0]))) return "?";
  return names[coord];
}
