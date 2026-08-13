#pragma once

#include <string>
#include <vector>

namespace fe
{
  /// @brief The capture devices of the machine, by the name their driver reports.
  ///
  /// OpenCV addresses cameras by index and cannot tell what is behind one, so the list the
  /// UI offers is read from DirectShow instead. The index of an entry is the index
  /// cv::VideoCapture is opened with, which is the order the same enumerator returns.
  class CameraEnumerator
  {
  public:
    struct Device
    {
      int index = 0;
      std::wstring name;
      std::wstring path;
    };

    /// @brief Enumerates the video input devices. Never throws; an empty result means
    /// either no camera or no DirectShow, and the caller falls back to probing by index.
    static std::vector<Device> Enumerate();
  };
}
