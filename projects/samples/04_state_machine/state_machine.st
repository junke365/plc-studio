PROGRAM StateMachine
VAR_INPUT
  StartBtn : BOOL;
  StopBtn : BOOL;
  SensorA : BOOL;
  SensorB : BOOL;
  SensorC : BOOL;
  EmergencyStop : BOOL;
END_VAR
VAR_OUTPUT
  ValveA : BOOL;
  ValveB : BOOL;
  ValveC : BOOL;
  ConveyorFwd : BOOL;
  ConveyorRev : BOOL;
  ProcessBusy : BOOL;
  ProcessDone : BOOL;
  FaultLamp : BOOL;
END_VAR
VAR
  (* 状态编码（使用数字常量） *)
  Step : INT := 0;

  (* 手动延时定时器 *)
  FillTimerAcc : INT := 0;
  FillTimerPreset : INT := 500;
  FillTimerDone : BOOL := FALSE;

  HeatTimerAcc : INT := 0;
  HeatTimerPreset : INT := 1000;
  HeatTimerDone : BOOL := FALSE;

  ProcessTimerAcc : INT := 0;
  ProcessTimerPreset : INT := 800;
  ProcessTimerDone : BOOL := FALSE;

  DischargeTimerAcc : INT := 0;
  DischargeTimerPreset : INT := 400;
  DischargeTimerDone : BOOL := FALSE;
END_VAR

(* 主状态机切换（用 IF/ELSIF 替代 CASE，兼容编译器限制） *)
IF Step = 0 THEN
  (* 空闲状态 *)
  ValveA := FALSE;
  ValveB := FALSE;
  ValveC := FALSE;
  ConveyorFwd := FALSE;
  ConveyorRev := FALSE;
  ProcessBusy := FALSE;
  ProcessDone := FALSE;
  IF StartBtn AND NOT EmergencyStop THEN
    Step := 10;
  END_IF

ELSIF Step = 10 THEN
  (* 填充阶段：打开 ValveA，等待 SensorA 或超时 *)
  ValveA := TRUE;
  FillTimerAcc := FillTimerAcc + 1;
  FillTimerDone := FillTimerAcc >= FillTimerPreset;
  IF SensorA OR FillTimerDone THEN
    ValveA := FALSE;
    FillTimerAcc := 0;
    Step := 20;
  END_IF

ELSIF Step = 20 THEN
  (* 加热阶段：打开 ValveB，等待 SensorB 或超时 *)
  ValveB := TRUE;
  HeatTimerAcc := HeatTimerAcc + 1;
  HeatTimerDone := HeatTimerAcc >= HeatTimerPreset;
  IF SensorB OR HeatTimerDone THEN
    ValveB := FALSE;
    HeatTimerAcc := 0;
    Step := 30;
  END_IF

ELSIF Step = 30 THEN
  (* 处理阶段：启动传送带，等待定时 *)
  ConveyorFwd := TRUE;
  ProcessTimerAcc := ProcessTimerAcc + 1;
  ProcessTimerDone := ProcessTimerAcc >= ProcessTimerPreset;
  IF ProcessTimerDone THEN
    ConveyorFwd := FALSE;
    ProcessTimerAcc := 0;
    Step := 40;
  END_IF

ELSIF Step = 40 THEN
  (* 排放阶段：打开 ValveC 反转传送带排出 *)
  ValveC := TRUE;
  ConveyorRev := TRUE;
  DischargeTimerAcc := DischargeTimerAcc + 1;
  DischargeTimerDone := DischargeTimerAcc >= DischargeTimerPreset;
  IF SensorC OR DischargeTimerDone THEN
    ValveC := FALSE;
    ConveyorRev := FALSE;
    DischargeTimerAcc := 0;
    Step := 50;
  END_IF

ELSIF Step = 50 THEN
  (* 完成：设置完成标志，等待确认 *)
  ProcessDone := TRUE;
  ProcessBusy := FALSE;
  IF StartBtn THEN
    Step := 0;
  END_IF

ELSIF Step = 99 THEN
  (* 故障处理 *)
  FaultLamp := TRUE;
  ValveA := FALSE;
  ValveB := FALSE;
  ValveC := FALSE;
  ConveyorFwd := FALSE;
  ConveyorRev := FALSE;
  ProcessBusy := FALSE;
  IF NOT EmergencyStop AND StartBtn THEN
    FaultLamp := FALSE;
    Step := 0;
  END_IF

ELSE
  Step := 0;
END_IF

(* 紧急停止覆盖所有状态 *)
IF EmergencyStop THEN
  Step := 99;
END_IF

(* 运行时指示 *)
IF Step >= 10 AND Step <= 50 THEN
  ProcessBusy := TRUE;
ELSE
  ProcessBusy := FALSE;
END_IF

(* 故障保护 *)
IF Step = 99 THEN
  ValveA := FALSE;
  ValveB := FALSE;
  ValveC := FALSE;
  ConveyorFwd := FALSE;
  ConveyorRev := FALSE;
END_IF

END_PROGRAM
