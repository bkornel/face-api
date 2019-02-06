package com.face.common.image;

import android.graphics.Bitmap;
import android.graphics.Matrix;

import com.squareup.picasso.Transformation;

public class ScaleTransformation implements Transformation {

    private float mWidth;
    private float mHeight;

    public ScaleTransformation(float iWidth, float iHeight) {
        mWidth = iWidth;
        mHeight = iHeight;
    }

    @Override
    public Bitmap transform(Bitmap iSource) {
        float srcWidth = iSource.getWidth();
        float srcHeight = iSource.getHeight();

        float xScale, yScale;
        if (srcWidth < srcHeight) {
            yScale = mHeight / srcHeight;
            xScale = yScale;
        } else {
            xScale = mWidth / srcWidth;
            yScale = xScale;
        }

        Matrix matrix = new Matrix();
        matrix.postScale(xScale, yScale);
        Bitmap scaledBitmap = Bitmap.createBitmap(iSource, 0, 0, (int) srcWidth, (int) srcHeight, matrix, true);

        if (scaledBitmap != iSource) {
            iSource.recycle();
        }

        return scaledBitmap;
    }

    @Override
    public String key() {
        return "ScaleTransformation" + mWidth + "x" + mHeight;
    }
}
