#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Messages/ImageMessage.h"

#include <functional>

namespace face
{
  class LastModule :
    public fw::Module,
    public fw::Port<bool(ImageMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(LastModule);

    LastModule() = default;

    ~LastModule() override = default;

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
    ImageMessage::Shared mLastImage = nullptr;
  };
}
