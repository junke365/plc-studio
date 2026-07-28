PROGRAM main
VAR
  (* 计数器 *)
  counter : INT := 0;
  (* 定时器 *)
  timerOn : TON;
  (* 输出 *)
  outputVal : BOOL := FALSE;
  (* 模拟量 *)
  analogIn : REAL := 0.0;
  analogOut : REAL := 0.0;
END_VAR

(* 主循环: 每秒翻转输出 *)
timerOn(IN := TRUE, PT := T#1s);
IF timerOn.Q THEN
  outputVal := NOT outputVal;
  counter := counter + 1;
  timerOn(IN := FALSE);
END_IF

(* 模拟量处理 *)
analogOut := analogIn * 0.5 + 2.5;

END_PROGRAM
