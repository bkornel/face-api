#pragma once

#include "Framework/Message.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <vector>

namespace fw
{
  /// @brief Delivers control messages to the modules that asked for them.
  ///
  /// Subscriptions are per concrete message type: publishing looks the type up once and
  /// calls only its subscribers, so a module is never woken for a message it ignores.
  /// Matching is on the exact type, subscribing to a base class does not catch derived
  /// messages.
  ///
  /// Handlers run on the publishing thread. They are expected to be cheap and to hand the
  /// message over to the owning thread, which is what fw::Module does.
  ///
  /// This class is thread-safe.
  class MessageBus
  {
  public:

    using Handler = std::function<void(std::shared_ptr<Message>)>;
    using Token = unsigned long long;

    static const Token sInvalidToken;

    MessageBus() = default;

    MessageBus(const MessageBus& iOther) = delete;

    MessageBus& operator=(const MessageBus& iOther) = delete;

    /// @brief Subscribes for MessageT for as long as iOwner is alive. The subscription
    /// drops itself once iOwner is gone, so an owner that forgets to unsubscribe cannot
    /// leave a dangling call behind.
    template <typename MessageT>
    Token Subscribe(const std::shared_ptr<void>& iOwner, Handler iHandler)
    {
      return Subscribe(std::type_index(typeid(MessageT)), std::move(iHandler), iOwner, true);
    }

    /// @brief Subscribes for MessageT without an owner to track. For subscribers whose
    /// lifetime is not managed by a shared_ptr, they must Unsubscribe() themselves.
    template <typename MessageT>
    Token Subscribe(Handler iHandler)
    {
      return Subscribe(std::type_index(typeid(MessageT)), std::move(iHandler), std::weak_ptr<void>(), false);
    }

    void Unsubscribe(Token iToken);

    void Publish(const std::shared_ptr<Message>& iMessage);

    std::size_t GetSubscriptionCount() const;

  private:
    struct Subscription
    {
      Token token = 0ULL;
      Handler handler;
      std::weak_ptr<void> owner;
      bool tracked = false;
    };

    Token Subscribe(std::type_index iType, Handler iHandler, std::weak_ptr<void> iOwner, bool iTracked);

    mutable std::mutex mMutex;
    std::map<std::type_index, std::vector<Subscription>> mSubscriptions;
    Token mNextToken = 1ULL;
  };
}
