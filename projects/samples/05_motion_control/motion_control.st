PROGRAM MotionControl
VAR_INPUT
  StartBtn : BOOL;
  StopBtn : BOOL;
  HomeBtn : BOOL;
  JogForward : BOOL;
  JogReverse : BOOL;
  LimitPositive : BOOL;
  LimitNegative : BOOL;
  HomeSensor : BOOL;
  TargetPosition : INT := 5000;
  MoveVelocity : INT := 1000;
  Acceleration : INT := 500;
  Deceleration : INT := 500;
  EmergencyStop : BOOL;
END_VAR
VAR_OUTPUT
  PulseOut : BOOL;
  DirectionOut : BOOL;
  IsRunning : BOOL;
  IsHomed : BOOL;
  IsMoving : BOOL;
  IsAtTarget : BOOL;
  FaultAlarm : BOOL;
  CurrentPosition : INT;
END_VAR
VAR
  AxisFault : BOOL := FALSE;
  AxisEnabled : BOOL := FALSE;
  AxisBusy : BOOL := FALSE;
  AxisHomed : BOOL := FALSE;
  MoveMode : INT := 0;
  SetpointPosition : INT := 0;
  ActualPosition : INT := 0;
  PositionError : INT := 0;
  VelocityCmd : INT := 0;
  LimitFaultLatch : BOOL := FALSE;
  PulseActive : BOOL := FALSE;
  PulseDir : BOOL := FALSE;
  PulseAccum : INT := 0;
  PulseTarget : INT := 0;
END_VAR

(* Rung 1: 轴使能控制 *)
IF StartBtn AND NOT StopBtn AND NOT AxisFault THEN
  AxisEnabled := TRUE;
ELSIF StopBtn OR AxisFault THEN
  AxisEnabled := FALSE;
END_IF

IsRunning := AxisEnabled;

(* Rung 2: 限位保护和故障锁存 *)
IF LimitPositive OR LimitNegative THEN
  LimitFaultLatch := TRUE;
END_IF

IF HomeBtn AND NOT LimitPositive AND NOT LimitNegative THEN
  LimitFaultLatch := FALSE;
END_IF

AxisFault := LimitFaultLatch;
FaultAlarm := AxisFault;

(* Rung 3: 模式选择 *)
IF NOT AxisEnabled THEN
  MoveMode := 0;
ELSIF HomeBtn AND AxisEnabled THEN
  MoveMode := 1;
ELSIF LimitFaultLatch OR EmergencyStop THEN
  MoveMode := 4;
ELSIF JogForward OR JogReverse THEN
  MoveMode := 2;
ELSIF StartBtn AND AxisEnabled AND AxisHomed THEN
  MoveMode := 3;
ELSE
  MoveMode := 0;
END_IF

(* 模式执行（用 IF/ELSIF 替代 CASE） *)
IF MoveMode = 0 THEN
  (* IDLE *)
  VelocityCmd := 0;
  PulseActive := FALSE;
  AxisBusy := FALSE;

ELSIF MoveMode = 1 THEN
  (* HOME: 回原点 *)
  PulseDir := FALSE;
  PulseActive := TRUE;
  VelocityCmd := 200;
  IF HomeSensor THEN
    PulseActive := FALSE;
    AxisHomed := TRUE;
    ActualPosition := 0;
    MoveMode := 0;
  END_IF
  IF LimitNegative THEN
    PulseDir := TRUE;
  END_IF

ELSIF MoveMode = 2 THEN
  (* JOG: 点动 *)
  IF JogForward AND NOT LimitPositive THEN
    PulseDir := TRUE;
    PulseActive := TRUE;
    VelocityCmd := 500;
  ELSIF JogReverse AND NOT LimitNegative THEN
    PulseDir := FALSE;
    PulseActive := TRUE;
    VelocityCmd := 500;
  ELSE
    PulseActive := FALSE;
    VelocityCmd := 0;
  END_IF

ELSIF MoveMode = 3 THEN
  (* MOVE: 绝对定位 *)
  PulseDir := TargetPosition > ActualPosition;
  PulseTarget := TargetPosition;
  PulseActive := TRUE;
  VelocityCmd := MoveVelocity;
  AxisBusy := TRUE;
  IF ActualPosition >= TargetPosition - 10 AND ActualPosition <= TargetPosition + 10 THEN
    PulseActive := FALSE;
    AxisBusy := FALSE;
  END_IF

ELSIF MoveMode = 4 THEN
  (* STOP: 急停 *)
  PulseActive := FALSE;
  VelocityCmd := 0;
  AxisBusy := FALSE;

ELSE
  MoveMode := 0;
END_IF

(* Rung 4: 脉冲输出 *)
PulseOut := PulseActive;
IF PulseDir THEN
  DirectionOut := TRUE;
ELSE
  DirectionOut := FALSE;
END_IF

(* Rung 5: 位置模拟 *)
IF PulseActive AND PulseDir THEN
  ActualPosition := ActualPosition + 1;
ELSIF PulseActive AND NOT PulseDir THEN
  ActualPosition := ActualPosition - 1;
END_IF

CurrentPosition := ActualPosition;

(* Rung 6: 状态输出 *)
IsHomed := AxisHomed;
IsMoving := PulseActive;
IsAtTarget := NOT PulseActive AND AxisHomed AND MoveMode = 0;
PositionError := TargetPosition - ActualPosition;

END_PROGRAM
