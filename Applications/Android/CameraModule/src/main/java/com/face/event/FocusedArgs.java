package com.face.event;

import android.hardware.Camera;

public class FocusedArgs extends EventArgs {

    private boolean mSuccess;
    private Camera mCamera;

    public FocusedArgs(boolean iSuccess, Camera iCamera) {
        mSuccess = iSuccess;
        mCamera = iCamera;
    }

    public boolean getSuccess() {
        return mSuccess;
    }

    public Camera getCamera() {
        return mCamera;
    }
}
