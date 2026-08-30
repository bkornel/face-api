#pragma once

#include <opencv2/core.hpp>

#include <string>

namespace fw
{
  /// @brief Where a network runs. The default is whatever OpenCV was built to prefer, which
  /// is the CPU on a stock build; a machine with a usable GPU can be told to use it without
  /// rebuilding anything, and the shape model is the most expensive stage in the pipeline.
  struct DnnOptions
  {
    int backend = 0; ///< cv::dnn::DNN_BACKEND_DEFAULT
    int target = 0;  ///< cv::dnn::DNN_TARGET_CPU
  };

  /// @brief Reads the "backend" and "target" values of a module's settings node.
  /// Names are the OpenCV ones, lower case: default, opencv, cuda, openvino / inference_engine
  /// for the backend, and cpu, opencl, opencl_fp16, cuda, cuda_fp16, vulkan for the target.
  /// An unknown name leaves that half at its default and says so in the log.
  DnnOptions get_dnn_options(const cv::FileNode& iNode);
}
