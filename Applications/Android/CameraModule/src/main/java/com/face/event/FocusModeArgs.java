package com.face.event;

import com.face.common.camera.FocusMode;

public class FocusModeArgs extends EventArgs {

    private FocusMode mFocusMode;

    public FocusModeArgs(FocusMode iFocusMode) {
        mFocusMode = iFocusMode;
    }

    public FocusMode getFocusMode() {
        return mFocusMode;
    }
}
