#include "Framework/Settings.h"

namespace fw
{
  bool get_value(const cv::FileNode& iNode, const std::string& iName, std::string& oValue)
  {
    oValue = "";

    const cv::FileNode& subNode = iNode[iName.c_str()];

    if (!subNode.empty())
    {
      oValue = subNode.string();
      return !oValue.empty();
    }

    return false;
  }
}
