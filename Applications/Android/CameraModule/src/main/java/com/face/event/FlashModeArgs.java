package com.face.event;

import com.face.common.camera.FlashMode;

public class FlashModeArgs extends EventArgs {

    private FlashMode mFlashMode;

    public FlashModeArgs(FlashMode iFlashMode) {
        mFlashMode = iFlashMode;
    }

    public FlashMode getFlashMode() {
        return mFlashMode;
    }
}
