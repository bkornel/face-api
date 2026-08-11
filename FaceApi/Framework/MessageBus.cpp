#include "Framework/MessageBus.h"

#include <algorithm>

namespace fw
{
  const MessageBus::Token MessageBus::sInvalidToken = 0ULL;

  MessageBus::Token MessageBus::Subscribe(std::type_index iType, Handler iHandler, std::weak_ptr<void> iOwner, bool iTracked)
  {
    if (!iHandler) return sInvalidToken;

    std::lock_guard<std::mutex> lock(mMutex);

    Subscription subscription;
    subscription.token = mNextToken++;
    subscription.handler = std::move(iHandler);
    subscription.owner = std::move(iOwner);
    subscription.tracked = iTracked;

    mSubscriptions[iType].emplace_back(std::move(subscription));

    return mSubscriptions[iType].back().token;
  }

  void MessageBus::Unsubscribe(Token iToken)
  {
    if (iToken == sInvalidToken) return;

    std::lock_guard<std::mutex> lock(mMutex);

    for (auto it = mSubscriptions.begin(); it != mSubscriptions.end();)
    {
      auto& subscriptions = it->second;

      subscriptions.erase(
        std::remove_if(subscriptions.begin(), subscriptions.end(), [iToken](const Subscription& iObj) {
          return iObj.token == iToken;
        }),
        subscriptions.end()
      );

      it = subscriptions.empty() ? mSubscriptions.erase(it) : std::next(it);
    }
  }

  void MessageBus::Publish(const Message::Shared& iMessage)
  {
    if (!iMessage) return;

    // The dynamic type decides where the message goes, Message is polymorphic
    const std::type_index type(typeid(*iMessage));

    std::vector<Handler> handlers;
    {
      std::lock_guard<std::mutex> lock(mMutex);

      auto it = mSubscriptions.find(type);
      if (it == mSubscriptions.end()) return;

      auto& subscriptions = it->second;

      // Owners that are gone take their subscription with them
      subscriptions.erase(
        std::remove_if(subscriptions.begin(), subscriptions.end(), [](const Subscription& iObj) {
          return iObj.tracked && iObj.owner.expired();
        }),
        subscriptions.end()
      );

      handlers.reserve(subscriptions.size());
      for (const auto& subscription : subscriptions)
        handlers.emplace_back(subscription.handler);

      if (subscriptions.empty()) mSubscriptions.erase(it);
    }

    // Called with the lock released: a handler is allowed to publish in turn
    for (const auto& handler : handlers)
      handler(iMessage);
  }

  std::size_t MessageBus::GetSubscriptionCount() const
  {
    std::lock_guard<std::mutex> lock(mMutex);

    std::size_t count = 0U;
    for (const auto& s : mSubscriptions)
      count += s.second.size();

    return count;
  }
}
