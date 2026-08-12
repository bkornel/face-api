#pragma once

#include "Framework/Messaging/Message.h"

#include <cstdint>
#include <opencv2/core/core.hpp>

namespace face
{
  class CommandMessage : public fw::Message
  {
  public:

    enum class Type
    {
      Invalid = -1,
      RunFaceDetection,
      VerboseModeChanged
    };

    CommandMessage(Type iType, uint32_t iFrameId, fw::Timestamp iTimestamp);

    virtual ~CommandMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const CommandMessage& iMessage);

    Type GetType() const
    {
      return mType;
    }

    inline bool IsValid() const
    {
      return mType != Type::Invalid;
    }

  private:
    Type mType = Type::Invalid;
  };

  inline std::ostream& operator<<(std::ostream& ioStream, const CommandMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Type: " << static_cast<int>(iMessage.GetType());
    return ioStream;
  }
}
