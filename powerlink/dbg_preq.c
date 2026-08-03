#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "plk/plk.h"
#include "plk/frame.h"

int main(void) {
  printf("sizeof(PlkFrame)=%d\n", (int)sizeof(PlkFrame));
  printf("sizeof(PlkPreqFrame)=%d\n", (int)sizeof(PlkPreqFrame));
  printf("offsetof(preq.reserved1)=%d\n", (int)offsetof(PlkFrame, data.preq.reserved1));
  printf("offsetof(preq.flag1)=%d\n", (int)offsetof(PlkFrame, data.preq.flag1));
  printf("offsetof(preq.pdoVersion)=%d\n", (int)offsetof(PlkFrame, data.preq.pdoVersion));
  printf("offsetof(preq.sizeLe)=%d\n", (int)offsetof(PlkFrame, data.preq.sizeLe));
  printf("offsetof(preq.aPayload)=%d\n", (int)offsetof(PlkFrame, data.preq.aPayload));
  printf("offsetof(frame.messageType)=%d\n", (int)offsetof(PlkFrame, messageType));
  printf("offsetof(frame.dstNodeId)=%d\n", (int)offsetof(PlkFrame, dstNodeId));
  printf("offsetof(frame.srcNodeId)=%d\n", (int)offsetof(PlkFrame, srcNodeId));
  printf("sizeof(PlkSocFrame)=%d\n", (int)sizeof(PlkSocFrame));
  printf("offsetof(PlkSocFrame.relativeTimeLe)=%d\n", (int)offsetof(PlkSocFrame, relativeTimeLe));
  return 0;
}
