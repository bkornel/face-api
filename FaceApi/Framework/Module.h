#pragma once

#include "Framework/Event.hpp"
#include "Framework/FlowGraph.hpp"
#include "Framework/Message.h"
#include "Framework/MessageBus.h"
#include "Framework/Thread.h"

#include <opencv2/core.hpp>

#include <mutex>
#include <string>
#include <memory>
#include <vector>

namespace fw
{
  class Module : public Thread
  {
  public:

    static std::string CreateModuleName(const cv::FileNode& iModuleNode);

    Module() = default;

    ~Module() override;

    // Must be called before Initialize(). iSelf may be empty for owners that are not held
    // by a shared_ptr, they are then responsible for outliving the bus.
    void Attach(MessageBus& ioBus, const std::shared_ptr<void>& iSelf);

    virtual ErrorCode Initialize(const cv::FileNode& iModuleNode);

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
    static const std::size_t sMaxPendingCommands;

    virtual ErrorCode InitializeInternal(const cv::FileNode& iModuleNode);

    virtual ErrorCode DeInitializeInternal();

    void Publish(const std::shared_ptr<Message>& iMessage);

    // Subscribes the module for a message type until DeInitialize()
    template <typename MessageT>
    void SubscribeCommand()
    {
      if (!mBus) return;

      const MessageBus::Token token =
        mSelf.expired()
          ? mBus->Subscribe<MessageT>([this](std::shared_ptr<Message> iMessage) { OnCommand(iMessage); })
          : mBus->Subscribe<MessageT>(mSelf.lock(), [this](std::shared_ptr<Message> iMessage) { OnCommand(iMessage); });

      if (token != MessageBus::sInvalidToken) mSubscriptions.emplace_back(token);
    }

    // Subscribes for the message types this module wants, called from Initialize()
    virtual void SubscribeCommands();

    // Called on the publishing thread, only queues the command
    virtual void OnCommand(std::shared_ptr<Message> iMessage);

    // Applies the queued commands, must be called from Main()
    void DrainCommands();

    // Called by DrainCommands() on the graph thread
    virtual void HandleCommand(std::shared_ptr<Message> iMessage);

    bool mInitialized = false;
    bool mVerboseMode = false;
    std::string mName;

    MessageBus* mBus = nullptr;
    std::weak_ptr<void> mSelf;

  private:
    void UnsubscribeCommands();

    std::mutex mCommandMutex;
    std::vector<std::shared_ptr<Message>> mPendingCommands;
    std::vector<MessageBus::Token> mSubscriptions;
  };
}
