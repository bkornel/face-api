package com.face.common.camera;

import org.jetbrains.annotations.NotNull;

public enum FlashMode {
    ON(0, "On"),
    AUTO(1, "Auto"),
    OFF(2, "Off");

    private int mId;
    private String mName;

    FlashMode(int iId, String iName) {
        mId = iId;
        mName = iName;
    }

    public static FlashMode getFlashModeById(int iId) {
        for (FlashMode mode : values()) {
            if (mode.mId == iId) {
                return mode;
            }
        }
        return null;
    }

    public int getId() {
        return mId;
    }

    @NotNull
    @Override
    public String toString() {
        return mName;
    }
}
