package com.face.common.camera;

import android.support.annotation.NonNull;

import java.util.ArrayList;

public class PictureSize {

    private int mId;
    private int mWidth;
    private int mHeight;
    private Ratio mRatio;
    private String mName;

    public PictureSize(int iId, int iWidth, int iHeight) {
        mId = iId;

        mWidth = iWidth;
        mHeight = iHeight;

        mName = Integer.toString(iWidth) + "x" + Integer.toString(iHeight);
        mRatio = Ratio.pickRatio(iWidth, iHeight);
    }

    public static ArrayList<String> toStringArrayList(@NonNull ArrayList<PictureSize> iSizes) {
        ArrayList<String> list = new ArrayList<>();

        for (PictureSize ps : iSizes) {
            list.add(ps.toString());
        }

        return list;
    }

    public static ArrayList<PictureSize> toPictureSizeArrayList(@NonNull ArrayList<String> iSizes) {
        ArrayList<PictureSize> list = new ArrayList<>();

        int idx = 0;
        for (String s : iSizes) {
            String[] parts = s.split("x");
            if (parts.length == 2) {
                list.add(new PictureSize(idx++, Integer.parseInt(parts[0]), Integer.parseInt(parts[1])));
            }
        }

        return list;
    }

    public int getId() {
        return mId;
    }

    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    public Ratio getRatio() {
        return mRatio;
    }

    public int getArea() {
        return mWidth * mHeight;
    }

    @Override
    public String toString() {
        return mName;
    }
}
