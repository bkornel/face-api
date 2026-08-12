#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/Event.hpp"
#include "Framework/Graph/ModuleGraph.h"

#include "Messages/ImageMessage.h"

#include "Modules/FirstModule/FirstModule.h"
#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/LastModule/LastModule.h"

#include <cstdint>
#include <memory>

namespace face
{
  /// @brief The face pipeline: fw::ModuleGraph builds and wires the graph, this class knows
  /// which modules exist (through the factory) and drives the graph one frame at a time.
  class ModuleGraph : public fw::ModuleGraph
  {
    using FrameProcessedHandler = fw::Event<void(std::shared_ptr<ImageMessage>)>;

  public:

    using FrameProcessedToken = FrameProcessedHandler::Token;

    ModuleGraph() = default;

    ~ModuleGraph() override;

    fw::ErrorCode Process();

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

    inline std::shared_ptr<LastModule> GetLastModule() const
    {
      return mLastModule;
    }

    inline uint32_t GetLastFrameId() const
    {
      return mLastModule ? mLastModule->GetLastFrameId() : 0U;
    }

    inline int64_t GetLastTimestamp() const
    {
      return mLastModule ? mLastModule->GetLastTimestamp() : 0LL;
    }

  private:
    static const int64_t sProcessTimeoutMs;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

    std::shared_ptr<fw::Module> CreateModule(const cv::FileNode& iModuleNode) override;

    void OnModuleCreated(const std::shared_ptr<fw::Module>& iModule) override;

    fw::ErrorCode ValidateModules() override;

    FrameProcessedHandler mFrameProcessed;

    std::shared_ptr<FirstModule> mFirstModule = nullptr;
    std::shared_ptr<LastModule> mLastModule = nullptr;
    std::shared_ptr<ImageQueue> mImageQueue = nullptr;
  };
}
