#pragma once

#include <atomic>
#include <string>

#include "FaceResult.h"
#include "Framework/MessageBus.h"
#include "Framework/MessageQueue.hpp"
#include "Framework/Module.h"
#include "Modules/ModuleGraph.h"

namespace face
{
  class FaceApi : public fw::Module
  {
    using MessageQueue = fw::MessageQueue<ImageMessage::Shared>;

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

    void OnFrameProcessed(ImageMessage::Shared iMessage);

    fw::MessageBus mBus;

    ModuleGraph::Shared mModuleGraph = nullptr;

    std::atomic<unsigned> mCameraFrameId{ 0U };

    MessageQueue mOutputQueue;
  };
} // namespace face
