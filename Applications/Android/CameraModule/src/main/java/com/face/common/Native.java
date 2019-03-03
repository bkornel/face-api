package com.face.common;

public enum Native {
    i;

    @SuppressWarnings("JniMissingFunction")
    public native int initialize(String iPath);

    @SuppressWarnings("JniMissingFunction")
    public native int process(int iRotation, int iWidth, int iHeight, byte iYUV[], int[] iRGBA);

    @SuppressWarnings("JniMissingFunction")
    public native int reset();
}
