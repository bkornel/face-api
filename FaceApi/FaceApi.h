#pragma once

#include <atomic>
#include <memory>
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

    void OnOffVerbose();

    inline unsigned GetLastFrameId() const
    {
      return mModuleGraph ? mModuleGraph->GetLastFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mModuleGraph ? mModuleGraph->GetLastTimestamp() : 0LL;
    }

    void SetWorkingDirectory(const std::string& iWorkingDirectory);

  private:
    static std::recursive_mutex sAppMutex; ///< The mutex to lock critical sections

    FaceApi();

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    void OnFrameProcessed(std::shared_ptr<ImageMessage> iMessage);

    fw::MessageBus mBus;

    ModuleGraph::FrameProcessedToken mFrameProcessedToken = 0ULL;

    std::shared_ptr<ModuleGraph> mModuleGraph = nullptr;

    std::atomic<unsigned> mCameraFrameId{ 0U };

    MessageQueue mOutputQueue;
  };
} // namespace face
