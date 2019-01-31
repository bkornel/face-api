package com.face.common.camera;

public enum Ratio {
    R_4x3(0, 4, 3),
    R_16x9(1, 16, 9);

    public int mWidth;
    public int mHeight;
    private int mId;

    Ratio(int iId, int iWidth, int iHeight) {
        mId = iId;
        mWidth = iWidth;
        mHeight = iHeight;
    }

    public static Ratio getRatioById(int iId) {
        for (Ratio ratio : values()) {
            if (ratio.mId == iId) {
                return ratio;
            }
        }
        return null;
    }

    public static Ratio pickRatio(int iWidth, int iHeight) {
        for (Ratio ratio : values()) {
            if (iWidth / ratio.mWidth == iHeight / ratio.mHeight) {
                return ratio;
            }
        }
        return null;
    }

    public int getId() {
        return mId;
    }

    @Override
    public String toString() {
        return mWidth + ":" + mHeight;
    }

}
