#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Event.hpp"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

#include "Messages/ImageMessage.h"

#include "Modules/FirstModule/FirstModule.h"
#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/LastModule/LastModule.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace face
{
  class ModuleGraph : public fw::Module
  {
    using PredecessorMap = std::map<int, std::shared_ptr<fw::Module>>;
    using FrameProcessedHandler = fw::Event<void(std::shared_ptr<ImageMessage>)>;

  public:

    static FrameProcessedHandler sFrameProcessed;

    ModuleGraph() = default;

    ModuleGraph(const ModuleGraph& iOther) = delete;

    ~ModuleGraph() override;

    ModuleGraph& operator=(const ModuleGraph& iOther) = delete;

    void Clear() override;

    fw::ErrorCode Process();

    // The camera path: frames are handed to the queue directly, not broadcast
    inline std::shared_ptr<ImageQueue> GetImageQueue() const
    {
      return mImageQueue;
    }

    inline std::shared_ptr<LastModule> GetLastModule() const
    {
      return mLastModule;
    }

    inline unsigned GetLastFrameId() const
    {
      return mLastModule ? mLastModule->GetLastFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastModule ? mLastModule->GetLastTimestamp() : 0LL;
    }

  private:
    static const long long sProcessTimeoutMs;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode CreateModules(const cv::FileNode& iModulesNode);

    fw::ErrorCode CreateConnections(const cv::FileNode& iModulesNode);

    fw::ErrorCode GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors);

    std::vector<cv::FileNode> GetConnectionOrder(const cv::FileNode& iModulesNode);

    std::shared_ptr<FirstModule> mFirstModule = nullptr;
    std::shared_ptr<LastModule> mLastModule = nullptr;
    std::shared_ptr<ImageQueue> mImageQueue = nullptr;
    std::vector<std::shared_ptr<fw::Module>> mModules;
  };
}
