#pragma once

#include "Common/Configuration.h"
#include "Framework/MessageQueue.hpp"
#include "Framework/Module.h"
#include "Graph/Graph.h"

#include <string>
#include <vector>

namespace face
{
  class FaceApi :
    public fw::Module
  {
    typedef fw::MessageQueue<ImageMessage::Shared> MessageQueue;

  public:
    static FaceApi& GetInstance();

    virtual ~FaceApi();

    void PushCameraFrame(const cv::Mat& iFrame);

    fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

    void Clear() override;

    void SetRunFaceDetector();

    void OnOffVerbose();

    inline unsigned GetLastFrameId() const
    {
      return mGraph ? mGraph->GetLastFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mGraph ? mGraph->GetLastTimestamp() : 0LL;
    }

    void SetWorkingDirectory(const std::string& iWorkingDirectory);

  private:
    static std::recursive_mutex sAppMutex;		///< The mutex to lock critical sections

    FaceApi();

    FaceApi(const FaceApi& iOther) = delete;

    FaceApi& operator=(const FaceApi& iOther) = delete;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    void OnFrameProcessed(ImageMessage::Shared iMessage);

    Graph::Shared mGraph = nullptr;
    unsigned mCameraFrameId = 0U;
    MessageQueue mOutputQueue;
  };
}
