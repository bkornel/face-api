#include "Modules/General/LastModule.h"

namespace face
{
  const cv::Mat LastModule::sEmptyMat;

  bool LastModule::Main(ImageMessage::Shared iImage)
  {
    mLastImage = iImage;

    return (mLastImage != nullptr && !mLastImage->IsEmpty());
  }

  bool LastModule::Get() const
  {
    return mOutputPort->Get();
  }

  void LastModule::Wait() const
  {
    mOutputPort->Wait();
  }
}
