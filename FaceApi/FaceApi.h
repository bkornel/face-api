#pragma once

#include "Framework/MessageQueue.hpp"
#include "Framework/Module.h"

#include "Modules/ModuleGraph.h"

#include <string>

namespace face
{
  class FaceApi :
    public fw::Module
  {
    using MessageQueue = fw::MessageQueue<ImageMessage::Shared>;

  public:
    static FaceApi& GetInstance();

    FaceApi(const FaceApi& iOther) = delete;

    ~FaceApi() override;

    FaceApi& operator=(const FaceApi& iOther) = delete;

    void PushCameraFrame(const cv::Mat& iFrame);

    fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

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
    static std::recursive_mutex sAppMutex;		///< The mutex to lock critical sections

    FaceApi();

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    void OnFrameProcessed(ImageMessage::Shared iMessage);

    ModuleGraph::Shared mModuleGraph = nullptr;
    unsigned mCameraFrameId = 0U;
    MessageQueue mOutputQueue;
  };
}
