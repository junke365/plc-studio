/**
 * plc_debug.h - 调试模块
 *
 * 提供运行时调试功能：
 * - 断点支持
 * - 变量监控
 * - 单步执行
 * - 日志输出
 */

#ifndef PLC_DEBUG_H
#define PLC_DEBUG_H

#include "plc_platform.h"
#include "plc_var.h"

/* 调试事件类型 */
typedef enum {
  DBG_EVENT_BREAKPOINT_HIT,
  DBG_EVENT_STEP_COMPLETE,
  DBG_EVENT_VARIABLE_CHANGE,
  DBG_EVENT_TASK_SWITCH,
  DBG_EVENT_ERROR,
  DBG_EVENT_LOG,
} DebugEventType;

/* 调试事件回调 */
typedef void (*DebugCallback)(DebugEventType event, const void* data, uint32_t size);

/* 调试会话 */
typedef struct {
  bool           active;
  bool           stepping;
  uint8_t        current_pou;
  uint32_t       current_line;
  DebugCallback  callback;
  void*          callback_ctx;
} PlcDebugSession;

/* 断点 */
typedef struct {
  uint8_t        pou_id;
  uint32_t       line;
  bool           enabled;
  uint32_t       hit_count;
} PlcBreakpoint;

#define PLC_MAX_BREAKPOINTS  32

/* 调试器 */
typedef struct {
  PlcDebugSession  session;
  PlcBreakpoint    breakpoints[PLC_MAX_BREAKPOINTS];
  uint8_t          breakpoint_count;
  uint32_t         total_breaks;
} PlcDebugger;

/* ========== 接口函数 ========== */

/**
 * 初始化调试器
 */
void plc_debug_init(PlcDebugger* debugger);

/**
 * 启动调试会话
 */
void plc_debug_start(PlcDebugger* debugger, DebugCallback callback, void* ctx);

/**
 * 停止调试会话
 */
void plc_debug_stop(PlcDebugger* debugger);

/**
 * 添加断点
 * @return 断点索引，-1=失败
 */
int plc_debug_add_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line);

/**
 * 删除断点
 */
void plc_debug_remove_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line);

/**
 * 启用/禁用断点
 */
void plc_debug_enable_breakpoint(PlcDebugger* debugger, uint8_t index, bool enabled);

/**
 * 检查是否命中断点
 * @return true=命中
 */
bool plc_debug_check_breakpoint(PlcDebugger* debugger, uint8_t pou_id, uint32_t line);

/**
 * 单步执行
 */
void plc_debug_step(PlcDebugger* debugger);

/**
 * 继续执行
 */
void plc_debug_continue(PlcDebugger* debugger);

/**
 * 暂停执行
 */
void plc_debug_pause(PlcDebugger* debugger);

/**
 * 发送变量更新事件
 */
void plc_debug_notify_variable(PlcDebugger* debugger, const PlcVariable* var);

/**
 * 发送日志消息
 */
void plc_debug_log(PlcDebugger* debugger, uint8_t level, const char* fmt, ...);

#endif /* PLC_DEBUG_H */
