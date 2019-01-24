package com.yalantis.cameramodule.event;

import com.yalantis.cameramodule.common.camera.FlashMode;

public class FlashModeArgs extends EventArgs {

    private FlashMode mFlashMode;

    public FlashModeArgs(FlashMode iFlashMode) {
        mFlashMode = iFlashMode;
    }

    public FlashMode getFlashMode() {
        return mFlashMode;
    }
}
