#include "plc_sofa_sim.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

/* ==================== Socket 辅助函数 ==================== */

/* 循环发送所有字节 */
static int sendAll(int fd, const char *data, int size)
{
  int total = 0;
  while (total < size) {
#ifdef _WIN32
    int n = (int)send(fd, data + total, size - total, 0);
#else
    int n = (int)write(fd, data + total, size - total);
#endif
    if (n <= 0) return -1;
    total += n;
  }
  return 0;
}

/* 循环接收所有字节 */
static int recvAll(int fd, char *data, int size)
{
  int total = 0;
  while (total < size) {
#ifdef _WIN32
    int n = (int)recv(fd, data + total, size - total, 0);
#else
    int n = (int)read(fd, data + total, size - total);
#endif
    if (n <= 0) return -1;
    total += n;
  }
  return 0;
}

/* 关闭 socket */
static void closeSocket(int fd)
{
  if (fd < 0) return;
#ifdef _WIN32
  closesocket(fd);
#else
  close(fd);
#endif
}

/* ==================== 线程包装 ==================== */

#ifdef _WIN32
typedef uintptr_t ThreadHandle;
static ThreadHandle createThread(void (*func)(void *), void *arg)
{
  return _beginthreadex(NULL, 0, (unsigned int (__stdcall *)(void *))func, arg, 0, NULL);
}
static void detachThread(ThreadHandle h)
{
  if (h) CloseHandle((HANDLE)h);
}
#else
typedef pthread_t ThreadHandle;
static ThreadHandle createThread(void *(*func)(void *), void *arg)
{
  pthread_t tid;
  pthread_create(&tid, NULL, func, arg);
  return tid;
}
static void detachThread(ThreadHandle h)
{
  pthread_detach(h);
}
#endif

/* ==================== 客户端处理线程 ==================== */

typedef struct {
  SofaSimulator *ss;
  int clientFd;
} ClientContext;

/* 处理单个客户端命令 */
static void processClient(ClientContext *ctx)
{
  SofaSimulator *ss = ctx->ss;
  int fd = ctx->clientFd;

  uint8_t cmdBuf[4];    /* 命令 + 保留 */
  int32_t lenBuf;       /* 数据长度 */

  while (ss->running) {
    /* 接收命令帧: 4字节 (cmd + reserved) + 4字节 (len) */
    if (recvAll(fd, (char *)cmdBuf, 4) != 0) break;
    if (recvAll(fd, (char *)&lenBuf, 4) != 0) break;

    uint8_t cmd = cmdBuf[0];
    int dataLen = (int)lenBuf;

    switch (cmd) {
      case CMD_SIM_STATE_REQ: {
        /* 发送状态数据 */
        int stateLen = 0;
        const float *state = sofa_sim_getState(ss, &stateLen);
        int32_t netLen = (int32_t)stateLen;

        uint8_t respCmd[4] = {CMD_SIM_STATE_REQ, 0, 0, 0};
        sendAll(fd, (char *)respCmd, 4);
        sendAll(fd, (char *)&netLen, 4);
        if (stateLen > 0) {
          sendAll(fd, (char *)state, stateLen * (int)sizeof(float));
        }
        break;
      }

      case CMD_SIM_STATE_SET: {
        /* 接收状态数据并应用 */
        if (dataLen > 0 && dataLen <= 2048) {
          float *stateBuf = (float *)malloc((size_t)dataLen * sizeof(float));
          if (stateBuf) {
            if (recvAll(fd, (char *)stateBuf, dataLen * (int)sizeof(float)) == 0) {
              sofa_sim_applyState(ss, stateBuf, dataLen);
            }
            free(stateBuf);
          }
        }
        break;
      }

      case CMD_SIM_STEP: {
        /* 步进并返回新状态 */
        sofa_sim_step(ss);

        int stateLen = 0;
        const float *state = sofa_sim_getState(ss, &stateLen);
        int32_t netLen = (int32_t)stateLen;

        uint8_t respCmd[4] = {CMD_SIM_STEP, 0, 0, 0};
        sendAll(fd, (char *)respCmd, 4);
        sendAll(fd, (char *)&netLen, 4);
        if (stateLen > 0) {
          sendAll(fd, (char *)state, stateLen * (int)sizeof(float));
        }
        break;
      }

      case CMD_SIM_SET_JOINT: {
        /* 设置关节角: [robotIdx(4) + 8*float] */
        if (dataLen >= 4) {
          char *buf = (char *)malloc((size_t)dataLen);
          if (buf) {
            if (recvAll(fd, buf, dataLen) == 0) {
              int robotIdx;
              memcpy(&robotIdx, buf, 4);
              int jointCount = (dataLen - 4) / (int)sizeof(float);
              if (jointCount > 8) jointCount = 8;

              SurgicalRobot *robot = plc_sim_getRobot(&ss->sim, robotIdx);
              if (robot) {
                float *jointData = (float *)(buf + 4);
                for (int j = 0; j < jointCount && j < robot->dof; j++) {
                  robot->qCurrent[j] = jointData[j];
                }
              }
            }
            free(buf);
          }
        }
        break;
      }

      case CMD_SIM_START: {
        sofa_sim_start(ss);
        break;
      }

      case CMD_SIM_STOP: {
        sofa_sim_stop(ss);
        break;
      }

      case CMD_SIM_PING: {
        /* 回复心跳 */
        uint8_t pong[4] = {CMD_SIM_PING, 0, 0, 0};
        sendAll(fd, (char *)pong, 4);
        int32_t zero = 0;
        sendAll(fd, (char *)&zero, 4);
        break;
      }

      default:
        break;
    }
  }

  closeSocket(fd);
  ss->clientConnected = false;
  free(ctx);
}

#ifdef _WIN32
static void clientThreadWin(void *arg)
{
  processClient((ClientContext *)arg);
}
#else
static void *clientThreadPosix(void *arg)
{
  processClient((ClientContext *)arg);
  return NULL;
}
#endif

/* ==================== 公开 API ==================== */

void sofa_sim_handleClient(SofaSimulator *ss, int clientFd)
{
  ClientContext *ctx = (ClientContext *)malloc(sizeof(ClientContext));
  if (!ctx) {
    closeSocket(clientFd);
    return;
  }
  ctx->ss = ss;
  ctx->clientFd = clientFd;

  processClient(ctx);
}

void sofa_sim_runServer(SofaSimulator *ss)
{
  if (!ss) return;

#ifdef _WIN32
  if (!ss->wsaInit) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
      fprintf(stderr, "WSAStartup 失败\n");
      return;
    }
    ss->wsaInit = true;
  }
#endif

  /* 创建 socket */
  ss->serverFd = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (ss->serverFd < 0) {
    perror("socket");
    return;
  }

  /* 允许地址重用 */
  int opt = 1;
#ifdef _WIN32
  setsockopt(ss->serverFd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
#else
  setsockopt(ss->serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

  /* bind */
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)ss->port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(ss->serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    closeSocket(ss->serverFd);
    ss->serverFd = -1;
    return;
  }

  /* listen */
  if (listen(ss->serverFd, 5) < 0) {
    perror("listen");
    closeSocket(ss->serverFd);
    ss->serverFd = -1;
    return;
  }

  ss->running = true;

  /* 接受连接循环 */
  while (ss->running) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = (int)accept(ss->serverFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd < 0) {
      if (ss->running) perror("accept");
      break;
    }

    ss->clientConnected = true;
    printf("客户端已连接: %s:%d\n",
           inet_ntoa(clientAddr.sin_addr),
           ntohs(clientAddr.sin_port));

    /* 为新客户端创建线程 */
    ClientContext *ctx = (ClientContext *)malloc(sizeof(ClientContext));
    if (!ctx) {
      closeSocket(clientFd);
      continue;
    }
    ctx->ss = ss;
    ctx->clientFd = clientFd;

#ifdef _WIN32
    ThreadHandle h = createThread(clientThreadWin, ctx);
    detachThread(h);
#else
    ThreadHandle h = createThread(clientThreadPosix, ctx);
    detachThread(h);
#endif
  }

  /* 清理 */
  closeSocket(ss->serverFd);
  ss->serverFd = -1;
}
