#pragma once

#include "Common/Configuration.h"
#include "Framework/MessageQueue.hpp"
#include "Framework/Module.h"
#include "Graph/Graph.h"
#include "Modules/General/FirstModule.h"
#include "Modules/General/LastModule.h"

#include <string>
#include <vector>

namespace face
{
  class ImageQueue;
  class FaceDetection;
  class UserHistory;
  class UserManager;
  class UserProcessor;
  class Visualizer;

  class FaceApi :
    public fw::Module
  {
    typedef fw::MessageQueue<ImageMessage::Shared> MessageQueue;

  public:
    static FaceApi& GetInstance();

    virtual ~FaceApi();

    fw::ErrorCode PushCameraFrame(const cv::Mat& iFrame);

    fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

    void Clear() override;

    inline void SetRunFaceDetector()
    {
      if (mFirstModule) mFirstModule->RunFaceDetector();;
    }

    inline void OnOffVerbose()
    {
      static bool verbose = Configuration::GetInstance().GetOutput().verbose;
      verbose = !verbose;
      Configuration::GetInstance().SetVerboseMode(verbose);
    }

    inline unsigned GetLastFrameId() const
    {
      return mLastModule ? mLastModule->GetLastFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastModule ? mLastModule->GetLastTimestamp() : 0LL;
    }

    void SetWorkingDirectory(const std::string& iWorkingDirectory)
    {
      Configuration::GetInstance().SetWorkingDirectory(iWorkingDirectory);
    }

  private:
    static std::recursive_mutex sAppMutex;		///< The mutex to lock critical sections

    FaceApi();

    FaceApi(const FaceApi& iOther) = delete;

    FaceApi& operator=(const FaceApi& iOther) = delete;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    fw::ErrorCode CreateModules();

    fw::ErrorCode CreateConnections();

    std::vector<fw::Module*> mModules;
    std::shared_ptr<Graph> mGraph = nullptr;

    FirstModule* mFirstModule = nullptr;
    LastModule* mLastModule = nullptr;
    ImageQueue* mImageQueue = nullptr;
    FaceDetection* mFaceDetection = nullptr;
    UserHistory* mUserHistory = nullptr;
    UserManager* mUserManager = nullptr;
    UserProcessor* mUserProcessor = nullptr;
    Visualizer* mVisualizer = nullptr;

    MessageQueue mOutputQueue;
  };
}
