#include "plc_vision.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>

extern "C" {

int plc_vision_opencvInit(VisionOpenCvBridge *bridge)
{
  if (!bridge) return -1;
  bridge->init = nullptr;
  bridge->procFrame = nullptr;
  bridge->detect = nullptr;
  bridge->handle = new cv::dnn::Net();
  return 0;
}

int plc_vision_opencvProcFrame(VisionOpenCvBridge *bridge, VisionFrame *in, VisionFrame *out)
{
  if (!bridge || !in || !out) return -1;
  cv::Mat src(in->height, in->width, CV_8UC3, in->data);
  cv::Mat dst;

  switch (in->format) {
  case PIXEL_FORMAT_GRAY8:
    cv::cvtColor(src, dst, cv::COLOR_GRAY2BGR);
    break;
  case PIXEL_FORMAT_RGB24:
    cv::cvtColor(src, dst, cv::COLOR_RGB2BGR);
    break;
  case PIXEL_FORMAT_BGR24:
    dst = src.clone();
    break;
  default:
    dst = src.clone();
    break;
  }

  if (out->data && out->width == in->width && out->height == in->height) {
    memcpy(out->data, dst.data, dst.total() * dst.elemSize());
    out->width = dst.cols;
    out->height = dst.rows;
    out->format = PIXEL_FORMAT_BGR24;
    out->dataLen = dst.total() * dst.elemSize();
  }
  return 0;
}

int plc_vision_opencvDetect(VisionOpenCvBridge *bridge, VisionFrame *frame, VisionPipelineType pipe)
{
  if (!bridge || !frame) return -1;

  cv::Mat img(frame->height, frame->width, CV_8UC3, frame->data);
  if (frame->format == PIXEL_FORMAT_GRAY8) {
    cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
  }

  switch (pipe) {
  case PIPELINE_OBJECT_DETECT: {
    cv::dnn::Net *net = (cv::dnn::Net *)bridge->handle;
    if (net->empty()) return -1;
    cv::Mat blob = cv::dnn::blobFromImage(img, 1.0 / 255, cv::Size(300, 300),
                                           cv::Scalar(0, 0, 0), true, false);
    net->setInput(blob);
    cv::Mat detections = net->forward();
    return detections.size[2];
  }
  case PIPELINE_AR_MARKER: {
    cv::aruco::Dictionary dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    cv::aruco::detectMarkers(img, dict, corners, ids);
    return (int)ids.size();
  }
  case PIPELINE_OPTICAL_FLOW: {
    static cv::Mat prevGray;
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    if (!prevGray.empty()) {
      cv::Mat flow;
      cv::calcOpticalFlowFarneback(prevGray, gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
    }
    prevGray = gray.clone();
    return 0;
  }
  case PIPELINE_STEREO_DEPTH: {
    static cv::StereoBM sbm;
    sbm.setBlockSize(21);
    sbm.setNumDisparities(64);
    cv::Mat left, right;
    if (img.cols > img.rows) {
      left = img(cv::Rect(0, 0, img.cols / 2, img.rows));
      right = img(cv::Rect(img.cols / 2, 0, img.cols / 2, img.rows));
    } else { return -1; }
    cv::Mat grayL, grayR, disp;
    cv::cvtColor(left, grayL, cv::COLOR_BGR2GRAY);
    cv::cvtColor(right, grayR, cv::COLOR_BGR2GRAY);
    sbm.compute(grayL, grayR, disp);
    return 0;
  }
  default:
    return -1;
  }
  return -1;
}

int plc_vision_opencvLoadDnn(VisionOpenCvBridge *bridge, const char *modelPath,
                              const char *configPath, const char *framework)
{
  if (!bridge || !modelPath) return -1;
  cv::dnn::Net *net = (cv::dnn::Net *)bridge->handle;
  try {
    if (configPath) {
      *net = cv::dnn::readNet(modelPath, configPath, framework ? framework : "");
    } else {
      *net = cv::dnn::readNet(modelPath);
    }
  } catch (...) { return -1; }
  return net->empty() ? -1 : 0;
}

void plc_vision_opencvClose(VisionOpenCvBridge *bridge)
{
  if (bridge && bridge->handle) {
    delete (cv::dnn::Net *)bridge->handle;
    bridge->handle = nullptr;
  }
}

} /* extern "C" */
