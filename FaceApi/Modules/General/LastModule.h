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
    FW_DEFINE_SMART_POINTERS(LastModule);

    LastModule() = default;

    virtual ~LastModule() = default;

    bool Main(ImageMessage::Shared iImage) override;

    inline bool HasOutput() const
    {
      return mOutputPort->Get();
    }

    inline void Wait() const
    {
      mOutputPort->Wait();
    }

    inline unsigned GetLastFrameId() const
    {
      return mLastImage ? mLastImage->GetFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastImage ? mLastImage->GetTimestamp() : 0LL;
    }

    inline ImageMessage::Shared GetLastImage() const
    {
      return mLastImage;
    }

  private:
    static const cv::Mat sEmptyMat;
    ImageMessage::Shared mLastImage = nullptr;
  };
}
