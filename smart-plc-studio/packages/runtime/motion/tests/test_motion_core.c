#include "plc_motion.h"
#include "plc_planner.h"
#include "plc_interpolator.h"
#include "plc_axis.h"
#include "plc_group.h"
#include "plc_kinematics.h"
#include "plc_gcodeparser.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); testsPassed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); testsFailed++; } while(0)
#define ASSERT(cond, msg) do { \
  if (!(cond)) { FAIL(msg); return; } \
} while(0)

static float epsf(float a, float b, float eps)
{
  return fabsf(a - b) < eps;
}

/* ==================== G-Code 解析器测试 ==================== */

static void test_gcode_parse_g0()
{
  TEST("G0 解析");
  GCodeParserState state;
  plc_gcode_init(&state);

  GCodeBlock block;
  int ret = plc_gcode_parseLine(&state, "G0 X10 Y20 Z30", &block);
  ASSERT(ret == 0, "解析失败");
  ASSERT(block.gPresent == true, "G 未解析");
  ASSERT(block.gCode == 0, "G0 错误");
  ASSERT(block.isMotion == true, "非运动指令");
  ASSERT(block.axisPresent[0] == true && epsf(block.axisValue[0], 10, 1e-6), "X 错误");
  ASSERT(block.axisPresent[1] == true && epsf(block.axisValue[1], 20, 1e-6), "Y 错误");
  ASSERT(block.axisPresent[2] == true && epsf(block.axisValue[2], 30, 1e-6), "Z 错误");
  PASS();
}

static void test_gcode_parse_g1()
{
  TEST("G1 解析");
  GCodeParserState state;
  plc_gcode_init(&state);

  GCodeBlock block;
  int ret = plc_gcode_parseLine(&state, "G1 X100 F500", &block);
  ASSERT(ret == 0, "解析失败");
  ASSERT(block.gCode == 1, "G1 错误");
  ASSERT(block.fPresent && epsf(block.fNumber, 500, 1e-6), "F 错误");
  PASS();
}

static void test_gcode_parse_arc()
{
  TEST("圆弧 G2/G3 解析");
  GCodeParserState state;
  plc_gcode_init(&state);

  GCodeBlock block;
  int ret = plc_gcode_parseLine(&state, "G2 X50 Y50 I10 J0 F300", &block);
  ASSERT(ret == 0, "G2 解析失败");
  ASSERT(block.gCode == 2, "G2 错误");
  ASSERT(block.iPresent && epsf(block.iNumber, 10, 1e-6), "I 错误");
  ASSERT(block.jPresent && epsf(block.jNumber, 0, 1e-6), "J 错误");
  PASS();

  memset(&block, 0, sizeof(block));
  ret = plc_gcode_parseLine(&state, "G3 X0 Y0 I-10 J-10", &block);
  ASSERT(ret == 0, "G3 解析失败");
  ASSERT(block.gCode == 3, "G3 错误");
  PASS();
}

/* 用于 G-Code 执行测试的回调 */
typedef struct {
  int canonCount;
  CanonCommandData lastCmd;
} TestCanonCtx;

static void test_on_canon(const CanonCommandData *cmd, void *user)
{
  TestCanonCtx *ctx = (TestCanonCtx *)user;
  ctx->lastCmd = *cmd;
  ctx->canonCount++;
}

static void test_gcode_execute_g1()
{
  TEST("G1 执行 (规范命令)");
  GCodeParserState state;
  plc_gcode_init(&state);

  TestCanonCtx ctx;
  memset(&ctx, 0, sizeof(ctx));

  GCodeCallbacks cbs;
  cbs.onCanonCommand = test_on_canon;
  cbs.onComment = NULL;
  cbs.onError = NULL;

  int ret = plc_gcode_executeLine(&state, "G1 X50 Y25 F300", &cbs, &ctx);
  ASSERT(ret == 0, "G1 执行失败");
  ASSERT(ctx.canonCount == 1, "应产生 1 个规范命令");
  ASSERT(ctx.lastCmd.type == CANON_STRAIGHT_FEED, "类型应为 STRAIGHT_FEED");
  ASSERT(epsf(ctx.lastCmd.end[0], 50, 1e-6), "X 终点错误");
  ASSERT(epsf(ctx.lastCmd.end[1], 25, 1e-6), "Y 终点错误");
  PASS();
}

static void test_gcode_modal_groups()
{
  TEST("模态组保持");
  GCodeParserState state;
  plc_gcode_init(&state);

  GCodeBlock block;
  plc_gcode_parseLine(&state, "G1 X10 F300", &block);
  ASSERT(state.gModes[GC_MOTION] == 1, "G1 模态未保持");

  plc_gcode_parseLine(&state, "X20", &block);
  ASSERT(block.isMotion == true, "模态运动未传递");
  ASSERT(block.motionToBe == 1, "模态 G1 未继承");
  PASS();
}

static void test_gcode_mcodes()
{
  TEST("M 代码解析");
  GCodeParserState state;
  plc_gcode_init(&state);

  GCodeBlock block;
  plc_gcode_parseLine(&state, "M3 S1000", &block);
  ASSERT(block.mPresent && block.mCode == 3, "M3 错误");
  ASSERT(block.sPresent && epsf(block.sNumber, 1000, 1e-6), "S 错误");
  PASS();
}

/* ==================== 运动学测试 ==================== */

static void test_kinematics_identity()
{
  TEST("Identity 正运动学");
  Kinematics kins;
  KinematicsConfig cfg;
  cfg.type = KINEMATICS_IDENTITY;
  plc_kinematics_init(&kins, &cfg);

  float joint[6] = {10, 20, 30, 0, 0, 0};
  CartesianPose pose;
  plc_kinematics_forward(&kins, joint, &pose);
  ASSERT(epsf(pose.x, 10, 1e-6), "X 错误");
  ASSERT(epsf(pose.y, 20, 1e-6), "Y 错误");
  ASSERT(epsf(pose.z, 30, 1e-6), "Z 错误");
  PASS();

  TEST("Identity 逆运动学");
  CartesianPose pose2 = {15, 25, 35, 0, 0, 0};
  float joint2[6];
  plc_kinematics_inverse(&kins, &pose2, joint2);
  ASSERT(epsf(joint2[0], 15, 1e-6), "J0 错误");
  ASSERT(epsf(joint2[1], 25, 1e-6), "J1 错误");
  ASSERT(epsf(joint2[2], 35, 1e-6), "J2 错误");
  PASS();
}

static void test_kinematics_corexy()
{
  TEST("CoreXY 正运动学");
  Kinematics kins;
  KinematicsConfig cfg;
  cfg.type = KINEMATICS_CORE_XY;
  plc_kinematics_init(&kins, &cfg);

  /* CoreXY: X = (A+B)/2, Y = (A-B)/2 */
  float joint[6] = {20, 10, 0, 0, 0, 0};  /* A=20, B=10 */
  CartesianPose pose;
  plc_kinematics_forward(&kins, joint, &pose);
  ASSERT(epsf(pose.x, 15, 1e-6), "X=(A+B)/2 错误");   /* (20+10)/2 = 15 */
  ASSERT(epsf(pose.y, 5, 1e-6), "Y=(A-B)/2 错误");    /* (20-10)/2 = 5 */
  PASS();

  TEST("CoreXY 逆运动学");
  CartesianPose pose2 = {15, 5, 0, 0, 0, 0};
  float joint2[6];
  plc_kinematics_inverse(&kins, &pose2, joint2);
  ASSERT(epsf(joint2[0], 20, 1e-6), "A=X+Y 错误");    /* 15+5 = 20 */
  ASSERT(epsf(joint2[1], 10, 1e-6), "B=X-Y 错误");    /* 15-5 = 10 */
  PASS();
}

/* ==================== 规划器测试 ==================== */

static void test_planner_init_deinit()
{
  TEST("规划器初始化/销毁");
  Planner planner;
  PlannerConfig cfg;
  cfg.bufferSize = 16;
  cfg.defaultAcceleration = 1000;
  cfg.defaultJerk = 50000;
  cfg.maxVelocity = 100;
  cfg.junctionDeviation = 0.01f;
  cfg.termCond = TERM_COND_CONTINUOUS;
  cfg.enableLookAhead = true;

  int ret = plc_planner_init(&planner, &cfg);
  ASSERT(ret == 0, "初始化失败");
  ASSERT(planner.bufferSize == 16, "bufferSize 错误");
  ASSERT(plc_planner_isEmpty(&planner), "应为空");
  ASSERT(plc_planner_available(&planner) == 16, "槽位数错误");

  plc_planner_deinit(&planner);
  ASSERT(planner.buffer == NULL, "buffer 未释放");
  PASS();
}

static void test_planner_plan_linear()
{
  TEST("规划器 - 直线段");
  Planner planner;
  PlannerConfig cfg;
  cfg.bufferSize = 16;
  cfg.defaultAcceleration = 1000;
  cfg.defaultJerk = 50000;
  cfg.maxVelocity = 100;
  cfg.junctionDeviation = 0.01f;
  cfg.termCond = TERM_COND_CONTINUOUS;
  cfg.enableLookAhead = true;
  plc_planner_init(&planner, &cfg);

  float target[9] = {100, 0, 0, 0, 0, 0, 0, 0, 0};
  int ret = plc_planner_planLinear(&planner, target, 50, 0, 0);
  ASSERT(ret == 0, "planLinear 失败");
  ASSERT(plc_planner_queued(&planner) == 1, "段数错误");
  ASSERT(!plc_planner_isEmpty(&planner), "不应为空");

  PlannerSegment seg;
  ret = plc_planner_getNext(&planner, &seg);
  ASSERT(ret == 0, "getNext 失败");
  ASSERT(seg.type == SEGMENT_LINEAR, "类型错误");
  ASSERT(epsf(seg.length, 100, 1e-4), "长度应为 100");
  ASSERT(epsf(seg.target[0], 100, 1e-6), "X 目标错误");
  ASSERT(epsf(seg.plannedVelocity, 50, 1e-6), "规划速度错误");
  ASSERT(epsf(seg.direction[0], 1.0f, 1e-6), "X 方向错误");

  plc_planner_deinit(&planner);
  PASS();
}

static void test_planner_lookahead()
{
  TEST("规划器 - 前瞻 (2 段)");
  Planner planner;
  PlannerConfig cfg;
  cfg.bufferSize = 16;
  cfg.defaultAcceleration = 1000;
  cfg.defaultJerk = 50000;
  cfg.maxVelocity = 100;
  cfg.junctionDeviation = 0.1f;
  cfg.termCond = TERM_COND_CONTINUOUS;
  cfg.enableLookAhead = true;
  plc_planner_init(&planner, &cfg);

  float target1[9] = {100, 0, 0};
  float target2[9] = {100, 100, 0};

  plc_planner_planLinear(&planner, target1, 50, 0, 0);
  plc_planner_planLinear(&planner, target2, 50, 0, 0);
  ASSERT(plc_planner_queued(&planner) == 2, "应有两段");

  int ret = plc_planner_lookAhead(&planner);
  ASSERT(ret == 0, "前瞻失败");

  PlannerSegment seg1, seg2;
  plc_planner_getNext(&planner, &seg1);
  plc_planner_getNext(&planner, &seg2);

  /* 两段夹角 90°，拐角速度应小于规划速度 */
  ASSERT(seg1.junctionVelocity < seg1.plannedVelocity, "拐角速度应受限");
  ASSERT(seg1.exitVelocity <= seg1.junctionVelocity, "出口速度应 ≤ 拐角速度");
  ASSERT(seg2.entryVelocity <= seg1.exitVelocity + 0.001f, "入口速度 ≤ 出口速度");

  plc_planner_deinit(&planner);
  PASS();
}

static void test_planner_junction_velocity()
{
  TEST("规划器 - 拐角速度计算");
  /* 同向: cos=1 → junctionVelocity 应很大 */
  float dirA[3] = {1, 0, 0};
  float dirB[3] = {1, 0, 0};
  float v = plc_plan_junctionVelocity(dirA, dirB, 1000, 0.1f);
  ASSERT(v > 1e9f, "同向应无限制");

  /* 直角: cos=0 */
  float dirC[3] = {1, 0, 0};
  float dirD[3] = {0, 1, 0};
  v = plc_plan_junctionVelocity(dirC, dirD, 1000, 0.1f);
  ASSERT(v > 0 && v < 1000, "直角应有合理拐角速度");
  PASS();
}

static void test_planner_interpolate()
{
  TEST("规划器 - 段插补");
  PlannerSegment seg;
  memset(&seg, 0, sizeof(seg));
  seg.type = SEGMENT_LINEAR;
  seg.axisMask = 0x01; /* 仅 X 轴 */
  seg.length = 100;
  seg.direction[0] = 1.0f;
  seg.target[0] = 100;

  float pos[9];
  plc_planner_interpolate(&seg, 0.0f, pos);
  ASSERT(epsf(pos[0], 0, 1e-4), "s=0 应位于起点");

  plc_planner_interpolate(&seg, 0.5f, pos);
  ASSERT(epsf(pos[0], 50, 1e-4), "s=0.5 应在中点");

  plc_planner_interpolate(&seg, 1.0f, pos);
  ASSERT(epsf(pos[0], 100, 1e-4), "s=1 应在终点");
  PASS();
}

/* ==================== 插补器测试 ==================== */

static void test_interpolator_scurve()
{
  TEST("插补器 - S 曲线速度");
  PlannerSegment seg;
  memset(&seg, 0, sizeof(seg));
  seg.entryVelocity = 0;
  seg.cruiseVelocity = 50;
  seg.exitVelocity = 0;
  seg.accelerateUntil = 0.3f;
  seg.decelerateAfter = 0.7f;

  /* s=0: v_entry = 0 */
  float v0 = plc_interp_sCurveVelocity(&seg, 0.0f);
  ASSERT(epsf(v0, 0, 1e-4), "s=0 速度应为 0");

  /* s=0.5: 匀速段 */
  float vMid = plc_interp_sCurveVelocity(&seg, 0.5f);
  ASSERT(epsf(vMid, 50, 1e-4), "匀速段速度应为 50");

  /* s=1: v_exit = 0 */
  float vEnd = plc_interp_sCurveVelocity(&seg, 1.0f);
  ASSERT(epsf(vEnd, 0, 1e-4), "s=1 速度应为 0");
  PASS();
}

static void test_interpolator_step()
{
  TEST("插补器 - 单步执行");
  Interpolator interp;
  plc_interp_init(&interp, 0.001f);

  PlannerSegment seg;
  memset(&seg, 0, sizeof(seg));
  seg.type = SEGMENT_LINEAR;
  seg.axisMask = 0x01;
  seg.length = 10;
  seg.direction[0] = 1.0f;
  seg.target[0] = 10;
  seg.entryVelocity = 50;
  seg.cruiseVelocity = 50;
  seg.exitVelocity = 50;
  seg.accelerateUntil = 0;
  seg.decelerateAfter = 1.0f;
  seg.plannedVelocity = 50;

  plc_interp_loadSegment(&interp, &seg);
  plc_interp_start(&interp);

  float pos[9];
  uint8_t axisCount;
  int ret = plc_interp_step(&interp, pos, &axisCount);
  ASSERT(ret == 0, "第一步应返回 0 (进行中)");
  ASSERT(axisCount >= 1, "轴计数 >= 1");
  ASSERT(pos[0] > 0, "位置应 > 0");

  /* 运行到结束 */
  int steps = 0;
  while (ret == 0 && steps < 10000) {
    ret = plc_interp_step(&interp, pos, &axisCount);
    steps++;
  }
  ASSERT(ret == 1, "应返回 1 (段结束)");
  ASSERT(epsf(pos[0], 10, 1.0f), "终点应接近 10");  /* 允差 1mm */
  PASS();
}

/* ==================== 轴控制测试 ==================== */

static void test_axis_init()
{
  TEST("轴初始化");
  PlcAxis axis;
  MotorConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.drvType = MOTOR_DRV_STEPDIR;
  cfg.drv.stepdir.pulsePerMm = 80;
  cfg.drv.stepdir.maxPulseFreq = 100000;
  cfg.softLimitPos = 200;
  cfg.softLimitNeg = -200;
  cfg.followingErrorMax = 1.0f;

  int ret = plc_axis_init(&axis, 0, "X", &cfg);
  ASSERT(ret == 0, "初始化失败");
  ASSERT(strcmp(axis.name, "X") == 0, "轴名错误");
  ASSERT(axis.id == 0, "轴 ID 错误");
  ASSERT(axis.inPositionTolerance > 0, "到位精度应 > 0");
  PASS();
}

static void test_axis_move_and_limits()
{
  TEST("轴运动与限位");
  PlcAxis axis;
  MotorConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.drvType = MOTOR_DRV_STEPDIR;
  cfg.drv.stepdir.pulsePerMm = 80;
  cfg.drv.stepdir.maxPulseFreq = 100000;
  cfg.softLimitPos = 100;
  cfg.softLimitNeg = -100;

  plc_axis_init(&axis, 0, "X", &cfg);

  /* 未使能应失败 */
  axis.enabled = false;
  int ret = plc_axis_moveAbs(&axis, 50, 100, 1000);
  ASSERT(ret != 0, "未使能时应失败");

  /* 使能后应成功 */
  axis.enabled = true;
  ret = plc_axis_moveAbs(&axis, 50, 100, 1000);
  ASSERT(ret == 0, "使能后应成功");

  /* 超限应失败 */
  ret = plc_axis_moveAbs(&axis, 200, 100, 1000);
  ASSERT(ret != 0, "超限时应失败");
  PASS();
}

/* ==================== 轴组测试 ==================== */

static void test_group_coordinate_systems()
{
  TEST("轴组 - 坐标系转换");
  PlcGroup group;
  plc_group_init(&group, 0, "GROUP_0");

  /* 设置 G54 偏移 */
  float offsetVal = 50;
  plc_group_setCoordOffset(&group, COORD_G54, 0, offsetVal);
  ASSERT(epsf(plc_group_getCoordOffset(&group, COORD_G54, 0), 50, 1e-6), "G54 X 偏移错误");

  /* 机床 -> 工作坐标 */
  float machine[9] = {150, 0, 0};
  float work[9];
  plc_group_machineToWork(&group, machine, work);
  ASSERT(epsf(work[0], 100, 1e-6), "150 - 50 = 100");

  /* 工作 -> 机床 */
  float work2[9] = {100, 0, 0};
  float machine2[9];
  plc_group_workToMachine(&group, work2, machine2);
  ASSERT(epsf(machine2[0], 150, 1e-6), "100 + 50 = 150");
  PASS();
}

static void test_group_g92()
{
  TEST("轴组 - G92 偏移");
  PlcGroup group;
  plc_group_init(&group, 0, "GROUP_0");

  float offsets[9] = {10, 20, 0};
  plc_group_setG92(&group, offsets);

  float machine[9] = {110, 220, 0};
  float work[9];
  plc_group_machineToWork(&group, machine, work);
  ASSERT(epsf(work[0], 100, 1e-6), "110 - 10 = 100");
  ASSERT(epsf(work[1], 200, 1e-6), "220 - 20 = 200");

  plc_group_clearG92(&group);
  plc_group_machineToWork(&group, machine, work);
  ASSERT(epsf(work[0], 110, 1e-6), "清空 G92 后: 110");
  PASS();
}

/* ==================== CNC 系统集成测试 ==================== */

static void test_cnc_init()
{
  TEST("CNC 系统初始化");
  CncSystem cnc;
  CncConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.axisCount = 3;
  cfg.groupCount = 1;
  cfg.servoCycleSec = 0.001f;
  cfg.maxFeedRate = 6000;
  cfg.defaultAccel = 1000;
  cfg.defaultJerk = 50000;

  int ret = plc_cnc_init(&cnc, &cfg);
  ASSERT(ret == 0, "CNC 初始化失败");
  ASSERT(cnc.axisCount == 3, "轴数错误");
  ASSERT(cnc.groupCount == 1, "组数错误");
  ASSERT(cnc.status == CNC_STATUS_IDLE, "初始状态应为 IDLE");
  ASSERT(cnc.mode == CNC_MODE_IDLE, "初始模式应为 IDLE");

  plc_cnc_deinit(&cnc);
  PASS();
}

static void test_cnc_modes()
{
  TEST("CNC 模式/状态控制");
  CncSystem cnc;
  CncConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.axisCount = 3;
  cfg.groupCount = 1;
  cfg.servoCycleSec = 0.001f;
  cfg.maxFeedRate = 6000;
  cfg.defaultAccel = 1000;
  cfg.defaultJerk = 50000;
  plc_cnc_init(&cnc, &cfg);

  plc_cnc_setMode(&cnc, CNC_MODE_MANUAL);
  ASSERT(plc_cnc_getMode(&cnc) == CNC_MODE_MANUAL, "模式设置错误");

  plc_cnc_start(&cnc);
  ASSERT(cnc.status == CNC_STATUS_RUNNING, "启动后应 RUNNING");

  plc_cnc_pause(&cnc);
  ASSERT(cnc.status == CNC_STATUS_PAUSED, "暂停后应 PAUSED");

  plc_cnc_resume(&cnc);
  ASSERT(cnc.status == CNC_STATUS_RUNNING, "恢复后应 RUNNING");

  plc_cnc_stop(&cnc);
  ASSERT(cnc.status == CNC_STATUS_STOP, "停止后应 STOP");

  plc_cnc_estop(&cnc);
  ASSERT(cnc.status == CNC_STATUS_ESTOP, "急停后应 ESTOP");
  ASSERT(cnc.estop == true, "esotp 标志");

  plc_cnc_clearEstop(&cnc);
  ASSERT(cnc.status == CNC_STATUS_IDLE, "清除急停后应 IDLE");
  ASSERT(cnc.estop == false, "esotp 标志已清");

  plc_cnc_deinit(&cnc);
  PASS();
}

static void test_cnc_mdi()
{
  TEST("CNC MDI 命令");
  CncSystem cnc;
  CncConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.axisCount = 3;
  cfg.groupCount = 1;
  cfg.servoCycleSec = 0.001f;
  cfg.maxFeedRate = 6000;
  cfg.defaultAccel = 1000;
  cfg.defaultJerk = 50000;
  plc_cnc_init(&cnc, &cfg);

  /* 添加轴 */
  MotorConfig motorCfg;
  memset(&motorCfg, 0, sizeof(motorCfg));
  motorCfg.drvType = MOTOR_DRV_STEPDIR;
  motorCfg.drv.stepdir.pulsePerMm = 80;
  motorCfg.drv.stepdir.maxPulseFreq = 100000;

  plc_cnc_addAxis(&cnc, 0, "X", &motorCfg);
  plc_cnc_addAxis(&cnc, 1, "Y", &motorCfg);
  plc_cnc_addAxis(&cnc, 2, "Z", &motorCfg);

  /* MDI 执行 G1 */
  int ret = plc_cnc_mdi(&cnc, "G1 X100 Y50 F500");
  ASSERT(ret == 0, "MDI G1 执行失败");

  /* MDI 执行 G0 */
  ret = plc_cnc_mdi(&cnc, "G0 X0 Y0");
  ASSERT(ret == 0, "MDI G0 执行失败");

  plc_cnc_deinit(&cnc);
  PASS();
}

/* ==================== 主测试入口 ==================== */

int main()
{
  printf("========================================\n");
  printf("  PLC Motion 运动控制子系统测试\n");
  printf("========================================\n\n");

  /* G-Code 解析器 */
  printf("[G-Code 解析器]\n");
  test_gcode_parse_g0();
  test_gcode_parse_g1();
  test_gcode_parse_arc();
  test_gcode_execute_g1();
  test_gcode_modal_groups();
  test_gcode_mcodes();

  /* 运动学 */
  printf("\n[运动学]\n");
  test_kinematics_identity();
  test_kinematics_corexy();

  /* 规划器 */
  printf("\n[规划器]\n");
  test_planner_init_deinit();
  test_planner_plan_linear();
  test_planner_lookahead();
  test_planner_junction_velocity();
  test_planner_interpolate();

  /* 插补器 */
  printf("\n[插补器]\n");
  test_interpolator_scurve();
  test_interpolator_step();

  /* 轴控制 */
  printf("\n[轴控制]\n");
  test_axis_init();
  test_axis_move_and_limits();

  /* 轴组 */
  printf("\n[轴组]\n");
  test_group_coordinate_systems();
  test_group_g92();

  /* CNC 系统 */
  printf("\n[CNC 系统]\n");
  test_cnc_init();
  test_cnc_modes();
  test_cnc_mdi();

  /* 汇总 */
  printf("\n========================================\n");
  printf("  结果: %d 通过, %d 失败\n", testsPassed, testsFailed);
  printf("========================================\n");

  return testsFailed > 0 ? 1 : 0;
}
