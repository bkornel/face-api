package com.yalantis.cameramodule.event;

import com.yalantis.cameramodule.common.camera.PictureSize;

public class PictureSizeArgs extends EventArgs {

    private PictureSize mPictureSize;

    public PictureSizeArgs(PictureSize iPictureSize) {
        mPictureSize = iPictureSize;
    }

    public PictureSize getPictureSize() {
        return mPictureSize;
    }
}
