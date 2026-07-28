#ifndef PLC_SOFA_SIM_H
#define PLC_SOFA_SIM_H

#include "plc_simulation.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdbool.h>
#include <stdint.h>

/* ==================== 命令协议 ==================== */
#define CMD_SIM_STATE_REQ  0x01  /* 请求状态 */
#define CMD_SIM_STATE_SET  0x02  /* 设置状态 */
#define CMD_SIM_STEP       0x03  /* 步进 */
#define CMD_SIM_SET_JOINT  0x04  /* 设置关节角 */
#define CMD_SIM_START      0x05  /* 启动仿真 */
#define CMD_SIM_STOP       0x06  /* 停止仿真 */
#define CMD_SIM_PING       0xFF  /* 心跳 */

/* ==================== SOFA 仿真器 ==================== */
typedef struct {
  SimulationSystem sim;
  float stateBuffer[2048];
  int stateLen;
  int port;
  int serverFd;
  bool running;
  bool clientConnected;
#ifdef _WIN32
  bool wsaInit;
#endif
} SofaSimulator;

/* 初始化和反初始化 */
int sofa_sim_init(SofaSimulator *ss, int port);
void sofa_sim_deinit(SofaSimulator *ss);

/* 启动/停止仿真循环 */
int sofa_sim_start(SofaSimulator *ss);
void sofa_sim_stop(SofaSimulator *ss);

/* 仿真步进 */
int sofa_sim_step(SofaSimulator *ss);

/* 添加手术机器人 (快捷) */
int sofa_sim_addSurgical(SofaSimulator *ss, SurgicalRobotType type);

/* 获取当前状态 (供网络发送) */
const float *sofa_sim_getState(SofaSimulator *ss, int *len);

/* 从网络接收状态更新 */
int sofa_sim_applyState(SofaSimulator *ss, const float *buffer, int len);

/* 处理一个客户端连接 (阻塞，在新线程调用) */
void sofa_sim_handleClient(SofaSimulator *ss, int clientFd);

/* 运行服务器 (阻塞，接受连接并处理) */
void sofa_sim_runServer(SofaSimulator *ss);

#endif /* PLC_SOFA_SIM_H */
