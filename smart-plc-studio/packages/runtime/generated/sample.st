PROGRAM Main
  VAR_INPUT
    StartBtn : BOOL;
    StopBtn : BOOL;
    LimitSwitch : BOOL;
  END_VAR
  VAR_OUTPUT
    Motor : BOOL;
    Alarm : BOOL;
  END_VAR
  VAR
    RunCount : INT;
    TempValue : INT;
  END_VAR

  IF StartBtn AND NOT StopBtn THEN
    Motor := TRUE;
  ELSIF StopBtn OR LimitSwitch THEN
    Motor := FALSE;
  END_IF

  IF Motor THEN
    RunCount := RunCount + 1;
  END_IF

  IF RunCount > 1000 THEN
    Alarm := TRUE;
  ELSE
    Alarm := FALSE;
  END_IF

  TempValue := RunCount * 2;
END_PROGRAM
