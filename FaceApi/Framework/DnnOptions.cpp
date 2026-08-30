#include "Framework/DnnOptions.h"

#include "Framework/Settings.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/dnn.hpp>

#include <map>

namespace fw
{
  namespace
  {
    const std::map<std::string, int>& BackendsByName()
    {
      static const std::map<std::string, int> sBackends = {
        { "default", cv::dnn::DNN_BACKEND_DEFAULT },
        { "opencv", cv::dnn::DNN_BACKEND_OPENCV },
        { "cuda", cv::dnn::DNN_BACKEND_CUDA },
        { "openvino", cv::dnn::DNN_BACKEND_INFERENCE_ENGINE },
        { "inference_engine", cv::dnn::DNN_BACKEND_INFERENCE_ENGINE },
        { "vkcom", cv::dnn::DNN_BACKEND_VKCOM }
      };

      return sBackends;
    }

    const std::map<std::string, int>& TargetsByName()
    {
      static const std::map<std::string, int> sTargets = {
        { "cpu", cv::dnn::DNN_TARGET_CPU },
        { "opencl", cv::dnn::DNN_TARGET_OPENCL },
        { "opencl_fp16", cv::dnn::DNN_TARGET_OPENCL_FP16 },
        { "cuda", cv::dnn::DNN_TARGET_CUDA },
        { "cuda_fp16", cv::dnn::DNN_TARGET_CUDA_FP16 },
        { "vulkan", cv::dnn::DNN_TARGET_VULKAN }
      };

      return sTargets;
    }

    int Lookup(const std::map<std::string, int>& iByName, const std::string& iValue, int iFallback, const char* iWhat)
    {
      const auto it = iByName.find(str::to_lower(str::trim(iValue)));

      if (it != iByName.end()) return it->second;

      LOG(WARNING) << "Unknown DNN " << iWhat << " \"" << iValue << "\", using the default instead.";

      return iFallback;
    }
  }

  DnnOptions get_dnn_options(const cv::FileNode& iNode)
  {
    DnnOptions options;

    if (iNode.empty()) return options;

    std::string value;

    if (get_value(iNode, "backend", value))
      options.backend = Lookup(BackendsByName(), value, options.backend, "backend");

    if (get_value(iNode, "target", value))
      options.target = Lookup(TargetsByName(), value, options.target, "target");

    if (options.backend != cv::dnn::DNN_BACKEND_DEFAULT || options.target != cv::dnn::DNN_TARGET_CPU)
    {
      LOG(INFO) << "DNN backend " << options.backend << ", target " << options.target;
    }

    return options;
  }
}
