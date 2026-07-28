cmake_minimum_required(VERSION 3.10)
project(plc-vision-cpp C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_OPENCV_WRAPPER "Build OpenCV5 C++ wrapper" ON)

if(BUILD_OPENCV_WRAPPER)
  # 优先使用 vcpkg 或系统安装的 OpenCV
  find_package(OpenCV QUIET)
  if(OpenCV_FOUND)
    message(STATUS "Found OpenCV: ${OpenCV_DIR}")
    add_library(plc-vision-cpp STATIC src/plc_vision_wrapper.cpp)
    target_include_directories(plc-vision-cpp PRIVATE include ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(plc-vision-cpp ${OpenCV_LIBS})
  else()
    message(WARNING "OpenCV not found. Downloading and building minimal OpenCV...")
    # 下载 OpenCV 源码（仅必要模块）
    include(FetchContent)
    FetchContent_Declare(
      opencv
      GIT_REPOSITORY https://github.com/opencv/opencv.git
      GIT_TAG 5.x
      GIT_SHALLOW TRUE
    )
    set(BUILD_opencv_world OFF)
    set(BUILD_opencv_core ON)
    set(BUILD_opencv_imgproc ON)
    set(BUILD_opencv_highgui OFF)
    set(BUILD_opencv_videoio OFF)
    set(BUILD_opencv_imgcodecs ON)
    set(BUILD_opencv_calib3d ON)
    set(BUILD_opencv_features2d ON)
    set(BUILD_opencv_dnn ON)
    set(BUILD_opencv_objdetect ON)
    set(BUILD_opencv_video ON)
    set(BUILD_opencv_stitching OFF)
    set(BUILD_opencv_gapi OFF)
    set(BUILD_opencv_python OFF)
    set(BUILD_opencv_java OFF)
    set(BUILD_opencv_js OFF)
    set(BUILD_TESTS OFF)
    set(BUILD_PERF_TESTS OFF)
    set(BUILD_EXAMPLES OFF)
    set(CMAKE_BUILD_TYPE Release)

    FetchContent_MakeAvailable(opencv)

    add_library(plc-vision-cpp STATIC src/plc_vision_wrapper.cpp)
    target_include_directories(plc-vision-cpp PRIVATE
      include
      ${opencv_SOURCE_DIR}/include
      ${opencv_BINARY_DIR}
    )
    target_link_libraries(plc-vision-cpp opencv_core opencv_imgproc opencv_dnn
      opencv_calib3d opencv_features2d opencv_objdetect opencv_video opencv_imgcodecs)
  endif()
endif()

# 纯 C 视觉库 (不依赖 OpenCV)
add_library(plc-vision-c STATIC src/plc_vision.c)
target_include_directories(plc-vision-c PUBLIC include)
