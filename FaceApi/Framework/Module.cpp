#include "Framework/Module.h"
#include "Messages/ImageSizeChangedMessage.h"

namespace fw
{
  Module::CommandEventHandler Module::sCommand;

  ErrorCode Module::Initialize(const cv::FileNode& iSettings)
  {
    ErrorCode result = ErrorCode::OK;

    // Initialize only of it is not initialized
    if (!mInitialized)
    {
      // set the module name
      if (!iSettings.empty())
      {
        const cv::FileNode& nameNode = iSettings["name"];
        mName = !nameNode.empty() ? nameNode.name() : iSettings.name();
      }

      Clear();
      result = InitializeInternal(iSettings);
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
    if (std::dynamic_pointer_cast<face::ImageSizeChangedMessage>(iMessage))
    {
      Clear();
    }
  }

  ErrorCode Module::InitializeInternal(const cv::FileNode& iSettings)
  {
    return fw::ErrorCode::OK;
  }

  ErrorCode Module::DeInitializeInternal()
  {
    return fw::ErrorCode::OK;
  }
}
