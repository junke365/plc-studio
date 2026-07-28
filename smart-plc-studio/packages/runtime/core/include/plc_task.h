/**
 * plc_task.h - 任务调度模块
 *
 * 实现 IEC 61131-3 任务模型：
 * - 周期任务（Cyclic）
 * - 事件任务（Event）
 * - 初始化任务（Startup）
 * 支持优先级调度和抢占
 */

#ifndef PLC_TASK_H
#define PLC_TASK_H

#include "plc_platform.h"

/* ========== 任务类型 ========== */

typedef enum {
  TASK_TYPE_CYCLIC,    /* 周期执行 */
  TASK_TYPE_EVENT,     /* 事件触发 */
  TASK_TYPE_STARTUP,   /* 上电初始化（执行一次） */
  TASK_TYPE_FREERUN,   /* 自由运行（尽量快） */
} TaskType;

/* 任务状态 */
typedef enum {
  TASK_STATE_IDLE,
  TASK_STATE_READY,
  TASK_STATE_RUNNING,
  TASK_STATE_WAITING,
  TASK_STATE_ERROR,
} TaskState;

/* 任务回调函数类型 */
typedef void (*TaskCallback)(void* context);

/* 任务描述 */
typedef struct {
  uint8_t        id;
  const char*    name;
  TaskType       type;
  TaskState      state;
  uint32_t       interval_ms;   /* 周期（毫秒） */
  uint8_t        priority;      /* 优先级 0-255, 数值越大优先级越高 */
  uint32_t       last_run_tick; /* 上次执行时间戳 */
  uint32_t       exec_count;    /* 执行计数 */
  uint32_t       exec_time_us;  /* 最近一次执行耗时（微秒） */
  uint32_t       max_exec_us;   /* 最大执行耗时（微秒） */
  TaskCallback   callback;      /* 执行回调 */
  void*          context;       /* 回调上下文 */
  bool           enabled;       /* 是否启用 */
  bool           one_shot;      /* 是否只执行一次 */
} PlcTask;

/* 任务调度器 */
typedef struct {
  PlcTask        tasks[PLC_MAX_TASKS];
  uint8_t        task_count;
  uint8_t        current_task_id;
  uint32_t       tick_ms;
  uint32_t       total_cycles;
  uint32_t       total_overruns;
  bool           running;
} PlcTaskScheduler;

/* ========== 接口函数 ========== */

/**
 * 初始化任务调度器
 */
void plc_task_init(PlcTaskScheduler* scheduler);

/**
 * 创建任务
 * @return 任务ID，-1=失败
 */
int plc_task_create(PlcTaskScheduler* scheduler, const char* name,
                    TaskType type, uint32_t interval_ms, uint8_t priority,
                    TaskCallback callback, void* context);

/**
 * 启动所有任务
 */
void plc_task_start_all(PlcTaskScheduler* scheduler);

/**
 * 停止所有任务
 */
void plc_task_stop_all(PlcTaskScheduler* scheduler);

/**
 * 启动指定任务
 */
void plc_task_start(PlcTaskScheduler* scheduler, uint8_t task_id);

/**
 * 停止指定任务
 */
void plc_task_stop(PlcTaskScheduler* scheduler, uint8_t task_id);

/**
 * 调度器主循环（在平台主循环中调用）
 * 检查所有任务的周期是否到达，到达则执行
 */
void plc_task_schedule(PlcTaskScheduler* scheduler);

/**
 * 获取指定任务信息
 */
PlcTask* plc_task_get(PlcTaskScheduler* scheduler, uint8_t task_id);

/**
 * 获取调度器统计信息
 */
void plc_task_get_stats(PlcTaskScheduler* scheduler,
                        uint32_t* cycles, uint32_t* overruns);

#endif /* PLC_TASK_H */
