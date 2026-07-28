PROGRAM motionControl
VAR
  (* 轴控制 *)
  axisX : AXIS_REF;
  axisY : AXIS_REF;
  axisZ : AXIS_REF;
  axisR : AXIS_REF;
  
  (* 运动命令 *)
  moveCmd : BOOL := FALSE;
  targetX : LREAL := 100.0;
  targetY : LREAL := 50.0;
  targetZ : LREAL := 0.0;
  targetR : LREAL := 45.0;
  
  (* 状态 *)
  isMoving : BOOL := FALSE;
  currentX : LREAL;
  currentY : LREAL;
  currentZ : LREAL;
  
  (* 视觉反馈 *)
  visionOffsetX : LREAL := 0.0;
  visionOffsetY : LREAL := 0.0;
  markerDetected : BOOL := FALSE;
  
  (* 无人机控制 *)
  droneRoll : LREAL := 0.0;
  dronePitch : LREAL := 0.0;
  droneYaw : LREAL := 0.0;
  droneThrust : LREAL := 0.0;
END_VAR

(* 主运动控制循环 *)
IF moveCmd AND NOT isMoving THEN
  axisX.Velocity := 100.0;
  axisX.Position := targetX + visionOffsetX;
  axisY.Position := targetY + visionOffsetY;
  axisZ.Position := targetZ;
  axisR.Position := targetR;
  MC_Power(axisX, TRUE);
  MC_MoveAbsolute(axisX, targetX + visionOffsetX, 100.0);
  isMoving := TRUE;
END_IF

(* 视觉反馈补偿 *)
IF markerDetected THEN
  targetX := targetX + visionOffsetX * 0.1;
  targetY := targetY + visionOffsetY * 0.1;
END_IF

(* 无人机姿态控制 *)
droneThrust := 0.5;
droneRoll := visionOffsetY * 0.01;
dronePitch := visionOffsetX * -0.01;

END_PROGRAM
