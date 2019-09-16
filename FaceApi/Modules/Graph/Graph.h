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
  class Graph :
    public fw::Module
  {
    using PredecessorMap = std::map<int, fw::Module::Shared>;
    using FrameProcessedHandler = fw::Event<void(ImageMessage::Shared)>;

  public:
    FW_DEFINE_SMART_POINTERS(Graph);

    static FrameProcessedHandler sFrameProcessed;

    Graph() = default;

    Graph(const Graph& iOther) = delete;

    ~Graph() override;

    Graph& operator=(const Graph& iOther) = delete;

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

    FirstModule::Shared mFirstModule = nullptr;
    LastModule::Shared mLastModule = nullptr;
    std::vector<fw::Module::Shared> mModules;
  };
}
