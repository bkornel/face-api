package com.yalantis.cameramodule.interfaces;

import android.graphics.Bitmap;

import com.yalantis.cameramodule.common.image.ManagedTarget;

public interface StorageCallback {

    void setBitmap(String path, Bitmap bitmap);

    void addTarget(ManagedTarget target);

    void removeTarget(ManagedTarget target);
}
