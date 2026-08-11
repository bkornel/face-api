#pragma once

#include "FaceResult.h"
#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/ImageMessage.h"

#include <chrono>
#include <functional>
#include <mutex>

namespace face
{
  class LastModule : public fw::Module,
                     public fw::Port<bool(ImageMessage::Shared, ActiveUsersMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(LastModule);

    LastModule() = default;

    ~LastModule() override = default;

    bool Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iUsers) override;

    void Clear() override;

    inline bool HasOutput() const
    {
      return mOutputPort && mOutputPort->Ready() && mOutputPort->Get();
    }

    inline unsigned long long GetGeneration() const
    {
      return mOutputPort ? mOutputPort->GetGeneration() : 0ULL;
    }

    inline bool WaitForNewOutput(unsigned long long iGeneration, long long iTimeoutMs) const
    {
      return mOutputPort
               ? mOutputPort->WaitForNewValue(iGeneration, std::chrono::milliseconds(iTimeoutMs))
               : false;
    }

    unsigned GetLastFrameId() const;

    long long GetLastTimestamp() const;

    ImageMessage::Shared GetLastImage() const;

    fw::ErrorCode GetLastResults(FaceResults& oResults) const;

  private:
    mutable std::mutex mLastMutex;
    ImageMessage::Shared mLastImage = nullptr;
    ActiveUsersMessage::Shared mLastUsers = nullptr;
  };
}
