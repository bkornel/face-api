#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "Framework/ErrorCode.h"
#include "FaceResult.h"
#include "Framework/Messaging/MessageBus.h"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Graph/Module.h"
#include "Framework/Thread.h"
#include "Modules/ModuleGraph.h"

namespace face
{
  class FaceApi : public fw::Module,
                  public fw::Thread
  {
    using MessageQueue = fw::MessageQueue<std::shared_ptr<ImageMessage>>;

  public:
    static FaceApi& GetInstance();

    FaceApi(const FaceApi& iOther) = delete;

    ~FaceApi() override;

    FaceApi& operator=(const FaceApi& iOther) = delete;

    void PushCameraFrame(const cv::Mat& iFrame);

    fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

    // The overlay data of the last processed frame, for hosts that draw it themselves.
    // Needs the users to be wired into lastModule's second port, see settings.json.
    fw::ErrorCode GetResults(FaceResults& oResults) const;

    void Clear() override;

    void SetRunFaceDetector();

    void SetVerbose(bool iVerbose);

    void OnOffVerbose();

    inline uint32_t GetLastFrameId() const
    {
      return mModuleGraph ? mModuleGraph->GetLastFrameId() : 0U;
    }

    inline int64_t GetLastTimestamp() const
    {
      return mModuleGraph ? mModuleGraph->GetLastTimestamp() : 0LL;
    }

    void SetWorkingDirectory(const std::string& iWorkingDirectory);

  private:
    FaceApi();

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    void OnFrameProcessed(std::shared_ptr<ImageMessage> iMessage);

    /// @brief Serialises one frame of the graph against the calls that reset it
    std::mutex mProcessMutex;

    fw::MessageBus mBus;

    ModuleGraph::FrameProcessedToken mFrameProcessedToken = 0ULL;

    std::shared_ptr<ModuleGraph> mModuleGraph = nullptr;

    std::atomic<uint32_t> mCameraFrameId{ 0U };

    std::atomic<bool> mVerbose{ false };

    MessageQueue mOutputQueue;
  };
} // namespace face
