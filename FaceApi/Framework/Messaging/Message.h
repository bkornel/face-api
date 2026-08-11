#pragma once


#include "Framework/TimeExtensions.h"
#include <iostream>
#include <memory>

namespace fw
{
  class Message
  {
  public:

    Message(unsigned iFrameId, Timestamp iTimestamp);

    Message(const Message& iOther) = delete;

    virtual ~Message() = default;

    Message& operator=(const Message& iOther) = delete;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const Message& iMessage);

    inline unsigned GetFrameId() const
    {
      return mFrameId;
    }

    inline Timestamp GetTimestamp() const
    {
      return mTimestamp;
    }

  private:
    unsigned mFrameId = 0U;
    Timestamp mTimestamp;
  };

  inline std::ostream& operator<<(std::ostream& ioStream, const Message& iMessage)
  {
    ioStream << "[Base] Frame ID: " << iMessage.mFrameId << ", timestamp: " << to_epoch_ms(iMessage.mTimestamp);
    return ioStream;
  }
}
