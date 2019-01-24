package com.yalantis.cameramodule.control;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;

public class PreviewImageView extends ImageView {

    public PreviewImageView(Context context) {
        super(context);
    }

    public PreviewImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(getMeasuredWidth(), getMeasuredHeight());
    }

}
