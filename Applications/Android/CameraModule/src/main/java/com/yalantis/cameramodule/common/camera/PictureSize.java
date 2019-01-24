package com.yalantis.cameramodule.common.camera;

import android.support.annotation.NonNull;

import java.util.ArrayList;

public class PictureSize {

    public static ArrayList<String> toStringArrayList(@NonNull ArrayList<PictureSize> psizes) {
        ArrayList<String> strAL = new ArrayList<>();

        for (PictureSize ps:psizes) {
            strAL.add(ps.toString());
        }

        return strAL;
    }

    public static ArrayList<PictureSize> toPictureSizeArrayList(@NonNull ArrayList<String> sizesStr) {
        ArrayList<PictureSize> sizes = new ArrayList<>();

        int idx = 0;
        for (String s:sizesStr) {
            String[] parts = s.split("x");
            if(parts.length == 2){
                sizes.add(new PictureSize(idx++, Integer.parseInt(parts[0]), Integer.parseInt(parts[1])));
            }
        }

        return sizes;
    }

    private int mId;
    private int mWidth;
    private int mHeight;
    private Ratio mRatio;
    private String mName;

    public PictureSize(int id, int width, int height) {
        mId = id;

        mWidth = width;
        mHeight = height;

        mName = Integer.toString(width) + "x" + Integer.toString(height);
        mRatio = Ratio.pickRatio(width, height);
    }

    public int getId() { return mId; }

    public int getWidth() { return mWidth; }

    public int getHeight() { return mHeight; }

    public Ratio getRatio() { return mRatio; }

    public int getArea() { return mWidth * mHeight; }

    @Override
    public String toString() {
        return mName;
    }
}
