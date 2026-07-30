PROGRAM AnalogProcessing
VAR_INPUT
  (* 模拟量输入（0-27648 对应 0-10V） *)
  AiChannel1 : INT;
  AiChannel2 : INT;
  AiChannel3 : INT;
  (* 数字量输入 *)
  EnableScaling : BOOL;
  ResetAlarm : BOOL;
END_VAR
VAR_OUTPUT
  (* 工程值输出 *)
  Temperature : REAL;
  Pressure : REAL;
  FlowRate : REAL;
  (* 报警输出 *)
  TempHighAlarm : BOOL;
  TempLowAlarm : BOOL;
  PressHighAlarm : BOOL;
  PressLowAlarm : BOOL;
  FlowLowAlarm : BOOL;
  SystemFault : BOOL;
  (* 模拟量输出（0-27648 对应 0-10V） *)
  AoChannel1 : INT;
END_VAR
VAR
  (* 缩放参数 *)
  TempScaleMin : REAL := 0.0;
  TempScaleMax : REAL := 150.0;
  PressScaleMin : REAL := 0.0;
  PressScaleMax : REAL := 10.0;
  FlowScaleMin : REAL := 0.0;
  FlowScaleMax : REAL := 100.0;
  (* 报警阈值 *)
  TempHighLimit : REAL := 130.0;
  TempLowLimit : REAL := 10.0;
  PressHighLimit : REAL := 8.0;
  PressLowLimit : REAL := 0.5;
  FlowLowLimit : REAL := 5.0;
  (* 滤波 *)
  TempFiltered : REAL := 0.0;
  TempPrev : REAL := 0.0;
  PressFiltered : REAL := 0.0;
  PressPrev : REAL := 0.0;
  FilterAlpha : REAL := 0.3;
  (* 报警锁存 *)
  TempHighLatch : BOOL := FALSE;
  TempLowLatch : BOOL := FALSE;
  PressHighLatch : BOOL := FALSE;
  PressLowLatch : BOOL := FALSE;
  FlowLowLatch : BOOL := FALSE;
  (* 模拟量输出 *)
  AoOutputRaw : INT := 0;
END_VAR

(* Rung 1: 模拟量缩放 — 将 0-27648 线性映射到工程值 *)
IF EnableScaling THEN
  Temperature := INT_TO_REAL(AiChannel1) / 27648.0 * (TempScaleMax - TempScaleMin) + TempScaleMin;
  Pressure := INT_TO_REAL(AiChannel2) / 27648.0 * (PressScaleMax - PressScaleMin) + PressScaleMin;
  FlowRate := INT_TO_REAL(AiChannel3) / 27648.0 * (FlowScaleMax - FlowScaleMin) + FlowScaleMin;
ELSE
  Temperature := 0.0;
  Pressure := 0.0;
  FlowRate := 0.0;
END_IF

(* Rung 2: 一阶低通滤波 — 抑制噪声 *)
IF EnableScaling THEN
  TempFiltered := FilterAlpha * Temperature + (1.0 - FilterAlpha) * TempPrev;
  TempPrev := TempFiltered;
  PressFiltered := FilterAlpha * Pressure + (1.0 - FilterAlpha) * PressPrev;
  PressPrev := PressFiltered;
ELSE
  TempFiltered := 0.0;
  PressFiltered := 0.0;
END_IF

(* Rung 3: 超限报警 — 带滞后比较 *)
IF TempFiltered > TempHighLimit THEN
  TempHighLatch := TRUE;
ELSIF TempFiltered < TempHighLimit - 5.0 THEN
  TempHighLatch := FALSE;
END_IF
TempHighAlarm := TempHighLatch;

IF TempFiltered < TempLowLimit THEN
  TempLowLatch := TRUE;
ELSIF TempFiltered > TempLowLimit + 2.0 THEN
  TempLowLatch := FALSE;
END_IF
TempLowAlarm := TempLowLatch;

IF PressFiltered > PressHighLimit THEN
  PressHighLatch := TRUE;
ELSIF PressFiltered < PressHighLimit - 0.5 THEN
  PressHighLatch := FALSE;
END_IF
PressHighAlarm := PressHighLatch;

IF PressFiltered < PressLowLimit THEN
  PressLowLatch := TRUE;
ELSIF PressFiltered > PressLowLimit + 0.2 THEN
  PressLowLatch := FALSE;
END_IF
PressLowAlarm := PressLowLatch;

IF FlowRate < FlowLowLimit THEN
  FlowLowLatch := TRUE;
ELSIF FlowRate > FlowLowLimit + 2.0 THEN
  FlowLowLatch := FALSE;
END_IF
FlowLowAlarm := FlowLowLatch;

(* Rung 4: 系统故障 — 任一报警激活 *)
SystemFault := TempHighAlarm OR TempLowAlarm OR PressHighAlarm
               OR PressLowAlarm OR FlowLowAlarm;

(* Rung 5: 报警复位 *)
IF ResetAlarm THEN
  TempHighLatch := FALSE;
  TempLowLatch := FALSE;
  PressHighLatch := FALSE;
  PressLowLatch := FALSE;
  FlowLowLatch := FALSE;
END_IF

(* Rung 6: 模拟量输出 — 将温度映射到 0-10V 输出 *)
AoOutputRaw := REAL_TO_INT(Temperature / TempScaleMax * 27648.0);
IF AoOutputRaw > 27648 THEN
  AoOutputRaw := 27648;
END_IF
IF AoOutputRaw < 0 THEN
  AoOutputRaw := 0;
END_IF
AoChannel1 := AoOutputRaw;

END_PROGRAM
