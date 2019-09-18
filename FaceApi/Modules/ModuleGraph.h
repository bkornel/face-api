#pragma once

#include "Framework/Event.hpp"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

#include "Messages/ImageMessage.h"

#include "Modules/FirstModule/FirstModule.h"
#include "Modules/LastModule/LastModule.h"

#include <map>
#include <string>
#include <vector>

namespace face
{
  class ModuleGraph :
    public fw::Module
  {
    using PredecessorMap = std::map<int, fw::Module::Shared>;
    using FrameProcessedHandler = fw::Event<void(ImageMessage::Shared)>;

  public:
    FW_DEFINE_SMART_POINTERS(ModuleGraph);

    static FrameProcessedHandler sFrameProcessed;

    ModuleGraph() = default;

    ModuleGraph(const ModuleGraph& iOther) = delete;

    ~ModuleGraph() override;

    ModuleGraph& operator=(const ModuleGraph& iOther) = delete;

    void Clear() override;

    fw::ErrorCode Process();

    inline unsigned GetLastFrameId() const
    {
      return mLastModule ? mLastModule->GetLastFrameId() : 0U;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastModule ? mLastModule->GetLastTimestamp() : 0LL;
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

    fw::ErrorCode DeInitializeInternal() override;

    fw::ErrorCode CreateModules(const cv::FileNode& iModulesNode);

    fw::ErrorCode CreateConnections(const cv::FileNode& iModulesNode);

    fw::ErrorCode GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors);

    std::vector<cv::FileNode> GetConnectionOrder(const cv::FileNode& iModulesNode);

    FirstModule::Shared mFirstModule = nullptr;
    LastModule::Shared mLastModule = nullptr;
    std::vector<fw::Module::Shared> mModules;
  };
}
