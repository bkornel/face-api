package com.face.common.image;

import android.graphics.Bitmap;

public interface ImageTargetCallback {

    void setBitmap(String iPath, Bitmap iBitmap);

    void addTarget(ImageTarget iTarget);

    void removeTarget(ImageTarget iTarget);
}
