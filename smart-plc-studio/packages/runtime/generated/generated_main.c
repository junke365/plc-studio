#include "plc_generated.h"

/* ========== Main POU ==========
 * 电机自锁控制 + 运行计数 + 报警
 */
static void Main_body(void)
{
  /* 自锁电路: 启动(边沿)保持, 停止或限位断开 */
  if (Main_StartBtn && !Main_StopBtn) {
    Main_Motor = 1;
  } else if (Main_StopBtn || Main_LimitSwitch) {
    Main_Motor = 0;
  }

  /* 运行计数 */
  if (Main_Motor) {
    Main_RunCount = Main_RunCount + 1;
  }

  /* 报警阈值: 超过 1000 个周期 */
  if (Main_RunCount > 1000) {
    Main_Alarm = 1;
  } else {
    Main_Alarm = 0;
  }

  Main_TempValue = Main_RunCount * 2;
}

/* ========== Blink POU ==========
 * 500ms 闪烁输出（基于 Main_RunCount 分频）
 */
static void Blink_body(void)
{
  Blink_Period = 500; /* 500 个周期的闪烁周期 */

  if (Main_Motor) {
    Blink_Count = Blink_Count + 1;
    if (Blink_Count >= Blink_Period * 2) {
      Blink_Count = 0;
    }
    Blink_Out = (Blink_Count < Blink_Period) ? 1 : 0;
  } else {
    Blink_Out = 0;
    Blink_Count = 0;
  }
}

/* ========== Counter POU ==========
 * 高速计数器，由 Main_RunCount 触发
 */
static void Counter_body(void)
{
  if (Counter_Reset) {
    Counter_Value = 0;
  } else if (Counter_Enable) {
    /* 每 10 个 PLC 周期计 1 */
    if (Main_RunCount % 10 == 0) {
      Counter_Value = Counter_Value + 1;
    }
  }
}

/* ========== Axis POU ==========
 * 轴状态监测：检查限位开关状态
 */
static void Axis_body(void)
{
  /* 限位开关触发 → 停止电机 + 设置错误码 */
  if (Axis_X_LimitPos || Axis_X_LimitNeg ||
      Axis_Y_LimitPos || Axis_Y_LimitNeg ||
      Axis_Z_LimitPos || Axis_Z_LimitNeg) {
    Main_Motor = 0;
    Sys_ErrorCode = 100; /* 限位触发 */
  }

  /* 所有轴已回零（模拟条件：无限位触发 + 电机运行） */
  Axis_All_Homed = !Axis_X_LimitPos && !Axis_X_LimitNeg &&
                   !Axis_Y_LimitPos && !Axis_Y_LimitNeg &&
                   !Axis_Z_LimitPos && !Axis_Z_LimitNeg &&
                   Main_Motor;
}

/* ========== System POU ==========
 * 系统状态管理
 */
static void System_body(void)
{
  /* 心跳：每 500 个周期翻转一次 */
  Sys_Heartbeat = (Main_RunCount / 500) % 2;

  /* 错误码保持 */
  if (Sys_ErrorCode != 0 && !Main_Alarm) {
    Sys_ErrorCode = 0; /* 报警消除后清除错误 */
  }
}

/* ========== generated_main ========== */
void generated_main(void)
{
  Main_body();
  Blink_body();
  Counter_body();
  Axis_body();
  System_body();
}

uint32_t generated_pou_count(void)
{
  return 5;
}

const char* generated_pou_name(uint32_t index)
{
  switch (index) {
    case 0: return "Main";
    case 1: return "Blink";
    case 2: return "Counter";
    case 3: return "Axis";
    case 4: return "System";
    default: return "";
  }
}
