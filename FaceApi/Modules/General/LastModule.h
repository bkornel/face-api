#pragma once

#include "Modules/General/ModuleWithPort.hpp"
#include "Messages/ImageMessage.h"

#include <functional>

namespace face
{
  class LastModule :
    public ModuleWithPort<bool(ImageMessage::Shared)>
  {
  public:
    LastModule() = default;

    virtual ~LastModule() = default;

    bool Main(ImageMessage::Shared iImage) override;

    bool Get() const;

    void Wait() const;

    inline unsigned GetLastFrameId() const
    {
      return mLastImage ? mLastImage->GetFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastImage ? mLastImage->GetTimestamp() : 0LL;
    }

    inline const cv::Mat& GetLastResultBGR() const
    {
      return mLastImage ? mLastImage->GetFrameBGR() : sEmptyMat;
    }

  private:
    static const cv::Mat sEmptyMat;
    ImageMessage::Shared mLastImage = nullptr;
  };
}
