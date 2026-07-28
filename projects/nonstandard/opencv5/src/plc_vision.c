#include "plc_vision.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

void plc_vision_init(VisionSystem *vis)
{
  memset(vis, 0, sizeof(VisionSystem));
  vis->pipeline = PIPELINE_NONE;
}

int plc_vision_addCamera(VisionSystem *vis, int id, const char *name, const char *device,
                          uint32_t w, uint32_t h)
{
  if (!vis || !name || !device) return -1;
  if (vis->cameraCount >= VISION_MAX_CAMERAS) return -1;
  int idx = vis->cameraCount++;
  VisionCamera *cam = &vis->cameras[idx];
  cam->id = id;
  strncpy(cam->name, name, sizeof(cam->name) - 1);
  strncpy(cam->devicePath, device, sizeof(cam->devicePath) - 1);
  cam->width = w;
  cam->height = h;
  cam->format = PIXEL_FORMAT_BGR24;
  cam->fps = 30;
  return idx;
}

int plc_vision_start(VisionSystem *vis, VisionPipelineType pipeline)
{
  if (!vis) return -1;
  vis->pipeline = pipeline;
  vis->running = true;
  return 0;
}

void plc_vision_stop(VisionSystem *vis)
{
  if (vis) {
    vis->running = false;
    vis->pipeline = PIPELINE_NONE;
  }
}

int plc_vision_openCamera(VisionSystem *vis, int cameraId)
{
  if (!vis) return -1;
  for (int i = 0; i < vis->cameraCount; i++) {
    if (vis->cameras[i].id == cameraId) {
      vis->cameras[i].isOpen = true;
      return 0;
    }
  }
  return -1;
}

int plc_vision_closeCamera(VisionSystem *vis, int cameraId)
{
  if (!vis) return -1;
  for (int i = 0; i < vis->cameraCount; i++) {
    if (vis->cameras[i].id == cameraId) {
      vis->cameras[i].isOpen = false;
      return 0;
    }
  }
  return -1;
}

int plc_vision_capture(VisionSystem *vis, int cameraId, VisionFrame *out)
{
  (void)vis;
  (void)cameraId;
  (void)out;
  return -1;
}

int plc_vision_setParam(VisionSystem *vis, int cameraId, int paramId, float value)
{
  (void)vis;
  (void)cameraId;
  (void)paramId;
  (void)value;
  return 0;
}

/* ==================== 图像处理 (纯 C) ==================== */

void plc_vision_rgbToGray(const uint8_t *rgb, uint8_t *gray, int w, int h)
{
  for (int i = 0; i < w * h; i++) {
    int p = i * 3;
    gray[i] = (uint8_t)(0.299f * rgb[p] + 0.587f * rgb[p + 1] + 0.114f * rgb[p + 2]);
  }
}

void plc_vision_resize(const uint8_t *src, int sw, int sh, uint8_t *dst, int dw, int dh)
{
  for (int y = 0; y < dh; y++)
    for (int x = 0; x < dw; x++) {
      float sx = (float)x / dw * sw;
      float sy = (float)y / dh * sh;
      int ix = (int)sx;
      int iy = (int)sy;
      if (ix >= sw - 1) ix = sw - 2;
      if (iy >= sh - 1) iy = sh - 2;
      float fx = sx - ix, fy = sy - iy;
      for (int c = 0; c < 3; c++) {
        float v = (1 - fx) * (1 - fy) * src[(iy * sw + ix) * 3 + c]
                + fx * (1 - fy) * src[(iy * sw + ix + 1) * 3 + c]
                + (1 - fx) * fy * src[((iy + 1) * sw + ix) * 3 + c]
                + fx * fy * src[((iy + 1) * sw + ix + 1) * 3 + c];
        dst[(y * dw + x) * 3 + c] = (uint8_t)v;
      }
    }
}

void plc_vision_threshold(const uint8_t *src, uint8_t *dst, int w, int h, uint8_t thresh)
{
  for (int i = 0; i < w * h; i++)
    dst[i] = (src[i] > thresh) ? 255 : 0;
}

void plc_vision_sobel(const uint8_t *src, uint8_t *dst, int w, int h)
{
  static const int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  static const int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
  for (int y = 1; y < h - 1; y++)
    for (int x = 1; x < w - 1; x++) {
      int gx = 0, gy = 0;
      for (int ky = -1; ky <= 1; ky++)
        for (int kx = -1; kx <= 1; kx++) {
          uint8_t p = src[(y + ky) * w + (x + kx)];
          gx += p * sobelX[ky + 1][kx + 1];
          gy += p * sobelY[ky + 1][kx + 1];
        }
      int val = (int)(sqrtf((float)(gx * gx + gy * gy)) / 4.0f);
      if (val > 255) val = 255;
      dst[y * w + x] = (uint8_t)val;
    }
}

void plc_vision_gaussianBlur(const uint8_t *src, uint8_t *dst, int w, int h, float sigma)
{
  int kSize = (int)(sigma * 3) * 2 + 1;
  if (kSize < 3) kSize = 3;
  float *kernel = (float *)malloc(kSize * sizeof(float));
  float sum = 0;
  int half = kSize / 2;
  for (int i = 0; i < kSize; i++) {
    int x = i - half;
    kernel[i] = expf(-(float)(x * x) / (2 * sigma * sigma));
    sum += kernel[i];
  }
  for (int i = 0; i < kSize; i++) kernel[i] /= sum;
  float *temp = (float *)malloc(w * h * sizeof(float));
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      float v = 0;
      for (int kx = 0; kx < kSize; kx++) {
        int ix = x + kx - half;
        if (ix < 0) ix = 0;
        if (ix >= w) ix = w - 1;
        v += src[y * w + ix] * kernel[kx];
      }
      temp[y * w + x] = v;
    }
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      float v = 0;
      for (int ky = 0; ky < kSize; ky++) {
        int iy = y + ky - half;
        if (iy < 0) iy = 0;
        if (iy >= h) iy = h - 1;
        v += temp[iy * w + x] * kernel[ky];
      }
      dst[y * w + x] = (uint8_t)v;
    }
  free(kernel);
  free(temp);
}

void plc_vision_calibInit(VisionCalib *calib, float fx, float fy, float cx, float cy)
{
  memset(calib, 0, sizeof(VisionCalib));
  calib->fx = fx;
  calib->fy = fy;
  calib->cx = cx;
  calib->cy = cy;
  calib->cameraMatrix[0] = fx; calib->cameraMatrix[1] = 0;  calib->cameraMatrix[2] = cx;
  calib->cameraMatrix[3] = 0;  calib->cameraMatrix[4] = fy; calib->cameraMatrix[5] = cy;
  calib->cameraMatrix[6] = 0;  calib->cameraMatrix[7] = 0;  calib->cameraMatrix[8] = 1;
}

int plc_vision_calibFromCorners(VisionCalib *calib, float boardW, float boardH,
                                 int cols, int rows, const float *points, int pointCount)
{
  (void)calib;
  (void)boardW;
  (void)boardH;
  (void)cols;
  (void)rows;
  (void)points;
  (void)pointCount;
  return 0;
}

int plc_vision_detectArMarkers(const VisionFrame *frame, float markerSize,
                                VisionArMarker *markers, int maxMarkers)
{
  (void)frame;
  (void)markerSize;
  (void)markers;
  (void)maxMarkers;
  return 0;
}

int plc_vision_trackColor(const VisionFrame *frame, uint8_t rLow, uint8_t rHigh,
                           uint8_t gLow, uint8_t gHigh, uint8_t bLow, uint8_t bHigh,
                           float *centerX, float *centerY, float *area)
{
  (void)frame;
  (void)rLow;
  (void)rHigh;
  (void)gLow;
  (void)gHigh;
  (void)bLow;
  (void)bHigh;
  (void)centerX;
  (void)centerY;
  (void)area;
  return 0;
}

void plc_vision_setCallbacks(VisionSystem *vis, void (*onFrame)(VisionFrame *, void *),
                              void (*onDetect)(VisionDetection *, int, void *), void *userData)
{
  if (!vis) return;
  vis->onFrame = onFrame;
  vis->onDetect = onDetect;
  vis->userData = userData;
}

int plc_vision_opencvInit(VisionOpenCvBridge *bridge)
{
  (void)bridge;
  return 0;
}

int plc_vision_opencvProcFrame(VisionOpenCvBridge *bridge, VisionFrame *in, VisionFrame *out)
{
  (void)bridge;
  (void)in;
  (void)out;
  return 0;
}

int plc_vision_opencvDetect(VisionOpenCvBridge *bridge, VisionFrame *frame, VisionPipelineType pipe)
{
  (void)bridge;
  (void)frame;
  (void)pipe;
  return 0;
}
