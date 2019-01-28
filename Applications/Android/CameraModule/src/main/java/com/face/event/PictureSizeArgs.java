package com.face.event;

import com.face.common.camera.PictureSize;

public class PictureSizeArgs extends EventArgs {

    private PictureSize mPictureSize;

    public PictureSizeArgs(PictureSize iPictureSize) {
        mPictureSize = iPictureSize;
    }

    public PictureSize getPictureSize() {
        return mPictureSize;
    }
}
