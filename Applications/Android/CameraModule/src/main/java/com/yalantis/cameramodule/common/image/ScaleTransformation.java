package com.yalantis.cameramodule.common.image;

import android.graphics.Bitmap;
import android.graphics.Matrix;

import com.squareup.picasso.Transformation;

public class ScaleTransformation implements Transformation {

    private float width;
    private float height;

    public ScaleTransformation(float width, float height) {
        this.width = width;
        this.height = height;
    }

    @Override
    public Bitmap transform(Bitmap source) {
        float sWidth = source.getWidth();
        float sHeight = source.getHeight();

        float xScale;
        float yScale;
        if (sWidth < sHeight) {
            yScale = height / sHeight;
            xScale = yScale;
        } else {
            xScale = width / sWidth;
            yScale = xScale;
        }

        Matrix matrix = new Matrix();
        matrix.postScale(xScale, yScale);
        Bitmap scaledBitmap = Bitmap.createBitmap(source, 0, 0, (int) sWidth, (int) sHeight, matrix, true);

        scaledBitmap.getWidth();
        scaledBitmap.getHeight();
        if (scaledBitmap != source) {
            source.recycle();
        }

        return scaledBitmap;
    }

    @Override
    public String key() {
        return "scaleTo" + width + "x" + height;
    }

}
