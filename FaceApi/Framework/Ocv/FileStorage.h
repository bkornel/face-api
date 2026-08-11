#pragma once

#include <opencv2/core.hpp>

#include <string>

namespace fw
{
  namespace ocv
  {
    // Reads a named child of a settings node as a string.
    // @return false if the child is missing or its value is empty, leaving oValue empty
    bool get_value(const cv::FileNode& iNode, const std::string& iName, std::string& oValue);
  }
}
