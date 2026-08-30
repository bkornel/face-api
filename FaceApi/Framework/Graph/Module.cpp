#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <utility>

namespace fw
{
  const std::size_t Module::sMaxPendingCommands = 64U;

  Module::~Module()
  {
    // A failed init never reaches DeInitialize, so the subscriptions are dropped here too
    DropSubscriptions();
  }

  std::string Module::CreateModuleName(const cv::FileNode& iModuleNode)
  {
    CV_DbgAssert(!iModuleNode.empty() && iModuleNode.isNamed());

    std::string moduleName = fw::str::trim(iModuleNode.name());

    const cv::FileNode& instanceNode = iModuleNode["instance"];
    if (!instanceNode.empty())
    {
      moduleName += "." + fw::str::trim(instanceNode.string());
    }

    return moduleName;
  }

  ErrorCode Module::Initialize(const cv::FileNode& iModuleNode)
  {
    ErrorCode result = ErrorCode::OK;

    // Initialize only of it is not initialized
    if (!mInitialized)
    {
      // set the module name
      if (!iModuleNode.empty() && iModuleNode.isNamed())
      {
        mName = Module::CreateModuleName(iModuleNode);
      }

      Clear();

      if ((result = InitializeInternal(iModuleNode)) != fw::ErrorCode::OK)
      {
        LOG(ERROR) << "Error during initializing module: " << mName;
      }

      mInitialized = (result == ErrorCode::OK);
    }

    return result;
  }

  ErrorCode Module::DeInitialize()
  {
    ErrorCode result = ErrorCode::OK;

    if (mInitialized)
    {
      // First: a signal raised from here on would otherwise queue work on a module that
      // no longer runs
      DropSubscriptions();

      result = DeInitializeInternal();
      Clear();

      mInitialized = false;

      std::lock_guard<std::mutex> lock(mCommandMutex);
      mPendingCommands.clear();
    }

    return result;
  }

  void Module::DropSubscriptions()
  {
    for (const auto& unsubscribe : mUnsubscribers)
      unsubscribe();

    mUnsubscribers.clear();
  }

  void Module::Clear()
  {
    // There is nothing to clear here, override the method in the child classes
  }

  void Module::PostCommand(std::function<void()> iCommand)
  {
    // Runs on the posting thread, so the command is only queued here and applied later
    // from Main(). Module state stays owned by the graph thread this way.
    if (!iCommand) return;

    std::lock_guard<std::mutex> lock(mCommandMutex);

    if (mPendingCommands.size() >= sMaxPendingCommands)
    {
      LOG(WARNING) << "Command queue of " << mName << " is full, dropping the command.";
      return;
    }

    mPendingCommands.emplace_back(std::move(iCommand));
  }

  void Module::DrainCommands()
  {
    std::vector<std::function<void()>> commands;
    {
      std::lock_guard<std::mutex> lock(mCommandMutex);
      commands.swap(mPendingCommands);
    }

    for (const auto& command : commands)
      command();
  }

  ErrorCode Module::InitializeInternal(const cv::FileNode& iModuleNode)
  {
    return fw::ErrorCode::OK;
  }

  ErrorCode Module::DeInitializeInternal()
  {
    return fw::ErrorCode::OK;
  }
}
