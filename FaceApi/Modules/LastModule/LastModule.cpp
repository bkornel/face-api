#include "Modules/LastModule/LastModule.h"

namespace face
{
  bool LastModule::Main(ImageMessage::Shared iImage)
  {
    mLastImage = iImage;

    return (mLastImage != nullptr && !mLastImage->IsEmpty());
  }
}
