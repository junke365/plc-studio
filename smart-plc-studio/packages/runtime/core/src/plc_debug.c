/**
 * plc_debug.c - 调试模块实现
 *
 * 提供运行时调试功能：断点管理、变量监控、单步执行、日志输出
 * 断点使用线性搜索匹配
 */

#include "plc_debug.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void plc_debug_init(PlcDebugger* debugger) {
  if (debugger == NULL) return;
  memset(debugger, 0, sizeof(PlcDebugger));
}

void plc_debug_start(PlcDebugger* debugger, DebugCallback callback, void* ctx) {
  if (debugger == NULL) return;

  debugger->session.active = true;
  debugger->session.stepping = false;
  debugger->session.current_pou = 0;
  debugger->session.current_line = 0;
  debugger->session.callback = callback;
  debugger->session.callback_ctx = ctx;
}

void plc_debug_stop(PlcDebugger* debugger) {
  if (debugger == NULL) return;

  debugger->session.active = false;
  debugger->session.stepping = false;
  debugger->session.callback = NULL;
  debugger->session.callback_ctx = NULL;
}

int plc_debug_add_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line) {
  if (debugger == NULL) return -1;

  /* 检查是否已存在相同的断点 */
  for (uint8_t i = 0; i < debugger->breakpoint_count; i++) {
    PlcBreakpoint* bp = &debugger->breakpoints[i];
    if (bp->pou_id == pou_id && bp->line == line) {
      /* 已存在，直接返回索引 */
      return (int)i;
    }
  }

  /* 检查是否已满 */
  if (debugger->breakpoint_count >= PLC_MAX_BREAKPOINTS) return -1;

  /* 添加新断点 */
  uint8_t idx = debugger->breakpoint_count;
  PlcBreakpoint* bp = &debugger->breakpoints[idx];
  bp->pou_id = pou_id;
  bp->line = line;
  bp->enabled = true;
  bp->hit_count = 0;

  debugger->breakpoint_count++;

  return (int)idx;
}

void plc_debug_remove_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line) {
  if (debugger == NULL) return;

  for (uint8_t i = 0; i < debugger->breakpoint_count; i++) {
    PlcBreakpoint* bp = &debugger->breakpoints[i];
    if (bp->pou_id == pou_id && bp->line == line) {
      /* 将最后一个断点移动到当前位置（压缩数组） */
      if (i < debugger->breakpoint_count - 1) {
        debugger->breakpoints[i] = debugger->breakpoints[debugger->breakpoint_count - 1];
      }
      debugger->breakpoint_count--;
      return;
    }
  }
}

void plc_debug_enable_breakpoint(PlcDebugger* debugger, uint8_t index, bool enabled) {
  if (debugger == NULL || index >= debugger->breakpoint_count) return;
  debugger->breakpoints[index].enabled = enabled;
}

bool plc_debug_check_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line) {
  if (debugger == NULL || !debugger->session.active) return false;

  /* 线性搜索所有断点 */
  for (uint8_t i = 0; i < debugger->breakpoint_count; i++) {
    PlcBreakpoint* bp = &debugger->breakpoints[i];
    if (bp->enabled && bp->pou_id == pou_id && bp->line == line) {
      /* 断点命中 */
      bp->hit_count++;
      debugger->total_breaks++;

      /* 更新当前执行位置 */
      debugger->session.current_pou = pou_id;
      debugger->session.current_line = line;

      /* 触发回调通知 */
      if (debugger->session.callback != NULL) {
        /* 将命中信息打包传递给回调 */
        struct {
          uint8_t pou_id;
          uint32_t line;
          uint32_t hit_count;
        } hit_info;
        hit_info.pou_id = pou_id;
        hit_info.line = line;
        hit_info.hit_count = bp->hit_count;

        debugger->session.callback(DBG_EVENT_BREAKPOINT_HIT, &hit_info, sizeof(hit_info));
      }

      return true;
    }
  }

  return false;
}

void plc_debug_step(PlcDebugger* debugger) {
  if (debugger == NULL) return;

  debugger->session.stepping = true;

  /* 触发步进完成回调 */
  if (debugger->session.callback != NULL) {
    debugger->session.callback(DBG_EVENT_STEP_COMPLETE, NULL, 0);
  }
}

void plc_debug_continue(PlcDebugger* debugger) {
  if (debugger == NULL) return;
  debugger->session.stepping = false;
}

void plc_debug_pause(PlcDebugger* debugger) {
  if (debugger == NULL) return;
  debugger->session.active = false;
}

void plc_debug_notify_variable(PlcDebugger* debugger, const PlcVariable* var) {
  if (debugger == NULL || !debugger->session.active) return;
  if (var == NULL) return;

  if (debugger->session.callback != NULL) {
    debugger->session.callback(DBG_EVENT_VARIABLE_CHANGE, var, sizeof(PlcVariable));
  }
}

void plc_debug_log(PlcDebugger* debugger, uint8_t level, const char* fmt, ...) {
  if (debugger == NULL || !debugger->session.active) return;
  if (fmt == NULL) return;

  /* 使用平台日志输出 */
  va_list args;
  va_start(args, fmt);
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  plc_platform_log(level, "%s", buf);

  /* 同时通过回调通知调试器客户端 */
  if (debugger->session.callback != NULL) {
    debugger->session.callback(DBG_EVENT_LOG, buf, (uint32_t)strlen(buf));
  }
}
