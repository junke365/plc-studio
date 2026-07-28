#include "plc_sofa_sim.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  int port = 54001;
  if (argc > 1) port = atoi(argv[1]);

  SofaSimulator ss;
  if (sofa_sim_init(&ss, port) != 0) {
    fprintf(stderr, "SOFA 仿真器初始化失败\n");
    return 1;
  }

  /* 添加 MTM 和 PSM 机器人 */
  sofa_sim_addSurgical(&ss, SURGICAL_MTM);
  sofa_sim_addSurgical(&ss, SURGICAL_PSM);

  printf("SOFA 手术仿真器启动在端口 %d\n", port);
  sofa_sim_runServer(&ss);

  sofa_sim_deinit(&ss);
  return 0;
}
