PROGRAM BasicLogic
VAR_INPUT
  StartBtn : BOOL;
  StopBtn : BOOL;
  ForwardLimit : BOOL;
  ReverseLimit : BOOL;
  EmergencyStop : BOOL;
  Overload : BOOL;
END_VAR
VAR_OUTPUT
  MotorForward : BOOL;
  MotorReverse : BOOL;
  Running : BOOL;
  AlarmLamp : BOOL;
END_VAR
VAR
  RunLatch : BOOL := FALSE;
  FaultLatch : BOOL := FALSE;
  StartRise : BOOL := FALSE;
  StopRise : BOOL := FALSE;
  LastStart : BOOL := FALSE;
  LastStop : BOOL := FALSE;
END_VAR

(* 启动/停止自锁：按下 StartBtn 启动，按下 StopBtn 或故障时停止 *)
IF StartBtn AND NOT StopBtn AND NOT FaultLatch THEN
  RunLatch := TRUE;
ELSIF StopBtn OR FaultLatch THEN
  RunLatch := FALSE;
END_IF

Running := RunLatch AND NOT EmergencyStop;

(* 正反转互锁：同时只允许一个方向，且限位开关保护 *)
IF Running AND NOT MotorReverse AND NOT ForwardLimit THEN
  MotorForward := TRUE;
ELSE
  MotorForward := FALSE;
END_IF

IF Running AND NOT MotorForward AND NOT ReverseLimit THEN
  MotorReverse := TRUE;
ELSE
  MotorReverse := FALSE;
END_IF

(* 故障检测：过载或急停 *)
IF Overload OR EmergencyStop THEN
  FaultLatch := TRUE;
END_IF

IF NOT Overload AND NOT EmergencyStop AND StartBtn THEN
  FaultLatch := FALSE;
END_IF

AlarmLamp := FaultLatch OR EmergencyStop;

(* 上升沿检测 *)
StartRise := StartBtn AND NOT LastStart;
StopRise := StopBtn AND NOT LastStop;
LastStart := StartBtn;
LastStop := StopBtn;

END_PROGRAM
