#include "Framework/Module.h"
#include "Framework/UtilString.h"
#include "Messages/CommandMessage.h"
#include "Messages/ImageSizeChangedMessage.h"

#include <easyloggingpp/easyloggingpp.h>

namespace fw
{
  Module::CommandEventHandler Module::sCommand;

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
      sCommand += MAKE_DELEGATE(&Module::OnCommand, this);
    }

    return result;
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
      sCommand -= MAKE_DELEGATE(&Module::OnCommand, this);
    }

    return result;
  }

  void Module::Clear()
  {
    // There is nothing to clear here, override the method in the child classes
  }

  void Module::OnCommand(Message::Shared iMessage)
  {
    face::CommandMessage::Shared command = std::dynamic_pointer_cast<face::CommandMessage>(iMessage);
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
