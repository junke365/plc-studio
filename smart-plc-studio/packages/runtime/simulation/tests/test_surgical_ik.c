#include "../include/plc_surgical.h"
#include <stdio.h>
#include <math.h>

#define ASSERT(cond, msg) do { \
  if (!(cond)) { fprintf(stderr, "失败: %s\n", msg); return 1; } \
  else { printf("通过: %s\n", msg); } \
} while(0)

int test_mtm_ik() {
  SurgicalRobot robot;
  plc_surgical_init(&robot, SURGICAL_MTM);

  // 测试1: 零位附近的正逆解一致性
  float qNeutral[7] = {0, 0, 0, 0, 0, 0, 0};
  plcVec3 pos;
  plcQuat orient;
  plc_surgical_forward(&robot, qNeutral, &pos, &orient);

  float qResult[7] = {0};
  int nSoln = plc_surgical_inverse(&robot, pos, orient, qResult, 1);
  ASSERT(nSoln == 1, "MTM 零位 IK 应有解");

  // 检查关节角接近零
  float err = 0;
  for (int i = 0; i < 7; i++) err += fabsf(qResult[i] - qNeutral[i]);
  ASSERT(err < 0.01f, "MTM 零位 IK 误差 < 0.01 rad");

  // 测试2: 随机位置
  plcVec3 target = {200, 100, 300};
  plcQuat tOrient = plc_quat_identity();
  nSoln = plc_surgical_inverse(&robot, target, tOrient, qResult, 1);
  ASSERT(nSoln >= 1, "MTM 目标位置 IK 应有解");

  // 验证 FK( IK(target) ) ≈ target
  plcVec3 fkPos;
  plcQuat fkOrient;
  plc_surgical_forward(&robot, qResult, &fkPos, &fkOrient);
  float dist = sqrtf(powf(fkPos.x - target.x, 2) + powf(fkPos.y - target.y, 2) + powf(fkPos.z - target.z, 2));
  ASSERT(dist < 1.0f, "MTM IK→FK 位置误差 < 1mm");

  // 测试3: 奇异位置检查
  float sing = plc_surgical_singularity(&robot, qNeutral);
  ASSERT(sing > 0.01f, "MTM 零位非奇异");

  // 测试4: 工作空间检查
  ASSERT(plc_surgical_inWorkspace(&robot, plc_vec3(0, 300, 500)), "MTM 正常位置在工作空间内");
  ASSERT(!plc_surgical_inWorkspace(&robot, plc_vec3(9999, 0, 0)), "MTM 远处位置不在工作空间内");

  return 0;
}

int test_psm_ik() {
  SurgicalRobot robot;
  plc_surgical_init(&robot, SURGICAL_PSM);

  // 测试 PSM：插入方向
  float qHome[7] = {0, 0.5f, 0.1f, 0, 0, 0, 0};
  plcVec3 pos;
  plcQuat orient;
  plc_surgical_forward(&robot, qHome, &pos, &orient);

  float qResult[7] = {0};
  int nSoln = plc_surgical_inverse(&robot, pos, orient, qResult, 1);
  ASSERT(nSoln >= 1, "PSM IK 应有解");

  float err = 0;
  for (int i = 0; i < 7; i++) err += fabsf(qResult[i] - qHome[i]);
  ASSERT(err < 0.1f, "PSM IK→FK→IK 闭环误差 < 0.1 rad");

  return 0;
}

int main() {
  int failed = 0;
  failed += test_mtm_ik();
  failed += test_psm_ik();
  printf("\n%s: %d 个测试失败\n", failed ? "失败" : "全部通过", failed);
  return failed;
}
