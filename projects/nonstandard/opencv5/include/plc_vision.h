#ifndef PLC_VISION_H
#define PLC_VISION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VISION_MAX_CAMERAS     8
#define VISION_MAX_DETECTIONS  64
#define VISION_MAX_KEYPOINTS   1024
#define VISION_IMAGE_MAX_W     1920
#define VISION_IMAGE_MAX_H     1080
#define VISION_IMAGE_CHANNELS  3

/* ==================== 像素格式 ==================== */
typedef enum {
  PIXEL_FORMAT_GRAY8,
  PIXEL_FORMAT_RGB24,
  PIXEL_FORMAT_BGR24,
  PIXEL_FORMAT_RGBA32,
  PIXEL_FORMAT_BGRA32,
  PIXEL_FORMAT_YUYV,
  PIXEL_FORMAT_MJPEG,
} PixelFormat;

/* ==================== 图像帧 ==================== */
typedef struct {
  uint32_t width;
  uint32_t height;
  PixelFormat format;
  uint8_t *data;
  uint32_t dataLen;
  uint32_t stride;
  uint64_t timestamp;
  int cameraId;
} VisionFrame;

/* ==================== 检测结果 ==================== */
typedef struct {
  float x, y;          /* 中心坐标 (归一化 0~1) */
  float width, height; /* 宽高 (归一化) */
  float confidence;    /* 置信度 0~1 */
  int classId;         /* 类别 ID */
  char label[64];      /* 标签名 */
} VisionDetection;

/* ==================== 标定参数 ==================== */
typedef struct {
  float cameraMatrix[9];    /* 3x3 相机内参 */
  float distCoeffs[8];     /* 畸变系数 */
  float rotationVec[3];     /* 旋转向量 */
  float translationVec[3];  /* 平移向量 */
  float projectionMatrix[12]; /* 3x4 投影矩阵 */
  float fx, fy, cx, cy;     /* 内参简写 */
} VisionCalib;

/* ==================== AR 标记 ==================== */
typedef struct {
  int id;
  float corners[4][2];     /* 四个角点 (像素坐标) */
  float pose[16];          /* 4x4 位姿矩阵 */
  float size;              /* 标记实际尺寸 (mm) */
  bool detected;
} VisionArMarker;

/* ==================== 视觉管道 ==================== */
typedef enum {
  PIPELINE_NONE,
  PIPELINE_OBJECT_DETECT,    /* 目标检测 */
  PIPELINE_AR_MARKER,        /* AR 标记追踪 */
  PIPELINE_LINE_FOLLOW,      /* 线跟踪 */
  PIPELINE_COLOR_TRACK,      /* 颜色追踪 */
  PIPELINE_QR_CODE,          /* QR/条形码 */
  PIPELINE_OPTICAL_FLOW,     /* 光流 */
  PIPELINE_STEREO_DEPTH,     /* 立体视觉深度 */
} VisionPipelineType;

/* ==================== 相机 ==================== */
typedef struct {
  int id;
  char name[64];
  char devicePath[256];     /* /dev/video0, IP 相机 URL */
  uint32_t width;
  uint32_t height;
  PixelFormat format;
  float fps;
  bool isOpen;
  VisionCalib calib;
  void *handle;             /* OpenCV VideoCapture handle */
} VisionCamera;

/* ==================== 视觉系统 ==================== */
typedef struct {
  VisionCamera cameras[VISION_MAX_CAMERAS];
  int cameraCount;
  VisionPipelineType pipeline;
  VisionFrame currentFrame;
  VisionDetection detections[VISION_MAX_DETECTIONS];
  int detectionCount;
  VisionArMarker markers[VISION_MAX_DETECTIONS];
  int markerCount;
  /* 回调 */
  void (*onFrame)(VisionFrame *frame, void *userData);
  void (*onDetect)(VisionDetection *detections, int count, void *userData);
  void *userData;
  /* 内部状态 */
  bool running;
  int fpsCounter;
  float fpsActual;
} VisionSystem;

/* ==================== API ==================== */

/* 系统管理 */
void plc_vision_init(VisionSystem *vis);
int plc_vision_addCamera(VisionSystem *vis, int id, const char *name, const char *device, uint32_t w, uint32_t h);
int plc_vision_start(VisionSystem *vis, VisionPipelineType pipeline);
void plc_vision_stop(VisionSystem *vis);

/* 相机控制 */
int plc_vision_openCamera(VisionSystem *vis, int cameraId);
int plc_vision_closeCamera(VisionSystem *vis, int cameraId);
int plc_vision_capture(VisionSystem *vis, int cameraId, VisionFrame *out);
int plc_vision_setParam(VisionSystem *vis, int cameraId, int paramId, float value);

/* 图像处理 (纯 C 实现, 不依赖 OpenCV) */
void plc_vision_rgbToGray(const uint8_t *rgb, uint8_t *gray, int w, int h);
void plc_vision_resize(const uint8_t *src, int sw, int sh, uint8_t *dst, int dw, int dh);
void plc_vision_threshold(const uint8_t *src, uint8_t *dst, int w, int h, uint8_t thresh);
void plc_vision_sobel(const uint8_t *src, uint8_t *dst, int w, int h);
void plc_vision_gaussianBlur(const uint8_t *src, uint8_t *dst, int w, int h, float sigma);

/* 标定 */
void plc_vision_calibInit(VisionCalib *calib, float fx, float fy, float cx, float cy);
int plc_vision_calibFromCorners(VisionCalib *calib, float boardW, float boardH,
                                 int cols, int rows, const float *points, int pointCount);

/* AR 标记检测 (纯 C) */
int plc_vision_detectArMarkers(const VisionFrame *frame, float markerSize,
                                VisionArMarker *markers, int maxMarkers);

/* 颜色追踪 */
int plc_vision_trackColor(const VisionFrame *frame, uint8_t rLow, uint8_t rHigh,
                           uint8_t gLow, uint8_t gHigh, uint8_t bLow, uint8_t bHigh,
                           float *centerX, float *centerY, float *area);

/* OpenCV5 桥接 */
typedef struct {
  int (*init)(void);
  int (*procFrame)(const uint8_t *in, int w, int h, uint8_t *out);
  int (*detect)(const uint8_t *frame, int w, int h, VisionDetection *results, int max);
  void *handle;
} VisionOpenCvBridge;

int plc_vision_opencvInit(VisionOpenCvBridge *bridge);
int plc_vision_opencvProcFrame(VisionOpenCvBridge *bridge, VisionFrame *in, VisionFrame *out);
int plc_vision_opencvDetect(VisionOpenCvBridge *bridge, VisionFrame *frame, VisionPipelineType pipe);

/* 回调注册 */
void plc_vision_setCallbacks(VisionSystem *vis, void (*onFrame)(VisionFrame *, void *),
                              void (*onDetect)(VisionDetection *, int, void *), void *userData);

#ifdef __cplusplus
}
#endif

#endif /* PLC_VISION_H */
