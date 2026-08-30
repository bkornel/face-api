#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace fw
{
  template <typename T>
  class Event;

  /// @brief A typed, direct callback list: whoever raises it knows exactly who listens for
  /// what. It is what replaced the type-indexed message bus - a connection is made once, at
  /// wiring time, instead of being discovered per message with a cast chain.
  ///
  /// Subscribing hands back a token and unsubscribing takes it. The previous version stored
  /// std::function-like delegates and removed them by comparing the pair of pointers inside,
  /// which is why it needed a hand-written Delegate class and the MAKE_DELEGATE macro: a
  /// std::function cannot be compared. A token needs no comparison, so a subscriber can be a
  /// lambda that captures whatever it likes.
  ///
  /// This class is thread-safe. Handlers run on the raising thread.
  template <typename... ArgumentT>
  class Event<void(ArgumentT...)>
  {
  public:
    using Handler = std::function<void(ArgumentT...)>;
    using Token = uint64_t;

    static constexpr Token cInvalidToken = 0ULL;

    Event() = default;

    Event(const Event& iOther) = delete;

    Event& operator=(const Event& iOther) = delete;

    /// @return the token to unsubscribe with, or cInvalidToken if iHandler is empty
    Token Subscribe(Handler iHandler)
    {
      if (!iHandler) return cInvalidToken;

      std::lock_guard<std::mutex> lock(mMutex);

      const Token token = mNextToken++;
      mSubscribers.emplace_back(token, std::move(iHandler));

      return token;
    }

    void Unsubscribe(Token iToken)
    {
      if (iToken == cInvalidToken) return;

      std::lock_guard<std::mutex> lock(mMutex);

      std::erase_if(mSubscribers, [iToken](const Subscriber& iObj) {
        return iObj.first == iToken;
      });
    }

    void Raise(ArgumentT... iArgument)
    {
      std::vector<Subscriber> subscribers;
      {
        std::lock_guard<std::mutex> lock(mMutex);
        subscribers = mSubscribers;
      }

      // Called with the lock released: a handler may subscribe or unsubscribe in turn
      for (const auto& subscriber : subscribers)
        subscriber.second(iArgument...);
    }

    void Clear()
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mSubscribers.clear();
    }

    std::size_t GetSubscriberCount() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mSubscribers.size();
    }

  private:
    using Subscriber = std::pair<Token, Handler>;

    mutable std::mutex mMutex;
    std::vector<Subscriber> mSubscribers;
    Token mNextToken = cInvalidToken + 1ULL;
  };
}
