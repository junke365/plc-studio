#include "plc_gcodeparser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

/* ==================== 内部工具 ==================== */

static int char_to_axis(char c)
{
  switch (toupper(c)) {
  case 'X': return 0;
  case 'Y': return 1;
  case 'Z': return 2;
  case 'A': return 3;
  case 'B': return 4;
  case 'C': return 5;
  case 'U': return 6;
  case 'V': return 7;
  case 'W': return 8;
  default: return -1;
  }
}

static int gcode_to_group(int g)
{
  /* G 代码模态组映射 */
  if (g == 4) return GC_SPINDLE;       /* G96/G97 */

  switch (g / 10) {
  case 0:
    if (g >= 0 && g <= 3)   return GC_MOTION;
    if (g == 4)             return -1;  /* G4 不模态 */
    if (g == 10 || g == 28) return -1;
    if (g >= 17 && g <= 19) return GC_PLANE;
    if (g == 20 || g == 21) return GC_UNITS;
    if (g >= 40 && g <= 42) return GC_CUTTERCOMP;
    if (g == 43 || g == 49) return GC_TOOLOFFSET;
    if (g >= 53 && g <= 59) return GC_COORDSYS;
    if (g == 61 || g == 64) return GC_PATHCTRL;
    if (g == 90 || g == 91) return GC_DISTANCE;
    if (g >= 92 && g <= 99) return -1;
    return -1;
  default:
    return GC_MOTION;  /* G>=100 assumed motion */
  }
}

/* ==================== 初始化和重置 ==================== */

void plc_gcode_init(GCodeParserState *state)
{
  if (!state) return;
  memset(state, 0, sizeof(GCodeParserState));

  /* 默认模态值 */
  state->isMetric = true;            /* G21 公制 */
  state->isAbsolute = true;          /* G90 绝对坐标 */
  state->activePlane = 17;           /* G17 XY 平面 */
  state->feedRate = 300.0f;          /* 默认进给 300 mm/min */
  state->activeCoord = 1;            /* G54 */
  state->pathControl = 64;           /* G64 连续 */

  /* 初始化模态 G 代码 */
  state->gModes[GC_MOTION]     = 0;  /* G0 */
  state->gModes[GC_PLANE]      = 17;
  state->gModes[GC_DISTANCE]   = 90;
  state->gModes[GC_UNITS]      = 21;
  state->gModes[GC_COORDSYS]   = 54;
  state->gModes[GC_PATHCTRL]   = 64;
  state->gModes[GC_CUTTERCOMP] = 40;
  state->gModes[GC_TOOLOFFSET] = 49;
}

void plc_gcode_reset(GCodeParserState *state)
{
  plc_gcode_init(state);
}

/* ==================== 行解析 ==================== */

static char *trim_line(char *line)
{
  while (*line && isspace((unsigned char)*line)) line++;
  char *end = line + strlen(line) - 1;
  while (end > line && isspace((unsigned char)*end)) end--;
  *(end + 1) = '\0';

  /* 去掉注释 (从 ; 开始) */
  char *p = line;
  while (*p) {
    if (*p == ';') { *p = '\0'; break; }
    p++;
  }

  return line;
}

int plc_gcode_parseLine(GCodeParserState *state, const char *line, GCodeBlock *block)
{
  if (!state || !line || !block) return -1;

  memset(block, 0, sizeof(GCodeBlock));

  char buf[512];
  strncpy(buf, line, sizeof(buf) - 1);
  char *p = trim_line(buf);

  if (*p == '\0') return 0;

  /* 检查括号注释 */
  char *commentStart = strchr(p, '(');
  if (commentStart) {
    char *commentEnd = strchr(commentStart, ')');
    if (commentEnd) {
      size_t len = commentEnd - commentStart - 1;
      if (len > 0 && len < sizeof(block->comment)) {
        strncpy(block->comment, commentStart + 1, len);
        block->comment[len] = '\0';
      }
      /* 移除注释 */
      memmove(commentStart, commentEnd + 1, strlen(commentEnd + 1) + 1);
    }
  }

  /* 解析字地址 */
  while (*p) {
    if (isspace((unsigned char)*p)) { p++; continue; }

    char addr = toupper((unsigned char)*p);
    p++;

    /* 解析数字值 */
    char numBuf[64];
    char *np = numBuf;
    /* 支持 +, -, ., 数字 */
    if (*p == '+' || *p == '-') *np++ = *p++;
    while (isdigit((unsigned char)*p) || *p == '.') *np++ = *p++;
    *np = '\0';

    float fval = 0;
    int ival = 0;
    if (strlen(numBuf) > 0) {
      fval = (float)atof(numBuf);
      ival = atoi(numBuf);
    }

    switch (addr) {
    case 'X': block->axisPresent[0] = true; block->axisValue[0] = fval; break;
    case 'Y': block->axisPresent[1] = true; block->axisValue[1] = fval; break;
    case 'Z': block->axisPresent[2] = true; block->axisValue[2] = fval; break;
    case 'A': block->axisPresent[3] = true; block->axisValue[3] = fval; break;
    case 'B': block->axisPresent[4] = true; block->axisValue[4] = fval; break;
    case 'C': block->axisPresent[5] = true; block->axisValue[5] = fval; break;
    case 'U': block->axisPresent[6] = true; block->axisValue[6] = fval; break;
    case 'V': block->axisPresent[7] = true; block->axisValue[7] = fval; break;
    case 'W': block->axisPresent[8] = true; block->axisValue[8] = fval; break;
    case 'G':
      block->gPresent = true;
      block->gCode = ival;
      break;
    case 'M':
      block->mPresent = true;
      block->mCode = ival;
      break;
    case 'F': block->fPresent = true; block->fNumber = fval; break;
    case 'S': block->sPresent = true; block->sNumber = fval; break;
    case 'T': block->tPresent = true; block->tNumber = ival; break;
    case 'D': block->dPresent = true; block->dNumber = ival; break;
    case 'H': block->hPresent = true; block->hNumber = ival; break;
    case 'P': block->pPresent = true; block->pNumber = fval; break;
    case 'Q': block->qPresent = true; block->qNumber = fval; break;
    case 'R': block->rPresent = true; block->rNumber = fval; break;
    case 'L': block->lPresent = true; block->lNumber = ival; break;
    case 'I': block->iPresent = true; block->iNumber = fval; break;
    case 'J': block->jPresent = true; block->jNumber = fval; break;
    case 'K': block->kPresent = true; block->kNumber = fval; break;
    case 'N': block->lineNumber = ival; break;
    case 'O': block->oWord = true; strncpy(block->oName, numBuf, sizeof(block->oName) - 1); break;
    default:
      break;
    }
  }

  /* 应用当前模态到块 */
  memcpy(block->gModes, state->gModes, sizeof(state->gModes));

  /* 确定运动类型 */
  if (block->gPresent) {
    int group = gcode_to_group(block->gCode);
    if (group >= 0 && group < GC_GROUP_COUNT) {
      block->gModes[group] = block->gCode;
    }

    /* 检查运动类型 */
    switch (block->gCode) {
    case 0:  block->motionToBe = 0;  block->isMotion = true; break;
    case 1:  block->motionToBe = 1;  block->isMotion = true; break;
    case 2:
    case 3:  block->motionToBe = block->gCode; block->isMotion = true; break;
    case 38: block->motionToBe = 38; block->isMotion = true; break;
    case 80: block->motionToBe = 80; break;  /* 取消固定循环 */
    }
  }

  /* 检查模态运动 */
  if (!block->gPresent) {
    int motionG = state->gModes[GC_MOTION];
    if (motionG >= 0) {
      block->motionToBe = motionG;
      if (motionG != 80) block->isMotion = true;
    }
  }

  /* 检查是否有轴字(确认运动) */
  bool hasAxis = false;
  for (int i = 0; i < MAX_AXIS_WORDS; i++) {
    if (block->axisPresent[i]) { hasAxis = true; break; }
  }
  if (!hasAxis && block->motionToBe >= 0) {
    block->isMotion = false;  /* 没有轴字的运动模态不变 */
  }

  block->changed = (block->gPresent || block->mPresent || hasAxis ||
                    block->fPresent || block->sPresent || block->tPresent);

  /* 将块的模态值同步回状态（模态保持） */
  memcpy(state->gModes, block->gModes, sizeof(state->gModes));
  return 0;
}

/* ==================== 执行 (产生规范命令) ==================== */

static void emit_canon(CanonCommandData *cmd, const GCodeCallbacks *cbs, void *user)
{
  if (cbs && cbs->onCanonCommand) {
    cbs->onCanonCommand(cmd, user);
  }
}

int plc_gcode_executeLine(GCodeParserState *state, const char *line,
                          const GCodeCallbacks *callbacks, void *userData)
{
  if (!state || !line) return -1;

  GCodeBlock block;
  int ret = plc_gcode_parseLine(state, line, &block);
  if (ret < 0) return ret;
  if (!block.changed) return 0;

  CanonCommandData cmd;
  memset(&cmd, 0, sizeof(cmd));

  /* 处理 G 代码 */
  if (block.gPresent) {
    bool handled = false;

    switch (block.gCode) {
    /* 快速定位 */
    case 0: {
      cmd.type = CANON_STRAIGHT_TRAVERSE;
      for (int i = 0; i < MAX_AXIS_WORDS; i++) {
        if (block.axisPresent[i]) {
          cmd.axisMask |= (1 << i);
          cmd.end[i] = block.axisValue[i];
        }
      }
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    /* 直线插补 */
    case 1: {
      cmd.type = CANON_STRAIGHT_FEED;
      for (int i = 0; i < MAX_AXIS_WORDS; i++) {
        if (block.axisPresent[i]) {
          cmd.axisMask |= (1 << i);
          cmd.end[i] = block.axisValue[i];
        }
      }
      if (block.fPresent) cmd.feedRate = block.fNumber;
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    /* 圆弧插补 */
    case 2:
    case 3: {
      cmd.type = CANON_ARC_FEED;
      cmd.arcDir = (block.gCode == 2) ? -1 : 1;
      for (int i = 0; i < MAX_AXIS_WORDS; i++) {
        if (block.axisPresent[i]) {
          cmd.axisMask |= (1 << i);
          cmd.end[i] = block.axisValue[i];
        }
      }
      if (block.iPresent) cmd.center[0] = block.iNumber;
      if (block.jPresent) cmd.center[1] = block.jNumber;
      if (block.kPresent) cmd.center[2] = block.kNumber;
      if (block.fPresent) cmd.feedRate = block.fNumber;
      /* 计算半径 */
      cmd.radius = sqrtf(cmd.center[0]*cmd.center[0] +
                         cmd.center[1]*cmd.center[1] +
                         cmd.center[2]*cmd.center[2]);
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    /* 暂停 */
    case 4: {
      cmd.type = CANON_DWELL;
      cmd.dwellSec = block.pNumber;
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    /* 坐标系选择 */
    case 54: case 55: case 56: case 57:
    case 58: case 59: {
      cmd.type = CANON_SET_COORD_SYS;
      cmd.coordSystem = block.gCode - 54;
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    /* 单位 */
    case 20: state->isMetric = false; handled = true; break;
    case 21: state->isMetric = true;  handled = true; break;
    /* 绝对/增量 */
    case 90: state->isAbsolute = true;  handled = true; break;
    case 91: state->isAbsolute = false; handled = true; break;
    /* 平面选择 */
    case 17: state->activePlane = 17; handled = true; break;
    case 18: state->activePlane = 18; handled = true; break;
    case 19: state->activePlane = 19; handled = true; break;
    /* 刀具补偿 */
    case 43: cmd.type = CANON_USE_TOOL_LENGTH_OFFSET; emit_canon(&cmd, callbacks, userData); handled = true; break;
    case 49: /* 取消长度补偿 */ handled = true; break;
    /* 路径控制 */
    case 61: state->pathControl = 61; handled = true; break;
    case 64: state->pathControl = 64; handled = true; break;
    /* 回零 */
    case 28: {
      cmd.type = CANON_HOME_AXIS;
      for (int i = 0; i < MAX_AXIS_WORDS; i++) {
        if (block.axisPresent[i]) cmd.axisMask |= (1 << i);
      }
      emit_canon(&cmd, callbacks, userData);
      handled = true;
      break;
    }
    }

    if (handled) {
      /* 更新模态状态 */
      int group = gcode_to_group(block.gCode);
      if (group >= 0 && group < GC_GROUP_COUNT) {
        state->gModes[group] = block.gCode;
      }
    }
  }

  /* 处理 M 代码 */
  if (block.mPresent) {
    CanonCommandData mCmd;
    memset(&mCmd, 0, sizeof(mCmd));
    switch (block.mCode) {
    case 0:  mCmd.type = CANON_STOP;           emit_canon(&mCmd, callbacks, userData); break;
    case 1:  mCmd.type = CANON_ORIENTED_STOP;  emit_canon(&mCmd, callbacks, userData); break;
    case 2:
    case 30: mCmd.type = CANON_PROGRAM_END;    emit_canon(&mCmd, callbacks, userData); break;
    case 3:  mCmd.type = CANON_SPINDLE_ON;     mCmd.spindleDirection = 1; emit_canon(&mCmd, callbacks, userData); break;
    case 4:  mCmd.type = CANON_SPINDLE_ON;     mCmd.spindleDirection = -1; emit_canon(&mCmd, callbacks, userData); break;
    case 5:  mCmd.type = CANON_SPINDLE_OFF;    emit_canon(&mCmd, callbacks, userData); break;
    case 6:  mCmd.type = CANON_TOOL_CHANGE;    mCmd.toolNumber = block.tNumber; emit_canon(&mCmd, callbacks, userData); break;
    case 7:  mCmd.type = CANON_MIST_ON;        emit_canon(&mCmd, callbacks, userData); break;
    case 9:  mCmd.type = CANON_MIST_OFF;       emit_canon(&mCmd, callbacks, userData); break;
    case 8:  mCmd.type = CANON_FLOOD_ON;       emit_canon(&mCmd, callbacks, userData); break;
    }
  }

  /* 处理其他字 */
  if (block.fPresent && !block.gPresent) {
    state->feedRate = block.fNumber;
  }
  if (block.sPresent) {
    state->spindleSpeed = block.sNumber;
  }
  if (block.tPresent) {
    state->toolNumber = block.tNumber;
  }

  return 0;
}

int plc_gcode_executeFile(GCodeParserState *state, const char *filename,
                          const GCodeCallbacks *callbacks, void *userData)
{
  if (!state || !filename) return -1;

  FILE *fp = fopen(filename, "r");
  if (!fp) return -1;

  char line[512];
  int ret = 0;
  while (fgets(line, sizeof(line), fp)) {
    ret = plc_gcode_executeLine(state, line, callbacks, userData);
    if (ret < 0) break;
  }

  fclose(fp);
  return ret;
}

/* ==================== 工具函数 ==================== */

const char *plc_gcode_groupName(int group)
{
  static const char *names[] = {
    "?", "MOTION", "PLANE", "DISTANCE", "FEEDMODE", "UNITS",
    "PROGRAM", "CUTTERCOMP", "TOOLOFFSET", "RETURN", "?",
    "?", "COORDSYS", "PATHCTRL", "LATHE_DIAM", "SPINDLE", "MMODE"
  };
  if (group < 0 || group >= GC_GROUP_COUNT) return "?";
  return names[group];
}

const char *plc_gcode_canonName(CanonCommand type)
{
  static const char *names[] = {
    "NOP", "STRAIGHT_TRAVERSE", "STRAIGHT_FEED", "ARC_FEED",
    "DWELL", "SET_FEED_RATE", "SET_SPINDLE_SPEED", "SPINDLE_ON",
    "SPINDLE_OFF", "TOOL_CHANGE", "SELECT_TOOL", "CHANGE_TOOL",
    "MIST_ON", "MIST_OFF", "FLOOD_ON", "FLOOD_OFF",
    "COMMENT", "STOP", "ORIENTED_STOP", "PROGRAM_END",
    "HOME_AXIS", "SET_COORD_SYS", "USE_TOOL_LENGTH_OFFSET",
    "SET_TOOL_TABLE", "PROBE"
  };
  if (type < 0 || type >= (int)(sizeof(names)/sizeof(names[0]))) return "?";
  return names[type];
}

float plc_gcode_toMetric(float value, bool isMetric)
{
  return isMetric ? value : (value * 25.4f);
}

float plc_gcode_getParam(GCodeParserState *state, int param)
{
  if (!state || param < 1 || param >= 5400) return 0;
  return state->parameters[param];
}

void plc_gcode_setParam(GCodeParserState *state, int param, float value)
{
  if (!state || param < 1 || param >= 5400) return;
  state->parameters[param] = value;
}
