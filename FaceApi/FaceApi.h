#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "Configuration.h"
#include "Framework/ErrorCode.h"
#include "FaceResult.h"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Graph/Module.h"
#include "Framework/Thread.h"
#include "Modules/FacePipeline.h"

namespace face
{
  /// @brief One face pipeline and the worker thread that drives it. Instantiable: whoever
  /// creates it owns its lifetime, and nothing here - the configuration included - is
  /// shared between instances.
  class FaceApi : public fw::Module,
                  public fw::Thread
  {
    using MessageQueue = fw::MessageQueue<std::shared_ptr<ImageMessage>>;

  public:
    FaceApi();

    FaceApi(const FaceApi& iOther) = delete;

    ~FaceApi() override;

    FaceApi& operator=(const FaceApi& iOther) = delete;

    /// @return What the image queue said: OK when the frame went in, OutOfResources when
    /// the queue was full, BadData when it came faster than the sampling rate allows. A
    /// frame that was not OK will never come out of the pipeline.
    fw::ErrorCode PushCameraFrame(const cv::Mat& iFrame);

    fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

    // The overlay data of the last processed frame, for hosts that draw it themselves.
    // Needs a userManager module in the graph, see settings.json.
    fw::ErrorCode GetResults(FaceResults& oResults) const;

    /// @brief How many frames are waiting in the image queue right now
    int GetQueueSize() const;

    /// @brief The parsed settings.json, for hosts that read their own values out of it
    inline const Configuration& GetConfiguration() const
    {
      return mConfiguration;
    }

    void Clear() override;

    void SetRunFaceDetector();

    void SetVerbose(bool iVerbose);

    void OnOffVerbose();

    inline uint32_t GetLastFrameId() const
    {
      return mPipeline ? mPipeline->GetLastFrameId() : 0U;
    }

    inline int64_t GetLastTimestamp() const
    {
      return mPipeline ? mPipeline->GetLastTimestamp() : 0LL;
    }

    // The working directory - where settings.json and the model files are - comes from
    // fw::Module::SetWorkingDirectory(), called before Initialize(). Initialize() reads
    // the settings from there, and does so again on every re-initialization, which is how
    // a saved settings file takes effect on a pipeline reload.

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode Run() override;

    void OnFrameProcessed(std::shared_ptr<ImageMessage> iMessage);

    /// @brief Serialises one frame of the graph against the calls that reset it
    std::mutex mProcessMutex;

    Configuration mConfiguration;

    FacePipeline::FrameProcessedToken mFrameProcessedToken = 0ULL;

    std::shared_ptr<FacePipeline> mPipeline = nullptr;

    std::atomic<uint32_t> mCameraFrameId{ 0U };

    std::atomic<bool> mVerbose{ false };

    MessageQueue mOutputQueue;
  };
} // namespace face
