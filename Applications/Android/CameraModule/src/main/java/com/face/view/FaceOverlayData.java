package com.face.view;

import com.face.common.Native;

/**
 * Reads back the flat float buffer that {@link Native#getResults(float[])} fills.
 *
 * The layout has to stay in step with face::result_buffer in FaceApi/FaceResultBuffer.h, which
 * is what writes the buffer. It is a single reused array rather than a list of objects on
 * purpose: this is read on every processed frame, and the point of the whole data path is not
 * to allocate per frame.
 *
 * Coordinates are in the pixel coordinate system of the frame that was pushed in, so the
 * caller has to scale them to its view. See {@link FaceOverlayView#setFrameSize(int, int)}.
 */
public final class FaceOverlayData {

    private static final int HEADER_FLOATS = 1;
    private static final int MAX_LANDMARKS = 68;
    private static final int MAX_BOX_POINTS = 8;
    private static final int FACE_STRIDE = 8 + (MAX_LANDMARKS * 2) + (MAX_BOX_POINTS * 2) + 3 + 3;

    private static final int OFFSET_USER_ID = 0;
    private static final int OFFSET_FRAME_ID = 1;
    private static final int OFFSET_RECT = 2;
    private static final int OFFSET_LANDMARK_COUNT = 6;
    private static final int OFFSET_BOX_COUNT = 7;
    private static final int OFFSET_LANDMARKS = 8;
    private static final int OFFSET_BOX = OFFSET_LANDMARKS + (MAX_LANDMARKS * 2);
    private static final int OFFSET_POSE = OFFSET_BOX + (MAX_BOX_POINTS * 2);

    private final float[] mBuffer;
    private int mFaceCount;

    public FaceOverlayData(int iMaxFaces) {
        mBuffer = new float[HEADER_FLOATS + (Math.max(1, iMaxFaces) * FACE_STRIDE)];
        mFaceCount = 0;
    }

    /** Pulls the last results from the native side. @return true when there is something to draw. */
    public boolean update() {
        int count = Native.i.getResults(mBuffer);
        mFaceCount = Math.max(count, 0);
        return mFaceCount > 0;
    }

    public void clear() {
        mFaceCount = 0;
    }

    public int getFaceCount() {
        return mFaceCount;
    }

    private int base(int iFace) {
        return HEADER_FLOATS + (iFace * FACE_STRIDE);
    }

    public int getUserId(int iFace) {
        return (int) mBuffer[base(iFace) + OFFSET_USER_ID];
    }

    public int getFrameId(int iFace) {
        return (int) mBuffer[base(iFace) + OFFSET_FRAME_ID];
    }

    public float getRectLeft(int iFace) {
        return mBuffer[base(iFace) + OFFSET_RECT];
    }

    public float getRectTop(int iFace) {
        return mBuffer[base(iFace) + OFFSET_RECT + 1];
    }

    public float getRectWidth(int iFace) {
        return mBuffer[base(iFace) + OFFSET_RECT + 2];
    }

    public float getRectHeight(int iFace) {
        return mBuffer[base(iFace) + OFFSET_RECT + 3];
    }

    public int getLandmarkCount(int iFace) {
        return (int) mBuffer[base(iFace) + OFFSET_LANDMARK_COUNT];
    }

    public float getLandmarkX(int iFace, int iPoint) {
        return mBuffer[base(iFace) + OFFSET_LANDMARKS + (iPoint * 2)];
    }

    public float getLandmarkY(int iFace, int iPoint) {
        return mBuffer[base(iFace) + OFFSET_LANDMARKS + (iPoint * 2) + 1];
    }

    public int getBoxPointCount(int iFace) {
        return (int) mBuffer[base(iFace) + OFFSET_BOX_COUNT];
    }

    public float getBoxX(int iFace, int iPoint) {
        return mBuffer[base(iFace) + OFFSET_BOX + (iPoint * 2)];
    }

    public float getBoxY(int iFace, int iPoint) {
        return mBuffer[base(iFace) + OFFSET_BOX + (iPoint * 2) + 1];
    }

    /** Roll, pitch and yaw in radians. */
    public float getRoll(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE];
    }

    public float getPitch(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE + 1];
    }

    public float getYaw(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE + 2];
    }

    /** Head position in the 3-D camera coordinate system, millimetres. */
    public float getPositionX(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE + 3];
    }

    public float getPositionY(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE + 4];
    }

    public float getPositionZ(int iFace) {
        return mBuffer[base(iFace) + OFFSET_POSE + 5];
    }
}
