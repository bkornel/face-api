#include "Modules/LastModule/LastModule.h"

namespace face
{
  const cv::Mat LastModule::sEmptyMat;

  bool LastModule::Main(ImageMessage::Shared iImage)
  {
    mLastImage = iImage;

    return (mLastImage != nullptr && !mLastImage->IsEmpty());
  }
}
