package com.yalantis.cameramodule.event;

public class ZoomChangedArgs extends EventArgs {

    private int mIndex;
    private float mValue;

    public ZoomChangedArgs(int iIndex, float iValue) {
        mIndex = iIndex;
        mValue = iValue;
    }

    public int getIndex() {
        return mIndex;
    }

    public float getValue() {
        return mValue;
    }
}
