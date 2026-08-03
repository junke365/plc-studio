#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "smartlink/smartlink.h"
#include "smartlink/frame.h"

int main(void) {
  printf("sizeof(SlFrame)=%d\n", (int)sizeof(SlFrame));
  printf("sizeof(SlPreqFrame)=%d\n", (int)sizeof(SlPreqFrame));
  printf("offsetof(preq.reserved1)=%d\n", (int)offsetof(SlFrame, data.preq.reserved1));
  printf("offsetof(preq.flag1)=%d\n", (int)offsetof(SlFrame, data.preq.flag1));
  printf("offsetof(preq.pdoVersion)=%d\n", (int)offsetof(SlFrame, data.preq.pdoVersion));
  printf("offsetof(preq.sizeLe)=%d\n", (int)offsetof(SlFrame, data.preq.sizeLe));
  printf("offsetof(preq.aPayload)=%d\n", (int)offsetof(SlFrame, data.preq.aPayload));
  printf("offsetof(frame.messageType)=%d\n", (int)offsetof(SlFrame, messageType));
  printf("offsetof(frame.dstNodeId)=%d\n", (int)offsetof(SlFrame, dstNodeId));
  printf("offsetof(frame.srcNodeId)=%d\n", (int)offsetof(SlFrame, srcNodeId));
  printf("sizeof(SlSocFrame)=%d\n", (int)sizeof(SlSocFrame));
  printf("offsetof(SlSocFrame.relativeTimeLe)=%d\n", (int)offsetof(SlSocFrame, relativeTimeLe));
  return 0;
}
