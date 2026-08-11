package com.face.common;

public enum Native {
    i;

    @SuppressWarnings("JniMissingFunction")
    public native int initialize(String iPath);

    /**
     * Processes one camera frame.
     *
     * @param iRGBA optional. When null the rendered frame is not produced at all, which is
     *              what the overlay path wants: nothing but the geometry crosses JNI.
     */
    @SuppressWarnings("JniMissingFunction")
    public native int process(int iRotation, int iWidth, int iHeight, byte iYUV[], int[] iRGBA);

    /**
     * Copies the overlay data of the last processed frame into iBuffer.
     * See {@link com.face.view.FaceOverlayData} for the layout.
     *
     * @return the number of faces written, 0 when there is nothing to draw, negative on error
     */
    @SuppressWarnings("JniMissingFunction")
    public native int getResults(float[] iBuffer);

    @SuppressWarnings("JniMissingFunction")
    public native int reset();
}
