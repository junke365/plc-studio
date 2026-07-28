#include "plc_vision.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  VisionSystem sys;
  int cameraId;
  float markerX, markerY;
  int frameCount;
} HybridVision;

HybridVision *hybrid_vision_create(int cameraId)
{
  HybridVision *hv = (HybridVision *)calloc(1, sizeof(HybridVision));
  if (!hv) return NULL;
  plc_vision_init(&hv->sys);
  hv->cameraId = cameraId;
  plc_vision_addCamera(&hv->sys, cameraId, "hybrid_cam", "/dev/video0", 640, 480);
  plc_vision_openCamera(&hv->sys, cameraId);
  plc_vision_start(&hv->sys, PIPELINE_AR_MARKER);
  return hv;
}

int hybrid_vision_process(HybridVision *hv)
{
  if (!hv) return -1;
  hv->frameCount++;
  VisionFrame frame = {0};
  if (plc_vision_capture(&hv->sys, hv->cameraId, &frame) != 0) {
    VisionArMarker markers[4];
    int n = plc_vision_detectArMarkers(&frame, 20.0f, markers, 4);
    if (n > 0 && markers[0].detected) {
      hv->markerX = markers[0].pose[12];
      hv->markerY = markers[0].pose[13];
      return 1;
    }
  }
  return 0;
}

void hybrid_vision_getOffset(HybridVision *hv, float *ox, float *oy)
{
  if (ox) *ox = hv->markerX;
  if (oy) *oy = hv->markerY;
}

void hybrid_vision_destroy(HybridVision *hv)
{
  if (hv) {
    plc_vision_stop(&hv->sys);
    free(hv);
  }
}
