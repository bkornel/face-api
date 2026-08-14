#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/Event.hpp"
#include "Framework/Graph/ModuleGraph.h"

#include "FaceResult.h"
#include "Messages/ImageMessage.h"
#include "Messages/UserSnapshotMessage.h"

#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/Visualizer/Visualizer.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace face
{
  /// @brief The pipeline's control signals, one fw::Event per signal. Every connection is
  /// made once, by FacePipeline, when a module is created - so who reacts to what is decided
  /// at wiring time rather than discovered per message. Raising is synchronous and may come
  /// from any thread; a handler only posts the work to the module it belongs to.
  struct PipelineEvents
  {
    /// @brief Asks the detector to run at its next opportunity
    fw::Event<void()> runFaceDetection;

    /// @brief Carries the value rather than asking for a flip: a module that missed one or
    /// handled it twice would otherwise be left inverted for the rest of the run
    fw::Event<void(bool)> verboseChanged;

    /// @brief Raised by the image queue when the size of the incoming frames changes
    fw::Event<void(cv::Size)> imageSizeChanged;
  };

  /// @brief The face pipeline: knows which modules exist, creates them, wires them to the
  /// control signals, and drives the graph one frame at a time.
  ///
  /// The graph needs no designated first or last module. The image queue is the source -
  /// Trigger() starts a frame - and a frame is finished when every sink (a module whose
  /// output nothing consumes) has published. What the host reads back - the drawn frame,
  /// the per-user results - is collected here after the barrier, from whichever modules
  /// the settings wired in.
  class FacePipeline : public fw::ModuleGraph
  {
    using FrameProcessedHandler = fw::Event<void(std::shared_ptr<ImageMessage>)>;

  public:

    using FrameProcessedToken = FrameProcessedHandler::Token;

    FacePipeline() = default;

    ~FacePipeline() override;

    /// @brief What relative paths in module settings resolve against; set before Initialize()
    void SetWorkingDirectory(const std::string& iWorkingDirectory)
    {
      mWorkingDirectory = iWorkingDirectory;
    }

    fw::ErrorCode Process();

    void Clear() override;

    inline PipelineEvents& GetEvents()
    {
      return mEvents;
    }

    FrameProcessedToken SubscribeFrameProcessed(FrameProcessedHandler::Handler iHandler)
    {
      return mFrameProcessed.Subscribe(std::move(iHandler));
    }

    void UnsubscribeFrameProcessed(FrameProcessedToken iToken)
    {
      mFrameProcessed.Unsubscribe(iToken);
    }

    // The camera path: frames are handed to the queue directly, not broadcast
    inline std::shared_ptr<ImageQueue> GetImageQueue() const
    {
      return mImageQueue;
    }

    /// @brief The overlay data of the last finished frame, for hosts drawing it themselves
    fw::ErrorCode GetLastResults(FaceResults& oResults) const;

    uint32_t GetLastFrameId() const;

    int64_t GetLastTimestamp() const;

  private:
    static const int64_t sProcessTimeoutMs;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

    /// @brief The factory: creates the module iModuleNode names, initializes it and wires
    /// it to the pipeline's signals. Extend it here when adding a new module.
    std::shared_ptr<fw::Module> CreateModule(const cv::FileNode& iModuleNode) override;

    bool IsObsoleteModule(const std::string& iModuleName) const override;

    void OnModuleCreated(const std::shared_ptr<fw::Module>& iModule) override;

    fw::ErrorCode ValidateModules() override;

    void OnGraphConnected() override;

    /// @brief Reads what this frame produced from the wired modules and remembers it for
    /// the host; called once the barrier has passed
    void CollectFrameOutput();

    /// @brief Outlives the modules: DeInitialize() drops their subscriptions before the
    /// base lets go of them, so nothing is ever raised into a dead module
    PipelineEvents mEvents;

    FrameProcessedHandler mFrameProcessed;

    std::string mWorkingDirectory;

    std::shared_ptr<ImageQueue> mImageQueue = nullptr;
    std::shared_ptr<UserManager> mUserManager = nullptr;
    std::shared_ptr<Visualizer> mVisualizer = nullptr;

    /// @brief The output ports a frame is finished on, one per sink module
    std::vector<std::shared_ptr<fw::IFuture>> mBarrier;

    /// @brief The last finished frame, read from the app thread through the getters
    mutable std::mutex mLastMutex;
    std::shared_ptr<ImageMessage> mLastImage = nullptr;
    std::shared_ptr<UserSnapshotMessage> mLastUsers = nullptr;
  };
}
