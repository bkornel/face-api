package com.face.interfaces;

import android.graphics.Bitmap;

import com.face.common.image.ManagedTarget;

public interface StorageCallback {

    void setBitmap(String path, Bitmap bitmap);

    void addTarget(ManagedTarget target);

    void removeTarget(ManagedTarget target);
}
