#pragma once

#include "Framework/ErrorCode.h"
#include "FaceResult.h"
#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/ImageMessage.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace face
{
  class LastModule : public fw::Module,
                     public fw::Port<bool(std::shared_ptr<ImageMessage>, std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    LastModule() = default;

    ~LastModule() override = default;

    bool Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers) override;

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

    std::shared_ptr<ImageMessage> GetLastImage() const;

    fw::ErrorCode GetLastResults(FaceResults& oResults) const;

  private:
    mutable std::mutex mLastMutex;
    std::shared_ptr<ImageMessage> mLastImage = nullptr;
    std::shared_ptr<ActiveUsersMessage> mLastUsers = nullptr;
  };
}
