#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/Event.hpp"

#include <opencv2/core.hpp>

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace fw
{
  /// @brief A node of the module graph. A module does not own a thread: where its Main() runs
  /// is the executor's business, chosen per module through fw::IPortConnector::SetExecutor().
  ///
  /// The framework knows nothing about what a module reacts to. Whoever builds the graph
  /// wires the module to its signals with Listen() - once, at creation - and a handler must
  /// only PostCommand() the work, which Main() applies through DrainCommands(). Module state
  /// stays owned by the thread that runs it this way, and every subscription is dropped when
  /// the module is deinitialized or destroyed.
  class Module
  {
  public:

    static std::string CreateModuleName(const cv::FileNode& iModuleNode);

    Module() = default;

    Module(const Module& iOther) = delete;

    virtual ~Module();

    Module& operator=(const Module& iOther) = delete;

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

    /// @brief The directory relative paths in the module's settings resolve against.
    /// Set it before Initialize().
    void SetWorkingDirectory(const std::string& iWorkingDirectory)
    {
      mWorkingDirectory = iWorkingDirectory;
    }

    inline const std::string& GetWorkingDirectory() const
    {
      return mWorkingDirectory;
    }

    /// @brief Applied through a posted command by whoever wires the module, so the flag
    /// changes on the graph thread like the rest of the module's state
    void SetVerboseMode(bool iVerbose)
    {
      mVerboseMode = iVerbose;
    }

    /// @brief Subscribes iHandler to ioEvent and remembers how to undo it: DeInitialize()
    /// and the destructor drop every subscription, so a signal can never reach a module
    /// that no longer runs. ioEvent must outlive the subscription.
    template <typename SignatureT>
    void Listen(Event<SignatureT>& ioEvent, typename Event<SignatureT>::Handler iHandler)
    {
      const auto token = ioEvent.Subscribe(std::move(iHandler));
      mUnsubscribers.emplace_back([&ioEvent, token] { ioEvent.Unsubscribe(token); });
    }

    /// @brief Queues iCommand from any thread; DrainCommands() applies it later. Full queues
    /// drop the command with a warning, so a stalled module cannot hoard work forever.
    void PostCommand(std::function<void()> iCommand);

  protected:
    static const std::size_t sMaxPendingCommands;

    virtual ErrorCode InitializeInternal(const cv::FileNode& iModuleNode);

    virtual ErrorCode DeInitializeInternal();

    /// @brief Applies the queued commands, must be called from Main()
    void DrainCommands();

    bool mInitialized = false;
    bool mVerboseMode = false;
    std::string mName;
    std::string mWorkingDirectory;

  private:
    void DropSubscriptions();

    std::vector<std::function<void()>> mUnsubscribers;

    std::mutex mCommandMutex;
    std::vector<std::function<void()>> mPendingCommands;
  };
}
