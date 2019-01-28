package com.face.event;

import android.hardware.Camera;

public class FocusedArgs extends EventArgs {

    public enum Initiator {
        INVALID,
        TOUCH,
        TAKE_PICTURE
    }

    private boolean mSuccess;
    private Camera mCamera;
    private Initiator mInitiator;

    public FocusedArgs(boolean iSuccess, Camera iCamera, Initiator iInitiator) {
        mSuccess = iSuccess;
        mCamera = iCamera;
        mInitiator = iInitiator;
    }

    public boolean getSuccess() {
        return mSuccess;
    }

    public Camera getCamera() {
        return mCamera;
    }

    public Initiator getInitiator() {
        return mInitiator;
    }
}
