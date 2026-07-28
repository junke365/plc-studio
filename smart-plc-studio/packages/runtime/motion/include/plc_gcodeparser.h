#ifndef PLC_GCODEPARSER_H
#define PLC_GCODEPARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 模态 G 代码组 ==================== */
#define GC_GROUP_COUNT 17

/* G 代码模态组编号 (LinuxCNC 风格) */
typedef enum {
  GC_MOTION       = 1,   /* G0, G1, G2, G3, G33, G38.x, G73, G76, G80-89 */
  GC_PLANE        = 2,   /* G17, G18, G19 */
  GC_DISTANCE     = 3,   /* G90, G91 */
  GC_FEEDMODE     = 4,   /* G93, G94, G95 */
  GC_UNITS        = 5,   /* G20, G21 */
  GC_PROGRAM      = 6,   /* G98, G99 */
  GC_CUTTERCOMP   = 7,   /* G40, G41, G42 */
  GC_TOOLOFFSET   = 8,   /* G43, G49 */
  GC_RETURN       = 9,   /* G98, G99 */
  GC_COORDSYS     = 12,  /* G54-G59.3 */
  GC_PATHCTRL     = 13,  /* G61, G61.1, G64 */
  GC_LATHE_DIAM   = 14,  /* G7, G8 */
  GC_SPINDLE      = 15,  /* G96, G97 */
  GC_MMODE        = 16,  /* G98, G99 */
} GCodeGroup;

/* ==================== M 代码模态组 ==================== */
#define MC_GROUP_COUNT 11

/* ==================== 解析块 (Block) ==================== */
#define MAX_AXIS_WORDS 9

typedef struct {
  /* 轴字 */
  bool axisPresent[MAX_AXIS_WORDS];
  float axisValue[MAX_AXIS_WORDS];    /* X, Y, Z, A, B, C, U, V, W */

  /* 预备字 */
  bool gPresent;
  int gCode;                           /* G 代码号 */
  int gModes[GC_GROUP_COUNT];          /* 每组的模态值 (-1 表示未设置) */

  /* 辅助字 */
  bool mPresent;
  int mCode;
  bool mModal[MC_GROUP_COUNT];

  /* 其他字 */
  bool fPresent;  float fNumber;       /* 进给率 */
  bool sPresent;  float sNumber;       /* 主轴速度 */
  bool tPresent;  int   tNumber;       /* 刀具号 */
  bool dPresent;  int   dNumber;       /* 刀具半径补偿号 */
  bool hPresent;  int   hNumber;       /* 刀具长度补偿号 */
  bool pPresent;  float pNumber;       /* 暂停时间 / 子程序号 */
  bool qPresent;  float qNumber;       /* 啄钻增量 / G83 步进 */
  bool rPresent;  float rNumber;       /* 固定循环 R 平面 */
  bool lPresent;  int   lNumber;       /* 重复次数 (默认 1) */
  bool iPresent;  float iNumber;       /* 圆弧中心 X 偏移 / G87 */
  bool jPresent;  float jNumber;       /* 圆弧中心 Y 偏移 */
  bool kPresent;  float kNumber;       /* 圆弧中心 Z 偏移 */

  /* 注释 */
  char comment[256];

  /* 行号与标签 */
  int lineNumber;
  char label[64];

  /* O-word */
  bool oWord;
  int oType;                           /* O_SUB, O_REPEAT, etc. */
  char oName[64];
  int oArg;

  /* 标志 */
  bool isMotion;                       /* 包含运动指令 */
  int motionToBe;                      /* 运动类型 (G0/G1/G2/G3/...) */
  bool changed;                        /* 块有有效内容 */
} GCodeBlock;

/* ==================== G-Code 解析器状态 ==================== */
typedef struct {
  /* 当前模态状态 */
  int gModes[GC_GROUP_COUNT];          /* 每组当前模态值 */
  int mModals[MC_GROUP_COUNT];

  /* 当前位置 */
  float axisPos[MAX_AXIS_WORDS];       /* 各轴当前位置 */

  /* 进给率 */
  float feedRate;
  float spindleSpeed;
  int spindleDirection;                /* 0=off, 1=CW, 2=CCW */

  /* 刀具 */
  int toolNumber;
  int toolLengthOffset;
  int toolRadiusOffset;

  /* 坐标系 */
  int activeCoord;                     /* 0=G54 ... 6=G59.3 */

  /* 单位 */
  bool isMetric;                       /* true=mm(G21), false=inch(G20) */

  /* 距离模式 */
  bool isAbsolute;                     /* true=G90, false=G91 */

  /* 平面选择 */
  int activePlane;                     /* 17=XY, 18=XZ, 19=YZ */

  /* 路径控制 */
  int pathControl;                     /* 61=精确停, 64=连续 */

  /* 固定循环 */
  int cycleState;

  /* 参数 */
  float parameters[5400];              /* #1 - #5399 */

  /* 内部 */
  int lineNumber;
  int sequenceNumber;
  char lastLine[256];
} GCodeParserState;

/* ==================== 规范命令 (Canonical Command) ==================== */
typedef enum {
  CANON_NOP = 0,
  CANON_STRAIGHT_TRAVERSE,       /* G0 */
  CANON_STRAIGHT_FEED,           /* G1 */
  CANON_ARC_FEED,                /* G2/G3 */
  CANON_DWELL,                   /* G4 */
  CANON_SET_FEED_RATE,
  CANON_SET_SPINDLE_SPEED,
  CANON_SPINDLE_ON,
  CANON_SPINDLE_OFF,
  CANON_TOOL_CHANGE,
  CANON_SELECT_TOOL,
  CANON_CHANGE_TOOL,
  CANON_MIST_ON,
  CANON_MIST_OFF,
  CANON_FLOOD_ON,
  CANON_FLOOD_OFF,
  CANON_COMMENT,
  CANON_STOP,                    /* M0 */
  CANON_ORIENTED_STOP,           /* M1 */
  CANON_PROGRAM_END,             /* M2/M30 */
  CANON_HOME_AXIS,
  CANON_SET_COORD_SYS,
  CANON_USE_TOOL_LENGTH_OFFSET,
  CANON_SET_TOOL_TABLE,
  CANON_PROBE,
} CanonCommand;

typedef struct {
  CanonCommand type;

  /* 直线/圆弧数据 */
  uint8_t axisMask;                     /* 哪些轴参与 */
  float end[MAX_AXIS_WORDS];            /* 终点 */
  float center[3];                      /* 圆弧中心 (I,J,K) */
  int arcDir;                           /* 1=CW, -1=CCW */
  float radius;                         /* 圆弧半径 */

  /* 速度 */
  float feedRate;
  float spindleSpeed;

  /* 辅助 */
  int toolNumber;
  int spindleOn;
  int spindleDirection;
  int coordSystem;
  float dwellSec;
  char comment[256];
} CanonCommandData;

/* ==================== 回调接口 ==================== */
typedef struct {
  void (*onCanonCommand)(const CanonCommandData *cmd, void *userData);
  void (*onComment)(const char *text, void *userData);
  void (*onError)(const char *msg, void *userData);
} GCodeCallbacks;

/* ==================== 解析器 API ==================== */

/* 初始化解析器 */
void plc_gcode_init(GCodeParserState *state);

/* 重置解析器状态 */
void plc_gcode_reset(GCodeParserState *state);

/* 解析一行 G-Code (不执行) */
int plc_gcode_parseLine(GCodeParserState *state, const char *line, GCodeBlock *block);

/* 解析并执行一行 G-Code (产生规范命令) */
int plc_gcode_executeLine(GCodeParserState *state, const char *line,
                          const GCodeCallbacks *callbacks, void *userData);

/* 解析并执行一个完整的 G-Code 文件 */
int plc_gcode_executeFile(GCodeParserState *state, const char *filename,
                          const GCodeCallbacks *callbacks, void *userData);

/* 工具函数 */
const char *plc_gcode_groupName(int group);
const char *plc_gcode_canonName(CanonCommand type);
float plc_gcode_toMetric(float value, bool isMetric);

/* 参数读写 */
float plc_gcode_getParam(GCodeParserState *state, int param);
void  plc_gcode_setParam(GCodeParserState *state, int param, float value);

#ifdef __cplusplus
}
#endif

#endif /* PLC_GCODEPARSER_H */
