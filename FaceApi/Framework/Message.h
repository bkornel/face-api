#pragma once

#include "Framework/Util.h"

#include <iostream>
#include <memory>

namespace fw
{
  class Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(Message);

    Message(unsigned iFrameId, long long iTimestamp);

    virtual ~Message() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const Message& iMessage);

    inline unsigned GetFrameId() const
    {
      return mFrameId;
    }

    inline long long GetTimestamp() const
    {
      return mTimestamp;
    }

  private:
    Message(const Message& iOther) = delete;

    Message& operator=(const Message& iOther) = delete;

    unsigned mFrameId = 0U;
    long long mTimestamp = 0;
  };

  inline std::ostream& operator<< (std::ostream& ioStream, const Message& iMessage)
  {
    ioStream << "[Base] Frame ID: " << iMessage.mFrameId << ", timestamp: " << iMessage.mTimestamp;
    return ioStream;
  }
}
