#ifndef PLC_PLANNER_H
#define PLC_PLANNER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 轨迹段类型 ==================== */
typedef enum {
  SEGMENT_LINEAR = 0,
  SEGMENT_CIRCULAR_CW,
  SEGMENT_CIRCULAR_CCW,
} SegmentType;

/* ==================== 终止条件 ==================== */
typedef enum {
  TERM_COND_EXACT_STOP = 0,    /* 精确停止 (G61) */
  TERM_COND_EXACT_PATH,        /* 精确路径 (G61.1) */
  TERM_COND_CONTINUOUS,        /* 连续模式 (G64) */
} TermCondition;

/* ==================== 轨迹段 (规划单元) ==================== */
typedef struct {
  SegmentType type;
  uint8_t axisMask;                   /* 参与轴位掩码 */

  /* 终点 (工作坐标) */
  float target[9];

  /* 圆弧参数 */
  float center[3];                    /* I, J, K */
  float radius;
  int dir;                            /* 1=CW, -1=CCW */

  /* 路径长度 */
  float length;                       /* 路径总长 (mm) */

  /* 速度规划 */
  float entryVelocity;                /* 进入速度 */
  float plannedVelocity;              /* 规划最大速度 */
  float exitVelocity;                 /* 退出速度 (考虑拐角) */
  float acceleration;                 /* 段加速度 */
  float jerk;                         /* 加加速度 */

  /* 剖面参数 (S-curve) */
  float accelerateUntil;              /* 加速结束点 (距离占比) */
  float decelerateAfter;              /* 减速开始点 (距离占比) */
  float cruiseVelocity;               /* 匀速段速度 */

  /* 几何 */
  float direction[9];                 /* 单位方向向量 */
  float junctionCos;                  /* 拐角余弦值 */
  float junctionVelocity;             /* 拐角最大允许速度 */
  uint8_t segmentId;
} PlannerSegment;

/* ==================== 规划器配置 ==================== */
typedef struct {
  uint32_t bufferSize;                /* 规划缓冲区大小 (默认 32) */
  float defaultAcceleration;          /* 默认加速度 (mm/s²) */
  float defaultJerk;                  /* 默认加加速度 (mm/s³) */
  float maxVelocity;                  /* 全局最大速度 */
  float junctionDeviation;            /* 拐角偏差 (mm, G64 公差) */
  TermCondition termCond;             /* 终止条件 */
  bool enableLookAhead;               /* 启用前瞻 */
} PlannerConfig;

/* ==================== 规划器状态 ==================== */
typedef struct {
  PlannerConfig config;

  /* 段队列 */
  PlannerSegment *buffer;
  uint32_t bufferSize;
  uint32_t head;
  uint32_t tail;
  uint32_t count;

  /* 当前位置 */
  float currentPosition[9];
  float currentVelocity;

  /* 规划器状态 */
  bool isRunning;
  bool isEmpty;
  uint32_t plannedCount;              /* 已规划的段数 */
  uint32_t executedCount;             /* 已执行的段数 */
} Planner;

/* ==================== 规划器 API ==================== */

/* 初始化 */
int plc_planner_init(Planner *planner, const PlannerConfig *cfg);
int plc_planner_deinit(Planner *planner);

/* 配置 */
void plc_planner_setMaxVelocity(Planner *planner, float vMax);
void plc_planner_setAcceleration(Planner *planner, float accel);
void plc_planner_setJerk(Planner *planner, float jerk);
void plc_planner_setJunctionDeviation(Planner *planner, float dev);
void plc_planner_setTermCondition(Planner *planner, TermCondition cond);

/* 规划接口 */
int plc_planner_planLinear(Planner *planner, const float *target,
                           float feedRate, float accel, float jerk);
int plc_planner_planArc(Planner *planner, const float *target,
                        const float *center, float radius, int dir,
                        float feedRate, float accel, float jerk);

/* 前瞻规划器 (执行所有段的减速/拐角速度计算) */
int plc_planner_lookAhead(Planner *planner);

/* 清空队列 */
int plc_planner_clear(Planner *planner);

/* 获取下一个要执行的段 */
int plc_planner_getNext(Planner *planner, PlannerSegment *seg);

/* 查询 */
uint32_t plc_planner_available(const Planner *planner);  /* 可用槽位数 */
uint32_t plc_planner_queued(const Planner *planner);      /* 已排队数 */
bool plc_planner_isEmpty(const Planner *planner);
bool plc_planner_isFull(const Planner *planner);

/* 轨迹段上的插补 (给定 s [0..1] 返回位置) */
void plc_planner_interpolate(const PlannerSegment *seg, float s, float *pos);

/* ==================== 数学工具 ==================== */
float plc_plan_length(const float *start, const float *end, uint8_t axisMask);
float plc_plan_arcLength(float radius, float startAngle, float endAngle);
float plc_plan_junctionVelocity(const float *dirA, const float *dirB,
                                float maxAccel, float junctionDeviation);
float plc_plan_profileTime(float length, float vEntry, float vPlan,
                           float vExit, float accel, float jerk,
                           float *outTacc, float *outTdec);

#ifdef __cplusplus
}
#endif

#endif /* PLC_PLANNER_H */
