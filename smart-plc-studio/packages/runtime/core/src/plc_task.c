/**
 * plc_task.c - 任务调度模块实现
 *
 * 实现 IEC 61131-3 任务模型：
 * - 周期任务（Cyclic）
 * - 事件任务（Event）
 * - 初始化任务（Startup）
 * - 自由运行任务（FreeRun）
 * 支持优先级调度和执行时间追踪
 */

#include "plc_task.h"
#include <string.h>

void plc_task_init(PlcTaskScheduler* scheduler) {
  if (scheduler == NULL) return;
  memset(scheduler, 0, sizeof(PlcTaskScheduler));
  scheduler->running = false;
}

int plc_task_create(PlcTaskScheduler* scheduler, const char* name,
                    TaskType type, uint32_t interval_ms, uint8_t priority,
                    TaskCallback callback, void* context) {
  if (scheduler == NULL || name == NULL || callback == NULL) return -1;

  /* 检查是否还有空闲槽位 */
  if (scheduler->task_count >= PLC_MAX_TASKS) return -1;

  /* 查找空闲槽位 */
  int slot = -1;
  for (int i = 0; i < PLC_MAX_TASKS; i++) {
    if (scheduler->tasks[i].callback == NULL) {
      slot = i;
      break;
    }
  }
  if (slot < 0) return -1;

  PlcTask* task = &scheduler->tasks[slot];
  task->id = (uint8_t)slot;
  task->name = name;
  task->type = type;
  task->state = TASK_STATE_IDLE;
  task->interval_ms = interval_ms;
  task->priority = priority;
  task->last_run_tick = 0;
  task->exec_count = 0;
  task->exec_time_us = 0;
  task->max_exec_us = 0;
  task->callback = callback;
  task->context = context;
  task->enabled = false;
  task->one_shot = false;

  scheduler->task_count++;

  return slot;
}

void plc_task_start_all(PlcTaskScheduler* scheduler) {
  if (scheduler == NULL) return;

  for (int i = 0; i < PLC_MAX_TASKS; i++) {
    PlcTask* task = &scheduler->tasks[i];
    if (task->callback != NULL) {
      task->enabled = true;
      task->state = TASK_STATE_READY;
    }
  }
  scheduler->running = true;
}

void plc_task_stop_all(PlcTaskScheduler* scheduler) {
  if (scheduler == NULL) return;

  for (int i = 0; i < PLC_MAX_TASKS; i++) {
    PlcTask* task = &scheduler->tasks[i];
    if (task->callback != NULL) {
      task->enabled = false;
      task->state = TASK_STATE_IDLE;
    }
  }
  scheduler->running = false;
}

void plc_task_start(PlcTaskScheduler* scheduler, uint8_t task_id) {
  if (scheduler == NULL || task_id >= PLC_MAX_TASKS) return;

  PlcTask* task = &scheduler->tasks[task_id];
  if (task->callback != NULL) {
    task->enabled = true;
    task->state = TASK_STATE_READY;
  }
}

void plc_task_stop(PlcTaskScheduler* scheduler, uint8_t task_id) {
  if (scheduler == NULL || task_id >= PLC_MAX_TASKS) return;

  PlcTask* task = &scheduler->tasks[task_id];
  task->enabled = false;
  task->state = TASK_STATE_IDLE;
}

void plc_task_schedule(PlcTaskScheduler* scheduler) {
  if (scheduler == NULL || !scheduler->running) return;

  uint32_t now_tick = plc_platform_tick_ms();
  scheduler->tick_ms = now_tick;
  scheduler->total_cycles++;

  /* 按优先级排序执行：遍历所有任务，找到最高优先级的就绪任务 */
  for (int pass = 0; pass < PLC_MAX_TASKS; pass++) {
    uint8_t highest_priority = 0;
    int highest_idx = -1;

    /* 找到当前最高优先级的就绪任务 */
    for (int i = 0; i < PLC_MAX_TASKS; i++) {
      PlcTask* task = &scheduler->tasks[i];

      /* 跳过空槽位和禁用的任务 */
      if (task->callback == NULL || !task->enabled) continue;

      /* STARTUP 任务只执行一次 */
      if (task->type == TASK_TYPE_STARTUP && task->exec_count > 0) {
        task->enabled = false;
        continue;
      }

      /* 检查任务是否就绪 */
      bool ready = false;
      if (task->type == TASK_TYPE_FREERUN || task->type == TASK_TYPE_STARTUP) {
        /* 自由运行和启动任务始终就绪 */
        ready = true;
      } else {
        /* 周期和事件任务：检查周期是否到达 */
        uint32_t elapsed = now_tick - task->last_run_tick;
        if (elapsed >= task->interval_ms) {
          ready = true;
        }
      }

      if (ready && task->priority >= highest_priority) {
        highest_priority = task->priority;
        highest_idx = i;
      }
    }

    /* 没有就绪任务则退出 */
    if (highest_idx < 0) break;

    PlcTask* task = &scheduler->tasks[highest_idx];

    /* 记录开始执行时间 */
    task->state = TASK_STATE_RUNNING;
    uint64_t start_us = plc_platform_tick_us();

    /* 执行任务回调 */
    task->callback(task->context);

    /* 记录执行时间 */
    uint64_t end_us = plc_platform_tick_us();
    uint32_t elapsed_us = (uint32_t)(end_us - start_us);
    task->exec_time_us = elapsed_us;
    if (elapsed_us > task->max_exec_us) {
      task->max_exec_us = elapsed_us;
    }

    /* 检测执行超时（超过周期的 80% 视为超时） */
    uint32_t interval_us = task->interval_ms * 1000;
    if (task->type != TASK_TYPE_FREERUN && task->type != TASK_TYPE_STARTUP) {
      if (elapsed_us > (interval_us * 8 / 10)) {
        scheduler->total_overruns++;
      }
    }

    /* 更新执行统计 */
    task->exec_count++;
    task->last_run_tick = now_tick;

    /* 单次任务执行后禁用 */
    if (task->one_shot) {
      task->enabled = false;
      task->state = TASK_STATE_IDLE;
    } else {
      task->state = TASK_STATE_READY;
    }
  }
}

PlcTask* plc_task_get(PlcTaskScheduler* scheduler, uint8_t task_id) {
  if (scheduler == NULL || task_id >= PLC_MAX_TASKS) return NULL;

  PlcTask* task = &scheduler->tasks[task_id];
  if (task->callback == NULL) return NULL;

  return task;
}

void plc_task_get_stats(PlcTaskScheduler* scheduler,
                        uint32_t* cycles, uint32_t* overruns) {
  if (scheduler == NULL) return;
  if (cycles != NULL) *cycles = scheduler->total_cycles;
  if (overruns != NULL) *overruns = scheduler->total_overruns;
}
