#ifndef PLC_GENERATED_H
#define PLC_GENERATED_H

#include "plc_runtime.h"

/* ===== Main POU ===== */
extern plc_bool Main_StartBtn;
extern plc_bool Main_StopBtn;
extern plc_bool Main_LimitSwitch;
extern plc_bool Main_Motor;
extern plc_bool Main_Alarm;
extern plc_int  Main_RunCount;
extern plc_int  Main_TempValue;

/* ===== Blink POU (500ms 闪烁) ===== */
extern plc_bool Blink_Out;
extern plc_int  Blink_Count;
extern plc_int  Blink_Period;    /* 闪烁周期计数 */

/* ===== Counter POU (高速计数) ===== */
extern plc_bool Counter_Enable;
extern plc_bool Counter_Reset;
extern plc_int  Counter_Value;

/* ===== Axis POU (轴状态监测) ===== */
extern plc_bool Axis_X_LimitPos;
extern plc_bool Axis_X_LimitNeg;
extern plc_bool Axis_Y_LimitPos;
extern plc_bool Axis_Y_LimitNeg;
extern plc_bool Axis_Z_LimitPos;
extern plc_bool Axis_Z_LimitNeg;
extern plc_bool Axis_All_Homed;

/* ===== 系统状态 ===== */
extern plc_bool Sys_Heartbeat;
extern plc_int  Sys_ErrorCode;

/* 生成代码接口 */
void generated_init(PlcVarTable* var_table, PlcIoConfig* io_config);
void generated_main(void);
uint32_t generated_pou_count(void);
const char* generated_pou_name(uint32_t index);

#endif /* PLC_GENERATED_H */
