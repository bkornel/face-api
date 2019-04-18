#pragma once

#include "Framework/Event.hpp"
#include "Framework/FlowGraph.hpp"
#include "Framework/Message.h"
#include "Framework/Thread.h"

#include <opencv2/core.hpp>

#include <string>
#include <memory>

namespace fw
{
  class Module :
    public Thread
  {
  public:
    FW_DEFINE_SMART_POINTERS(Module);

    Module() = default;

    virtual ~Module() = default;

    virtual ErrorCode Initialize(const cv::FileNode& iSettings);

    virtual ErrorCode DeInitialize();

    // Should be deleted, now it's only called when the frame size has changed
    // This is going to be substituted with events
    virtual void Clear();

    inline bool IsInitialized() const
    {
      return mInitialized;
    }

    inline const std::string& GetName() const
    {
      return mName;
    }

  protected:
    using CommandEventHandler = Event<void(Message::Shared)>;

    static CommandEventHandler sCommand;

    virtual ErrorCode InitializeInternal(const cv::FileNode& iSettings);

    virtual ErrorCode DeInitializeInternal();

    virtual void OnCommand(Message::Shared iMessage);

    bool mInitialized = false;
    std::string mName;
  };
}
