package com.face.common.camera;

public enum FocusMode {

    AUTO(0, "Auto"),
    TOUCH(1, "Touch");

    private int mId;
    private String mName;

    FocusMode(int iId, String iName) {
        mId = iId;
        mName = iName;
    }

    public static FocusMode getFocusModeById(int iId) {
        for (FocusMode mode : values()) {
            if (mode.mId == iId) {
                return mode;
            }
        }
        return null;
    }

    public int getId() {
        return mId;
    }

    @Override
    public String toString() {
        return mName;
    }
}
