#include "plc_generated.h"

/* ========== POU 全局变量定义 ========== */

/* Main */
plc_bool Main_StartBtn;
plc_bool Main_StopBtn;
plc_bool Main_LimitSwitch;
plc_bool Main_Motor;
plc_bool Main_Alarm;
plc_int  Main_RunCount;
plc_int  Main_TempValue;

/* Blink */
plc_bool Blink_Out;
plc_int  Blink_Count;
plc_int  Blink_Period;

/* Counter */
plc_bool Counter_Enable;
plc_bool Counter_Reset;
plc_int  Counter_Value;

/* Axis */
plc_bool Axis_X_LimitPos;
plc_bool Axis_X_LimitNeg;
plc_bool Axis_Y_LimitPos;
plc_bool Axis_Y_LimitNeg;
plc_bool Axis_Z_LimitPos;
plc_bool Axis_Z_LimitNeg;
plc_bool Axis_All_Homed;

/* System */
plc_bool Sys_Heartbeat;
plc_int  Sys_ErrorCode;

/* ========== generated_init ========== */
void generated_init(PlcVarTable* var_table, PlcIoConfig* io_config) {
  /* === Main POU === */
  plc_var_register(var_table, "Main.StartBtn",    VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "启动按钮 (PB0)");
  plc_var_register(var_table, "Main.StopBtn",     VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "停止按钮 (PB1)");
  plc_var_register(var_table, "Main.LimitSwitch", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "限位开关 (PB2)");
  plc_var_register(var_table, "Main.Motor",       VAR_TYPE_BOOL, VAR_ATTR_OUTPUT, sizeof(plc_bool), "电机运行 (PC0)");
  plc_var_register(var_table, "Main.Alarm",       VAR_TYPE_BOOL, VAR_ATTR_OUTPUT, sizeof(plc_bool), "报警输出 (PC1)");
  plc_var_register(var_table, "Main.RunCount",    VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "运行计数");
  plc_var_register(var_table, "Main.TempValue",   VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "临时值");

  /* === Blink POU === */
  plc_var_register(var_table, "Blink.Out",     VAR_TYPE_BOOL, VAR_ATTR_OUTPUT, sizeof(plc_bool), "闪烁输出 (PA5)");
  plc_var_register(var_table, "Blink.Count",   VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "闪烁计数");
  plc_var_register(var_table, "Blink.Period",  VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "闪烁周期");

  /* === Counter POU === */
  plc_var_register(var_table, "Counter.Enable", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "计数器使能 (PB3)");
  plc_var_register(var_table, "Counter.Reset",  VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "计数器复位 (PB3)");
  plc_var_register(var_table, "Counter.Value",  VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "计数器值");

  /* === Axis POU (轴状态监测) === */
  plc_var_register(var_table, "Axis.X.LimitPos", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "X+ 限位");
  plc_var_register(var_table, "Axis.X.LimitNeg", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "X- 限位");
  plc_var_register(var_table, "Axis.Y.LimitPos", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "Y+ 限位");
  plc_var_register(var_table, "Axis.Y.LimitNeg", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "Y- 限位");
  plc_var_register(var_table, "Axis.Z.LimitPos", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "Z+ 限位");
  plc_var_register(var_table, "Axis.Z.LimitNeg", VAR_TYPE_BOOL, VAR_ATTR_INPUT,  sizeof(plc_bool), "Z- 限位");
  plc_var_register(var_table, "Axis.AllHomed",   VAR_TYPE_BOOL, VAR_ATTR_OUTPUT, sizeof(plc_bool), "所有轴已回零");

  /* === System POU === */
  plc_var_register(var_table, "Sys.Heartbeat", VAR_TYPE_BOOL, VAR_ATTR_OUTPUT, sizeof(plc_bool), "系统心跳");
  plc_var_register(var_table, "Sys.ErrorCode", VAR_TYPE_INT,  VAR_ATTR_LOCAL,  sizeof(plc_int),  "错误码");

  /* === I/O 映射绑定 === */
  plc_io_register(io_config, IO_TYPE_DI, "StartBtn",   "Main.StartBtn",    0x10); /* PB0 */
  plc_io_register(io_config, IO_TYPE_DI, "StopBtn",    "Main.StopBtn",     0x11); /* PB1 */
  plc_io_register(io_config, IO_TYPE_DI, "LimitSw",    "Main.LimitSwitch", 0x12); /* PB2 */
  plc_io_register(io_config, IO_TYPE_DI, "CounterEn",  "Counter.Enable",   0x13); /* PB3 */
  plc_io_register(io_config, IO_TYPE_DO, "Motor",      "Main.Motor",       0x20); /* PC0 */
  plc_io_register(io_config, IO_TYPE_DO, "Alarm",      "Main.Alarm",       0x21); /* PC1 */
  plc_io_register(io_config, IO_TYPE_DO, "Blink",      "Blink.Out",        0x25); /* PA5 (LED) */
  plc_io_register(io_config, IO_TYPE_DI, "LimitXPos",  "Axis.X.LimitPos",  0x30);
  plc_io_register(io_config, IO_TYPE_DI, "LimitXNeg",  "Axis.X.LimitNeg",  0x31);
  plc_io_register(io_config, IO_TYPE_DI, "LimitYPos",  "Axis.Y.LimitPos",  0x32);
  plc_io_register(io_config, IO_TYPE_DI, "LimitYNeg",  "Axis.Y.LimitNeg",  0x33);
  plc_io_register(io_config, IO_TYPE_DI, "LimitZPos",  "Axis.Z.LimitPos",  0x34);
  plc_io_register(io_config, IO_TYPE_DI, "LimitZNeg",  "Axis.Z.LimitNeg",  0x35);

  /* 绑定所有 I/O 通道到变量 */
  for (uint16_t i = 0; i < io_config->channel_count; i++) {
    plc_io_bind(io_config, i, var_table);
  }
}

