package com.yalantis.cameramodule.event;

import com.yalantis.cameramodule.common.camera.FocusMode;

public class FocusModeArgs extends EventArgs {

    private FocusMode mFocusMode;

    public FocusModeArgs(FocusMode iFocusMode) {
        mFocusMode = iFocusMode;
    }

    public FocusMode getFocusMode() {
        return mFocusMode;
    }
}
