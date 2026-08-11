#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Messages/ImageMessage.h"

#include <chrono>
#include <functional>

namespace face
{
  class LastModule : public fw::Module,
                     public fw::Port<bool(ImageMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(LastModule);

    LastModule() = default;

    ~LastModule() override = default;

    bool Main(ImageMessage::Shared iImage) override;

    inline bool HasOutput() const
    {
      return mOutputPort && mOutputPort->Ready() && mOutputPort->Get();
    }

    /// @brief Output generation of this module, read before ticking the graph.
    inline unsigned long long GetGeneration() const
    {
      return mOutputPort ? mOutputPort->GetGeneration() : 0ULL;
    }

    /// @brief Blocks until the graph produced an output newer than iGeneration.
    /// The output Future is reused for every frame and keeps its last value, so
    /// waiting for "a value to be present" would return immediately from the
    /// second frame on. The generation is what identifies the current frame.
    /// @return false if the timeout elapsed, which means the graph did not finish.
    inline bool WaitForNewOutput(unsigned long long iGeneration, long long iTimeoutMs) const
    {
      return mOutputPort
               ? mOutputPort->WaitForNewValue(iGeneration, std::chrono::milliseconds(iTimeoutMs))
               : false;
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
