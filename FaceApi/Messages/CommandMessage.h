#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

namespace face
{
  class CommandMessage :
    public fw::Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(CommandMessage);

    enum class Type
    {
      Invalid = -1,
      RunFaceDetection,
      VerboseModeChanged
    };

    CommandMessage(Type iType, unsigned iFrameId, long long iTimestamp);

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

  inline std::ostream& operator<< (std::ostream& ioStream, const CommandMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Type: " << static_cast<int>(iMessage.GetType());
    return ioStream;
  }
}
