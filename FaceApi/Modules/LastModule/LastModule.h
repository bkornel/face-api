#pragma once

#include "Framework/ErrorCode.h"
#include "FaceResult.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"
#include "Messages/UserSnapshotMessage.h"
#include "Messages/ImageMessage.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

namespace face
{
  class LastModule : public fw::Module,
                     public fw::Port<bool(std::shared_ptr<ImageMessage>, std::shared_ptr<UserSnapshotMessage>)>
  {
  public:

    LastModule() = default;

    ~LastModule() override = default;

    bool Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<UserSnapshotMessage> iUsers) override;

    void Clear() override;

    inline bool HasOutput() const
    {
      return mOutputPort && mOutputPort->Ready() && mOutputPort->Get();
    }

    inline uint64_t GetGeneration() const
    {
      return mOutputPort ? mOutputPort->GetGeneration() : 0ULL;
    }

    inline bool WaitForNewOutput(uint64_t iGeneration, int64_t iTimeoutMs) const
    {
      return mOutputPort
               ? mOutputPort->WaitForNewValue(iGeneration, fw::Milliseconds(iTimeoutMs))
               : false;
    }

    uint32_t GetLastFrameId() const;

    int64_t GetLastTimestamp() const;

    std::shared_ptr<ImageMessage> GetLastImage() const;

    fw::ErrorCode GetLastResults(FaceResults& oResults) const;

  private:
    mutable std::mutex mLastMutex;
    std::shared_ptr<ImageMessage> mLastImage = nullptr;
    std::shared_ptr<UserSnapshotMessage> mLastUsers = nullptr;
  };
}
