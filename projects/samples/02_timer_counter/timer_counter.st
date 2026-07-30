PROGRAM TimerCounter
VAR_INPUT
  StartBtn : BOOL;
  StopBtn : BOOL;
  SensorA : BOOL;
  SensorB : BOOL;
  ResetBtn : BOOL;
END_VAR
VAR_OUTPUT
  ConveyorMotor : BOOL;
  GreenLamp : BOOL;
  YellowLamp : BOOL;
  RedLamp : BOOL;
  Buzzer : BOOL;
END_VAR
VAR
  (* 手动延时接通定时器（替代 TON） *)
  ConveyorTimerEn : BOOL := FALSE;
  ConveyorTimerAcc : INT := 0;
  ConveyorTimerPreset : INT := 200;
  ConveyorTimerDone : BOOL := FALSE;

  (* 手动延时断开定时器（替代 TOF） *)
  StopTimerEn : BOOL := FALSE;
  StopTimerAcc : INT := 0;
  StopTimerPreset : INT := 300;
  StopTimerDone : BOOL := FALSE;

  (* 闪烁定时器 *)
  BlinkTimerEn : BOOL := FALSE;
  BlinkTimerAcc : INT := 0;
  BlinkTimerPreset : INT := 50;
  BlinkOut : BOOL := FALSE;

  (* 手动计数器（替代 CTU） *)
  ProductCount : INT := 0;
  CountPreset : INT := 100;
  CountDone : BOOL := FALSE;

  (* 辅助变量 *)
  PrevSensorA : BOOL := FALSE;
  SensorARise : BOOL := FALSE;

  (* 闪烁状态 *)
  BlinkState : BOOL := FALSE;
END_VAR

(* Rung 1: 传送带延时启动——按下启动按钮后延时 2 秒 *)
ConveyorTimerEn := StartBtn AND NOT StopBtn;
IF ConveyorTimerEn THEN
  ConveyorTimerAcc := ConveyorTimerAcc + 1;
  IF ConveyorTimerAcc >= ConveyorTimerPreset THEN
    ConveyorTimerDone := TRUE;
  END_IF
ELSE
  ConveyorTimerAcc := 0;
  ConveyorTimerDone := FALSE;
END_IF

IF ConveyorTimerDone AND NOT StopBtn THEN
  ConveyorMotor := TRUE;
ELSE
  ConveyorMotor := FALSE;
END_IF

(* Rung 2: 停止后延时断开 *)
StopTimerEn := NOT StopBtn AND ConveyorTimerDone;
IF StopTimerEn THEN
  StopTimerAcc := StopTimerAcc + 1;
  IF StopTimerAcc >= StopTimerPreset THEN
    StopTimerDone := TRUE;
  END_IF
ELSE
  StopTimerAcc := 0;
  StopTimerDone := FALSE;
END_IF

(* Rung 3: 闪烁指示 *)
BlinkTimerEn := ConveyorMotor;
IF BlinkTimerEn THEN
  BlinkTimerAcc := BlinkTimerAcc + 1;
  IF BlinkTimerAcc >= BlinkTimerPreset THEN
    BlinkState := NOT BlinkState;
    BlinkTimerAcc := 0;
  END_IF
ELSE
  BlinkTimerAcc := 0;
  BlinkState := FALSE;
END_IF

GreenLamp := ConveyorMotor AND BlinkState;

(* Rung 4: 产品计数——SensorA 上升沿 *)
SensorARise := SensorA AND NOT PrevSensorA;
PrevSensorA := SensorA;

IF SensorARise AND NOT ResetBtn THEN
  ProductCount := ProductCount + 1;
END_IF

IF ResetBtn THEN
  ProductCount := 0;
END_IF

CountDone := ProductCount >= CountPreset;

IF CountDone THEN
  YellowLamp := TRUE;
  Buzzer := TRUE;
ELSE
  YellowLamp := FALSE;
  Buzzer := FALSE;
END_IF

(* Rung 5: 报警——SensorB 触发红灯 *)
RedLamp := SensorB;

END_PROGRAM
