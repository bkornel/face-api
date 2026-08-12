#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/ModuleConnector.h"

#include <opencv2/core.hpp>

#include <memory>
#include <string>
#include <vector>

namespace fw
{
  /// @brief Builds and owns a graph of modules described by a settings node: creates them,
  /// orders them by their depth and wires their ports together. What kinds of module exist
  /// is the one thing it cannot know, so a derived class supplies CreateModule() and may
  /// pick out the modules it needs by role in OnModuleCreated().
  class ModuleGraph : public Module
  {
  public:
    using PredecessorMap = ModuleConnector::PredecessorMap;

    ModuleGraph() = default;

    ModuleGraph(const ModuleGraph& iOther) = delete;

    ~ModuleGraph() override = default;

    ModuleGraph& operator=(const ModuleGraph& iOther) = delete;

    void Clear() override;

  protected:
    ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

    ErrorCode DeInitializeInternal() override;

    /// @brief Creates a module from its settings node, or empty if the name is unknown
    virtual std::shared_ptr<Module> CreateModule(const cv::FileNode& iModuleNode) = 0;

    /// @brief Called for every created module, before the graph is connected
    virtual void OnModuleCreated(const std::shared_ptr<Module>& iModule);

    /// @brief Called once every module exists, before the graph is connected
    virtual ErrorCode ValidateModules();

    inline const std::vector<std::shared_ptr<Module>>& GetModules() const
    {
      return mModules;
    }

  private:
    ErrorCode CreateModules(const cv::FileNode& iModulesNode);

    ErrorCode CreateConnections(const cv::FileNode& iModulesNode);

    ErrorCode GetPredecessors(const cv::FileNode& iModule, PredecessorMap& oPredecessors);

    std::vector<cv::FileNode> GetConnectionOrder(const cv::FileNode& iModulesNode);

    std::vector<std::shared_ptr<Module>> mModules;
  };
}
