#include "Configuration.h"
#include "User/UserData.hpp"

namespace face
{
  UserData::UserData(const UserData& iOther)
  {
    CopyFrom(iOther);
  }

  UserData& UserData::operator=(const UserData& iOther)
  {
    if (this != &iOther)
      CopyFrom(iOther);

    return *this;
  }

  void UserData::CopyFrom(const UserData& iOther)
  {
    // The matrices are cloned, not assigned: a copy is meant to be a snapshot, so it must
    // not end up sharing pixel buffers with the record it was taken from.
    mFaceRect = iOther.mFaceRect;
    mFaceBox = iOther.mFaceBox;

    mRPY = iOther.mRPY;
    mPosition3D = iOther.mPosition3D;
    mCameraMatrix = iOther.mCameraMatrix.clone();
    mExtrinsics = iOther.mExtrinsics.clone();
    mRvec = iOther.mRvec.clone();
    mTvec = iOther.mTvec.clone();

    mShape2D = iOther.mShape2D;
    mNormShape2D = iOther.mNormShape2D;
    mShape3D = iOther.mShape3D;
    mNormShape3D = iOther.mNormShape3D;

    mExpression = iOther.mExpression;
  }
}
