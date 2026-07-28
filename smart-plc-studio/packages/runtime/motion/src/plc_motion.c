#include "plc_motion.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* strdup 兼容层 (跨平台) */
static char *plc_strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  char *p = (char *)malloc(len);
  if (p) memcpy(p, s, len);
  return p;
}

/* ==================== 内部: 规范命令回调 ==================== */

typedef struct {
  CncSystem *cnc;
} CncCallbackCtx;

static void on_canon_command(const CanonCommandData *cmd, void *userData)
{
  CncCallbackCtx *ctx = (CncCallbackCtx *)userData;
  CncSystem *cnc = ctx->cnc;

  switch (cmd->type) {
  case CANON_STRAIGHT_TRAVERSE:
  case CANON_STRAIGHT_FEED: {
    /* 将规范命令转为规划段 */
    float target[9];
    uint8_t axes = 0;
    for (int i = 0; i < 9; i++) {
      if (cmd->axisMask & (1 << i)) {
        target[i] = cmd->end[i];
        axes |= (1 << i);
      } else {
        target[i] = cnc->groups[cnc->activeGroup].axes[i]
                      ? cnc->groups[cnc->activeGroup].axes[i]->commandPosition
                      : 0;
      }
    }
    float feed = (cmd->feedRate > 0) ? cmd->feedRate / 60.0f : cnc->config.maxFeedRate / 60.0f;
    plc_planner_planLinear(&cnc->planner, target, feed,
                           cnc->config.defaultAccel, cnc->config.defaultJerk);
    break;
  }
  case CANON_ARC_FEED: {
    float target[9];
    for (int i = 0; i < 9; i++) {
      if (cmd->axisMask & (1 << i)) {
        target[i] = cmd->end[i];
      } else {
        target[i] = cnc->groups[cnc->activeGroup].axes[i]
                      ? cnc->groups[cnc->activeGroup].axes[i]->commandPosition
                      : 0;
      }
    }
    float feed = (cmd->feedRate > 0) ? cmd->feedRate / 60.0f : cnc->config.maxFeedRate / 60.0f;
    plc_planner_planArc(&cnc->planner, target, cmd->center,
                        cmd->radius, cmd->arcDir,
                        feed, cnc->config.defaultAccel, cnc->config.defaultJerk);
    break;
  }
  case CANON_DWELL:
    /* 暂停由上层处理 */
    break;

  case CANON_SET_COORD_SYS:
    if (cnc->groupCount > 0) {
      plc_group_selectCoord(&cnc->groups[cnc->activeGroup],
                            (CoordSystem)(cmd->coordSystem + COORD_G54));
    }
    break;

  case CANON_SPINDLE_ON:
    /* 主轴控制由上层处理 */
    break;
  case CANON_SPINDLE_OFF:
    break;

  case CANON_STOP:
    cnc->status = CNC_STATUS_STOP;
    break;
  case CANON_PROGRAM_END:
    cnc->status = CNC_STATUS_IDLE;
    cnc->mode = CNC_MODE_IDLE;
    break;

  default:
    break;
  }
}

static void on_gcode_comment(const char *text, void *userData)
{
  (void)userData;
  (void)text;
}

static void on_gcode_error(const char *msg, void *userData)
{
  (void)userData;
  (void)msg;
}

/* ==================== CNC 初始化 ==================== */

int plc_cnc_init(CncSystem *cnc, const CncConfig *cfg)
{
  if (!cnc || !cfg) return -1;
  memset(cnc, 0, sizeof(CncSystem));

  cnc->config = *cfg;
  cnc->axisCount = (cfg->axisCount > 0 && cfg->axisCount <= 9) ? cfg->axisCount : 3;
  cnc->groupCount = (cfg->groupCount > 0 && cfg->groupCount <= 4) ? cfg->groupCount : 1;

  /* 初始化运动学 (默认直角坐标) */
  KinematicsConfig kCfg;
  memset(&kCfg, 0, sizeof(kCfg));
  kCfg.type = KINEMATICS_IDENTITY;
  plc_kinematics_init(&cnc->kins, &kCfg);

  /* 初始化 G-Code 解析器 */
  plc_gcode_init(&cnc->gcodeState);

  /* 初始化规划器 */
  PlannerConfig pCfg;
  memset(&pCfg, 0, sizeof(pCfg));
  pCfg.bufferSize = 32;
  pCfg.defaultAcceleration = cfg->defaultAccel;
  pCfg.defaultJerk = cfg->defaultJerk;
  pCfg.maxVelocity = cfg->maxFeedRate / 60.0f;
  pCfg.junctionDeviation = cfg->junctionDeviation;
  pCfg.termCond = TERM_COND_CONTINUOUS;
  pCfg.enableLookAhead = true;
  plc_planner_init(&cnc->planner, &pCfg);

  /* 初始化插补器 */
  plc_interp_init(&cnc->interpolator, cfg->servoCycleSec);

  /* 创建默认轴组 */
  for (uint32_t g = 0; g < cnc->groupCount; g++) {
    char name[16];
    snprintf(name, sizeof(name), "GROUP_%u", g);
    plc_group_init(&cnc->groups[g], g, name);
  }
  cnc->activeGroup = 0;

  /* 默认状态 */
  cnc->mode = CNC_MODE_IDLE;
  cnc->status = CNC_STATUS_IDLE;
  cnc->feedOverride = 1.0f;
  cnc->rapidOverride = 1.0f;
  cnc->spindleOverride = 1.0f;
  cnc->estop = false;

  return 0;
}

int plc_cnc_deinit(CncSystem *cnc)
{
  if (!cnc) return -1;

  for (uint32_t i = 0; i < cnc->axisCount; i++) {
    plc_axis_deinit(&cnc->axes[i]);
  }
  for (uint32_t g = 0; g < cnc->groupCount; g++) {
    plc_group_deinit(&cnc->groups[g]);
  }
  plc_planner_deinit(&cnc->planner);

  memset(cnc, 0, sizeof(CncSystem));
  return 0;
}

/* ==================== 轴管理 ==================== */

int plc_cnc_addAxis(CncSystem *cnc, uint8_t id, const char *name, const MotorConfig *motorCfg)
{
  if (!cnc || !name) return -1;
  if (id >= cnc->axisCount) return -1;

  int ret = plc_axis_init(&cnc->axes[id], id, name, motorCfg);
  if (ret < 0) return ret;

  /* 添加到默认组 */
  if (cnc->groupCount > 0 && id < 9) {
    plc_group_addAxis(&cnc->groups[0], &cnc->axes[id], id, 1.0f, 0);
  }

  return 0;
}

/* ==================== 模式/状态控制 ==================== */

int plc_cnc_setMode(CncSystem *cnc, CncMode mode)
{
  if (!cnc) return -1;
  cnc->mode = mode;
  return 0;
}

CncMode plc_cnc_getMode(const CncSystem *cnc)
{
  return cnc ? cnc->mode : CNC_MODE_IDLE;
}

int plc_cnc_start(CncSystem *cnc)
{
  if (!cnc) return -1;
  if (cnc->estop) return -CNC_ERR_ESTOP;

  cnc->status = CNC_STATUS_RUNNING;
  cnc->mode = CNC_MODE_AUTO;
  cnc->currentLine = 0;

  return 0;
}

int plc_cnc_stop(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->status = CNC_STATUS_STOP;
  cnc->mode = CNC_MODE_IDLE;
  plc_planner_clear(&cnc->planner);
  plc_interp_stop(&cnc->interpolator);
  return 0;
}

int plc_cnc_pause(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->status = CNC_STATUS_PAUSED;
  plc_interp_hold(&cnc->interpolator);
  return 0;
}

int plc_cnc_resume(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->status = CNC_STATUS_RUNNING;
  plc_interp_resume(&cnc->interpolator);
  return 0;
}

int plc_cnc_estop(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->estop = true;
  cnc->status = CNC_STATUS_ESTOP;
  plc_planner_clear(&cnc->planner);
  plc_interp_stop(&cnc->interpolator);
  for (uint32_t i = 0; i < cnc->axisCount; i++) {
    plc_axis_abort(&cnc->axes[i]);
  }
  return 0;
}

int plc_cnc_clearEstop(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->estop = false;
  cnc->status = CNC_STATUS_IDLE;
  return 0;
}

/* ==================== 进给控制 ==================== */

void plc_cnc_setFeedOverride(CncSystem *cnc, float override)
{
  if (cnc) cnc->feedOverride = (override < 0) ? 0 : (override > 2) ? 2 : override;
}

void plc_cnc_setRapidOverride(CncSystem *cnc, float override)
{
  if (cnc) cnc->rapidOverride = (override < 0) ? 0 : (override > 1) ? 1 : override;
}

void plc_cnc_setSpindleOverride(CncSystem *cnc, float override)
{
  if (cnc) cnc->spindleOverride = (override < 0) ? 0 : (override > 2) ? 2 : override;
}

/* ==================== G-Code 加载和执行 ==================== */

int plc_cnc_loadGCode(CncSystem *cnc, const char **lines, uint32_t count)
{
  if (!cnc || !lines) return -1;
  cnc->gcodeLines = lines;
  cnc->gcodeLineCount = count;
  cnc->currentLine = 0;
  plc_gcode_reset(&cnc->gcodeState);
  return 0;
}

int plc_cnc_loadGCodeFile(CncSystem *cnc, const char *filename)
{
  if (!cnc || !filename) return -1;

  FILE *fp = fopen(filename, "r");
  if (!fp) return -1;

  /* 统计行数 */
  char buf[512];
  uint32_t count = 0;
  while (fgets(buf, sizeof(buf), fp)) count++;
  fseek(fp, 0, SEEK_SET);

  /* 分配内存 */
  const char **lines = (const char **)calloc(count, sizeof(char *));
  if (!lines) { fclose(fp); return -1; }

  uint32_t idx = 0;
  while (fgets(buf, sizeof(buf), fp) && idx < count) {
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    lines[idx] = plc_strdup(buf);
    idx++;
  }
  fclose(fp);

  return plc_cnc_loadGCode(cnc, lines, count);
}

int plc_cnc_mdi(CncSystem *cnc, const char *command)
{
  if (!cnc || !command) return -1;

  strncpy(cnc->mdiBuffer, command, sizeof(cnc->mdiBuffer) - 1);
  cnc->mode = CNC_MODE_MDI;
  cnc->status = CNC_STATUS_RUNNING;

  CncCallbackCtx ctx = { cnc };
  GCodeCallbacks cbs;
  cbs.onCanonCommand = on_canon_command;
  cbs.onComment = on_gcode_comment;
  cbs.onError = on_gcode_error;

  int ret = plc_gcode_executeLine(&cnc->gcodeState, command, &cbs, &ctx);

  /* 执行前瞻 */
  if (ret == 0 && !plc_planner_isEmpty(&cnc->planner)) {
    plc_planner_lookAhead(&cnc->planner);
  }

  cnc->status = CNC_STATUS_IDLE;
  return ret;
}

/* ==================== 手动/点动 ==================== */

int plc_cnc_jog(CncSystem *cnc, uint8_t axisIdx, float velocity)
{
  if (!cnc) return -1;
  if (axisIdx >= cnc->axisCount) return -1;

  cnc->mode = CNC_MODE_MANUAL;
  return plc_axis_moveVel(&cnc->axes[axisIdx], velocity, cnc->config.defaultAccel);
}

int plc_cnc_jogStop(CncSystem *cnc, uint8_t axisIdx)
{
  if (!cnc) return -1;
  if (axisIdx >= cnc->axisCount) return -1;
  return plc_axis_stop(&cnc->axes[axisIdx]);
}

/* ==================== 回零 ==================== */

int plc_cnc_homeAll(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->mode = CNC_MODE_HOMING;
  for (uint32_t g = 0; g < cnc->groupCount; g++) {
    plc_group_homeAll(&cnc->groups[g]);
  }
  return 0;
}

/* ==================== 主轴 ==================== */

int plc_cnc_spindleOn(CncSystem *cnc, int direction, float speed)
{
  if (!cnc) return -1;
  cnc->gcodeState.spindleDirection = direction;
  cnc->gcodeState.spindleSpeed = speed;
  return 0;
}

int plc_cnc_spindleOff(CncSystem *cnc)
{
  if (!cnc) return -1;
  cnc->gcodeState.spindleDirection = 0;
  return 0;
}

/* ==================== 冷却 ==================== */

int plc_cnc_coolantMistOn(CncSystem *cnc) { (void)cnc; return 0; }
int plc_cnc_coolantMistOff(CncSystem *cnc) { (void)cnc; return 0; }
int plc_cnc_coolantFloodOn(CncSystem *cnc) { (void)cnc; return 0; }
int plc_cnc_coolantFloodOff(CncSystem *cnc) { (void)cnc; return 0; }

/* ==================== 周期性更新 ==================== */

int plc_cnc_update(CncSystem *cnc, float dtSec)
{
  if (!cnc) return -1;
  if (cnc->estop) return -CNC_ERR_ESTOP;

  /* 1. 更新所有轴组 */
  for (uint32_t g = 0; g < cnc->groupCount; g++) {
    plc_group_update(&cnc->groups[g], dtSec);
  }

  /* 2. 自动运行模式下，从 G-Code 队列读取下一行 */
  if (cnc->mode == CNC_MODE_AUTO && cnc->status == CNC_STATUS_RUNNING) {
    /* 如果规划器为空且还有 G-Code 行，读取下一行 */
    if (plc_planner_isEmpty(&cnc->planner) && cnc->currentLine < cnc->gcodeLineCount) {
      CncCallbackCtx ctx = { cnc };
      GCodeCallbacks cbs;
      cbs.onCanonCommand = on_canon_command;
      cbs.onComment = on_gcode_comment;
      cbs.onError = on_gcode_error;

      const char *line = cnc->gcodeLines[cnc->currentLine];
      int ret = plc_gcode_executeLine(&cnc->gcodeState, line, &cbs, &ctx);
      cnc->currentLine++;

      /* 执行前瞻 */
      if (ret == 0) {
        plc_planner_lookAhead(&cnc->planner);
      }
    }

    /* 如果规划器有段，喂给插补器 */
    if (!plc_planner_isEmpty(&cnc->planner) && plc_interp_isIdle(&cnc->interpolator)) {
      PlannerSegment seg;
      if (plc_planner_getNext(&cnc->planner, &seg) == 0) {
        plc_interp_loadSegment(&cnc->interpolator, &seg);
        plc_interp_start(&cnc->interpolator);
      }
    }

    /* 检查是否完成 */
    if (plc_planner_isEmpty(&cnc->planner) &&
        plc_interp_isIdle(&cnc->interpolator) &&
        cnc->currentLine >= cnc->gcodeLineCount) {
      cnc->status = CNC_STATUS_IDLE;
    }
  }

  /* 3. 执行插补步骤 */
  if (cnc->interpolator.mode == INTERP_MODE_RUNNING) {
    float jointPos[9];
    uint8_t axisCount = 0;
    int ret = plc_interp_step(&cnc->interpolator, jointPos, &axisCount);

    /* 将插补位置发送到轴 */
    for (uint8_t i = 0; i < axisCount && i < cnc->axisCount; i++) {
      if (cnc->groups[cnc->activeGroup].axes[i]) {
        /* 通过运动学转换 */
        CartesianPose pose;
        plc_kinematics_forward(&cnc->kins, jointPos, &pose);

        /* 目前简化: 直接发送关节位置到轴 */
        plc_axis_moveAbs(cnc->groups[cnc->activeGroup].axes[i],
                         jointPos[i], 0, 0);
      }
    }

    if (ret == 1) {
      /* 段结束，插补器回到空闲 */
    }
  }

  return 0;
}

/* ==================== 状态查询 ==================== */

CncStatus plc_cnc_getStatus(const CncSystem *cnc)
{
  return cnc ? cnc->status : CNC_STATUS_NONE;
}

const char *plc_cnc_statusStr(CncStatus status)
{
  static const char *names[] = {
    "NONE", "IDLE", "RUNNING", "PAUSED", "HOLD", "STOP", "FAULT", "ESTOP"
  };
  if (status < 0 || status >= (int)(sizeof(names)/sizeof(names[0]))) return "?";
  return names[status];
}

const char *plc_cnc_modeStr(CncMode mode)
{
  static const char *names[] = {
    "IDLE", "MANUAL", "MDI", "AUTO", "SINGLE_BLOCK", "HOMING"
  };
  if (mode < 0 || mode >= (int)(sizeof(names)/sizeof(names[0]))) return "?";
  return names[mode];
}

int plc_cnc_lastError(CncSystem *cnc)
{
  (void)cnc;
  return 0;
}

const char *plc_cnc_errorStr(int err)
{
  static const char *errors[] = {
    "OK", "GENERAL", "INVALID_PARAM", "NOT_INIT", "BUSY",
    "TIMEOUT", "LIMIT", "FOLLOWING", "HOMING", "GCODE",
    "PLANNER_FULL", "ESTOP", "NO_AXIS", "NO_FILE"
  };
  int idx = -err;
  if (idx < 0 || idx >= (int)(sizeof(errors)/sizeof(errors[0]))) return "?";
  return errors[idx];
}
