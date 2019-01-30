package com.face.event;

import android.hardware.Camera;

import com.face.common.camera.FlashMode;

public class FlashModeArgs extends EventArgs {

    private FlashMode mFlashMode;

    public FlashModeArgs(FlashMode iFlashMode) {
        mFlashMode = iFlashMode;
    }

    public FlashMode getFlashMode() {
        return mFlashMode;
    }

    public String getCameraFlashMode() {
        switch (mFlashMode) {
            case ON:
                return Camera.Parameters.FLASH_MODE_ON;
            case OFF:
                return Camera.Parameters.FLASH_MODE_OFF;
            case AUTO:
                return Camera.Parameters.FLASH_MODE_AUTO;
        }

        return null;
    }
}
