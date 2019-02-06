package com.face.common.image;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;

import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;

public class ImageTarget implements Target {

    private Target mTarget;
    private String mPath;
    private ImageTargetCallback mCallback;

    ImageTarget(Target iTarget, String iPath, ImageTargetCallback iCallback) {
        mTarget = iTarget;
        mPath = iPath;
        mCallback = iCallback;
        mCallback.addTarget(this);
    }

    @Override
    public void onBitmapLoaded(Bitmap iBitmap, Picasso.LoadedFrom iLoadedFrom) {
        mTarget.onBitmapLoaded(iBitmap, iLoadedFrom);
        mCallback.setBitmap(mPath, iBitmap);
        mCallback.removeTarget(this);
    }

    @Override
    public void onBitmapFailed(Drawable iDrawable) {
        mTarget.onBitmapFailed(iDrawable);
        mCallback.removeTarget(this);
    }

    @Override
    public void onPrepareLoad(Drawable iDrawable) {
        mTarget.onPrepareLoad(iDrawable);
    }

    @Override
    public int hashCode() {
        return mPath.hashCode();
    }

    @Override
    public boolean equals(Object iOther) {
        if (super.equals(iOther)) {
            return true;
        }

        if (iOther == null) {
            return false;
        }

        if (iOther.getClass() != this.getClass()) {
            return false;
        }

        ImageTarget other = (ImageTarget) iOther;
        return other.mPath.equals(mPath);
    }
}
