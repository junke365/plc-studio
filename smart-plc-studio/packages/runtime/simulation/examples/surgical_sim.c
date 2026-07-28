#include "plc_simulation.h"
#include "plc_surgical.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int main(void)
{
  SimConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.dt = 0.001f;
  cfg.gravity = 9.81f;
  cfg.solverIterations = 10;
  cfg.enableCollision = true;
  cfg.enableSoftBody = true;
  cfg.enableMultibody = true;

  SimulationSystem sim;
  int ret = plc_sim_init(&sim, &cfg);
  if (ret != SIM_OK) {
    printf("仿真初始化失败: %d\n", ret);
    return -1;
  }
  printf("仿真系统初始化成功\n");
  printf("  步长: %.4f s\n", sim.config.dt);
  printf("  重力: %.2f m/s²\n", sim.config.gravity);

  /* 创建 MTM 手术机器人 */
  SurgicalRobot mtm;
  ret = plc_surgical_init(&mtm, SURGICAL_MTM);
  if (ret != 0) {
    printf("MTM 初始化失败\n");
    return -1;
  }
  plc_sim_addSurgicalRobot(&sim, &mtm);
  printf("添加 MTM (7-DOF) 手术机器人\n");

  /* 创建 PSM 手术机器人 */
  SurgicalRobot psm;
  ret = plc_surgical_init(&psm, SURGICAL_PSM);
  if (ret != 0) {
    printf("PSM 初始化失败\n");
    return -1;
  }
  plc_sim_addSurgicalRobot(&sim, &psm);
  printf("添加 PSM (7-DOF) 手术机器人\n");

  /* 创建软体 (模拟组织) */
  plcSoftBody tissue;
  plc_soft_init(&tissue, SOFT_MASS_SPRING, 100.0f);
  plcVec3 center = {0, -0.2f, 0.3f};
  plcVec3 size = {0.1f, 0.03f, 0.08f};
  plc_soft_createBox(&tissue, center, size, 4, 2, 3, SOFT_MASS_SPRING, 100.0f);
  plc_sim_addSoftBody(&sim, &tissue);
  printf("添加软组织软体 (%d 节点, %d 弹簧)\n", tissue.nodeCount, tissue.springCount);

  /* 正运动学测试 */
  float mtmJoints[7] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.1f, 0.2f};
  plcVec3 tcpPos;
  plcQuat tcpOrient;
  plc_surgical_forward(&mtm, mtmJoints, &tcpPos, &tcpOrient);
  printf("MTM 正运动学:\n");
  printf("  TCP 位置: %.4f, %.4f, %.4f\n", tcpPos.x, tcpPos.y, tcpPos.z);
  printf("  TCP 姿态: %.4f, %.4f, %.4f, %.4f\n",
         tcpOrient.x, tcpOrient.y, tcpOrient.z, tcpOrient.w);

  /* 关节空间插值测试 */
  float qStart[7] = {0, 0, 0, 0, 0, 0, 0};
  float qEnd[7] = {0.5f, -0.3f, 0.2f, -0.5f, 0.1f, -0.2f, 0.3f};
  float qMid[7];
  plc_surgical_jointInterp(&mtm, qStart, qEnd, 0.5f, qMid);
  printf("关节插值 (t=0.5): ");
  for (int i = 0; i < 7; i++) printf("%.3f ", qMid[i]);
  printf("\n");

  /* 笛卡尔空间插值测试 */
  plcVec3 pStart = {0.3f, 0, 0.4f};
  plcVec3 pEnd = {0.35f, 0.05f, 0.38f};
  plcQuat qStartQ = plc_quat_identity();
  plcQuat qEndQ = plc_quat_axisAngle(plc_vec3(0, 1, 0), 0.2f);
  plcVec3 pOut;
  plcQuat qOut;
  plc_surgical_cartesianInterp(&mtm, pStart, qStartQ, pEnd, qEndQ, 0.3f, &pOut, &qOut);
  printf("笛卡尔插值 (t=0.3): pos=(%.4f, %.4f, %.4f)\n", pOut.x, pOut.y, pOut.z);

  /* 运行仿真 */
  printf("\n开始仿真 (1000 步)...\n");
  plc_sim_start(&sim);
  for (int i = 0; i < 1000; i++) {
    plc_sim_step(&sim);
    if (i % 200 == 0) {
      printf("  步 %d: 仿真时间=%.3f s, 软体节点0 pos=(%.4f, %.4f, %.4f)\n",
             i, sim.simTime,
             sim.softBodies[0].nodes[0].pos.x,
             sim.softBodies[0].nodes[0].pos.y,
             sim.softBodies[0].nodes[0].pos.z);
    }
  }
  plc_sim_stop(&sim);
  printf("仿真完成: %d 步, 仿真时间 %.3f s\n",
         sim.stats.stepCount, sim.simTime);

  /* 统计 */
  SimStats stats = plc_sim_getStats(&sim);
  printf("仿真统计: 步数=%d, 仿真时间=%.3f s\n",
         stats.stepCount, sim.simTime);

  plc_sim_deinit(&sim);
  printf("仿真系统已释放\n");

  return 0;
}
