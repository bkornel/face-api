#include "Framework/ErrorCode.h"
#include "Framework/Module.h"
#include "Framework/Text.h"
#include "Messages/CommandMessage.h"
#include "Messages/ImageSizeChangedMessage.h"

#include <easyloggingpp/easyloggingpp.h>

namespace fw
{
  const std::size_t Module::sMaxPendingCommands = 64U;

  Module::~Module()
  {
    // A failed init never reaches DeInitialize, so the subscriptions are dropped here too
    UnsubscribeCommands();
  }

  void Module::Attach(MessageBus& ioBus, const std::shared_ptr<void>& iSelf)
  {
    mBus = &ioBus;
    mSelf = iSelf;
  }

  void Module::Publish(const std::shared_ptr<Message>& iMessage)
  {
    if (!mBus)
    {
      LOG(ERROR) << "Module " << mName << " is not attached to a message bus.";
      return;
    }

    mBus->Publish(iMessage);
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

      // Only listen for commands once the module is actually usable.
      if (mInitialized)
      {
        SubscribeCommands();
      }
    }

    return result;
  }

  void Module::SubscribeCommands()
  {
    SubscribeCommand<face::CommandMessage>();
    SubscribeCommand<face::ImageSizeChangedMessage>();
  }

  void Module::UnsubscribeCommands()
  {
    if (mBus)
    {
      for (const MessageBus::Token token : mSubscriptions)
        mBus->Unsubscribe(token);
    }

    mSubscriptions.clear();
  }

  ErrorCode Module::DeInitialize()
  {
    ErrorCode result = ErrorCode::OK;

    if (mInitialized)
    {
      if (IsRunning())
      {
        result = StopThread();
      }

      DeInitializeInternal();
      Clear();

      mInitialized = false;
      UnsubscribeCommands();

      std::lock_guard<std::mutex> lock(mCommandMutex);
      mPendingCommands.clear();
    }

    return result;
  }

  void Module::Clear()
  {
    // There is nothing to clear here, override the method in the child classes
  }

  void Module::OnCommand(std::shared_ptr<Message> iMessage)
  {
    // Runs on the publishing thread, so the command is only queued here and applied later
    // from Main(). Module state stays owned by the graph thread this way. No type check is
    // needed, the bus only delivers the types this module subscribed for.
    if (!iMessage) return;

    std::lock_guard<std::mutex> lock(mCommandMutex);

    if (mPendingCommands.size() >= sMaxPendingCommands)
    {
      LOG(WARNING) << "Command queue of " << mName << " is full, dropping the command.";
      return;
    }

    mPendingCommands.emplace_back(iMessage);
  }

  void Module::DrainCommands()
  {
    std::vector<std::shared_ptr<Message>> commands;
    {
      std::lock_guard<std::mutex> lock(mCommandMutex);
      commands.swap(mPendingCommands);
    }

    for (const auto& command : commands)
      HandleCommand(command);
  }

  void Module::HandleCommand(std::shared_ptr<Message> iMessage)
  {
    std::shared_ptr<face::CommandMessage> command = std::dynamic_pointer_cast<face::CommandMessage>(iMessage);
    if (command)
    {
      if (command->GetType() == face::CommandMessage::Type::VerboseModeChanged)
      {
        mVerboseMode = !mVerboseMode;
      }

      return;
    }

    if (std::dynamic_pointer_cast<face::ImageSizeChangedMessage>(iMessage))
    {
      Clear();
      return;
    }
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
