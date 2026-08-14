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

    /// @brief Called once the whole graph is wired, for derived classes that want to look
    /// at the finished topology - the sinks, say
    virtual void OnGraphConnected();

    /// @brief A settings file may still name modules that no longer exist. Naming one is
    /// tolerated with a warning rather than failing the whole graph, and this says which
    /// names get that treatment. iModuleName is lower case.
    virtual bool IsObsoleteModule(const std::string& iModuleName) const;

    inline const std::vector<std::shared_ptr<Module>>& GetModules() const
    {
      return mModules;
    }

    /// @brief The modules whose output nobody consumes: where a frame is finished. Empty
    /// until the graph is connected.
    inline const std::vector<std::shared_ptr<Module>>& GetSinkModules() const
    {
      return mSinks;
    }

  private:
    ErrorCode CreateModules(const cv::FileNode& iModulesNode);

    ErrorCode CreateConnections(const cv::FileNode& iModulesNode);

    ErrorCode GetPredecessors(const cv::FileNode& iModule, PredecessorMap& oPredecessors);

    std::vector<cv::FileNode> GetConnectionOrder(const cv::FileNode& iModulesNode);

    std::vector<std::shared_ptr<Module>> mModules;
    std::vector<std::shared_ptr<Module>> mSinks;
  };
}
