package com.face.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.widget.ImageView;

public class PhotoView extends ImageView {
    private static final int CLICK = 3;

    // We can be in one of these 3 states
    private static final int STATE_NONE = 0;
    private static final int STATE_DRAG = 1;
    private static final int STATE_ZOOM = 2;
    private final float mMinScale = 1.0f;
    private final float mMaxScale = 3.0f;
    private int mState = STATE_NONE;
    // Remember some things for zooming
    private PointF mLastPt = new PointF();
    private PointF mStartPt = new PointF();
    private float mSaveScale = 1.0f;

    private Matrix mMatrix = new Matrix();
    private float[] mMatrixValues = new float[9];

    private float mRedundantXSpace, mRedundantYSpace;

    private float mWidth, mHeight;
    private float mRight, mBottom, mOrigWidth, mOrigHeight, mBitmapWidth, mBitmapHeight;

    private ScaleGestureDetector mScaleDetector;

    public PhotoView(Context iContext) {
        super(iContext);
        initialize(iContext);
    }

    public PhotoView(Context iContext, AttributeSet iAttributeSet) {
        super(iContext, iAttributeSet);
        initialize(iContext);
    }

    private void initialize(Context iContext) {
        super.setClickable(true);

        mScaleDetector = new ScaleGestureDetector(iContext, new ScaleListener());
        mMatrix.setTranslate(mMinScale, mMinScale);

        setImageMatrix(mMatrix);
        setScaleType(ImageView.ScaleType.MATRIX);

        setOnTouchListener((v, event) -> {
            mScaleDetector.onTouchEvent(event);

            mMatrix.getValues(mMatrixValues);
            float x = mMatrixValues[Matrix.MTRANS_X];
            float y = mMatrixValues[Matrix.MTRANS_Y];
            PointF curr = new PointF(event.getX(), event.getY());

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    mLastPt.set(event.getX(), event.getY());
                    mStartPt.set(mLastPt);
                    mState = STATE_DRAG;
                    break;

                case MotionEvent.ACTION_MOVE:
                    if (mState == STATE_DRAG) {
                        float deltaX = curr.x - mLastPt.x;
                        float deltaY = curr.y - mLastPt.y;
                        float scaleWidth = Math.round(mOrigWidth * mSaveScale);
                        float scaleHeight = Math.round(mOrigHeight * mSaveScale);

                        if (scaleWidth < mWidth) {
                            deltaX = 0.0f;
                            if (y + deltaY > 0.0f)
                                deltaY = -y;
                            else if (y + deltaY < -mBottom)
                                deltaY = -(y + mBottom);

                        } else if (scaleHeight < mHeight) {
                            deltaY = 0.0f;
                            if (x + deltaX > 0.0f)
                                deltaX = -x;
                            else if (x + deltaX < -mRight)
                                deltaX = -(x + mRight);

                        } else {
                            if (x + deltaX > 0.0f)
                                deltaX = -x;
                            else if (x + deltaX < -mRight)
                                deltaX = -(x + mRight);

                            if (y + deltaY > 0.0f)
                                deltaY = -y;
                            else if (y + deltaY < -mBottom)
                                deltaY = -(y + mBottom);

                        }

                        mMatrix.postTranslate(deltaX, deltaY);
                        mLastPt.set(curr.x, curr.y);
                    }
                    break;

                case MotionEvent.ACTION_UP:
                    mState = STATE_NONE;
                    int xDiff = (int) Math.abs(curr.x - mStartPt.x);
                    int yDiff = (int) Math.abs(curr.y - mStartPt.y);

                    if (xDiff < CLICK && yDiff < CLICK) {
                        performClick();
                    }
                    break;

                case MotionEvent.ACTION_POINTER_UP:
                    mState = STATE_NONE;
                    break;
            }

            setImageMatrix(mMatrix);
            invalidate();

            // indicate event was handled
            return true;
        });
    }

    @Override
    public void setImageBitmap(Bitmap iBitmap) {
        super.setImageBitmap(iBitmap);
        if (iBitmap != null) {
            mBitmapWidth = iBitmap.getWidth();
            mBitmapHeight = iBitmap.getHeight();
        }
    }

    @Override
    public void setImageDrawable(Drawable iDrawable) {
        super.setImageDrawable(iDrawable);
        if (iDrawable == null) {
            return;
        }

        if (iDrawable instanceof BitmapDrawable) {
            BitmapDrawable bitmapDrawable = (BitmapDrawable) iDrawable;
            if (bitmapDrawable.getBitmap() != null) {
                mBitmapWidth = bitmapDrawable.getBitmap().getWidth();
                mBitmapHeight = bitmapDrawable.getBitmap().getHeight();
            }
        } else {
            int intrinsicHeight = iDrawable.getIntrinsicHeight();
            int intrinsicWidth = iDrawable.getIntrinsicWidth();
            mBitmapHeight = intrinsicHeight > 0.0f ? intrinsicHeight : 0.0f;
            mBitmapWidth = intrinsicWidth > 0.0f ? intrinsicWidth : 0.0f;
        }
    }

    private void scale(float iScaleFactor, float iFocusX, float iFocusY) {
        float origScale = mSaveScale;
        mSaveScale *= iScaleFactor;

        if (mSaveScale > mMaxScale) {
            mSaveScale = mMaxScale;
            iScaleFactor = mMaxScale / origScale;
        } else if (mSaveScale < mMinScale) {
            mSaveScale = mMinScale;
            iScaleFactor = mMinScale / origScale;
        }

        mRight = mWidth * mSaveScale - mWidth - (2.0f * mRedundantXSpace * mSaveScale);
        mBottom = mHeight * mSaveScale - mHeight - (2.0f * mRedundantYSpace * mSaveScale);

        if (mOrigWidth * mSaveScale <= mWidth || mOrigHeight * mSaveScale <= mHeight) {
            mMatrix.postScale(iScaleFactor, iScaleFactor, mWidth / 2.0f, mHeight / 2.0f);

            if (iScaleFactor < 1.0f) {
                mMatrix.getValues(mMatrixValues);
                float x = mMatrixValues[Matrix.MTRANS_X];
                float y = mMatrixValues[Matrix.MTRANS_Y];

                if (iScaleFactor < 1.0f) {
                    if (Math.round(mOrigWidth * mSaveScale) < mWidth) {
                        if (y < -mBottom)
                            mMatrix.postTranslate(0.0f, -(y + mBottom));
                        else if (y > 0.0f)
                            mMatrix.postTranslate(0.0f, -y);
                    } else {
                        if (x < -mRight)
                            mMatrix.postTranslate(-(x + mRight), 0.0f);
                        else if (x > 0.0f)
                            mMatrix.postTranslate(-x, 0.0f);
                    }
                }
            }
        } else {
            mMatrix.postScale(iScaleFactor, iScaleFactor, iFocusX, iFocusY);
            mMatrix.getValues(mMatrixValues);
            float x = mMatrixValues[Matrix.MTRANS_X];
            float y = mMatrixValues[Matrix.MTRANS_Y];

            if (iScaleFactor < 1.0f) {
                if (x < -mRight)
                    mMatrix.postTranslate(-(x + mRight), 0.0f);
                else if (x > 0.0f)
                    mMatrix.postTranslate(-x, 0.0f);
                if (y < -mBottom)
                    mMatrix.postTranslate(0.0f, -(y + mBottom));
                else if (y > 0.0f)
                    mMatrix.postTranslate(0.0f, -y);
            }
        }
    }

    public boolean canScrollHorizontally(int iDirection) {
        mMatrix.getValues(mMatrixValues);

        float x = Math.abs(mMatrixValues[Matrix.MTRANS_X]);
        float scaleWidth = Math.round(mOrigWidth * mSaveScale);

        boolean isReachLeft = (x - iDirection <= 0);
        boolean isReachRight = (x + mWidth - iDirection >= scaleWidth);

        return !(scaleWidth < mWidth || isReachLeft || isReachRight);
    }

    @Override
    protected void onMeasure(int iWidthMeasureSpec, int iHeightMeasureSpec) {
        super.onMeasure(iWidthMeasureSpec, iHeightMeasureSpec);
        setMeasuredDimension(getMeasuredWidth(), getMeasuredHeight());

        mWidth = View.MeasureSpec.getSize(iWidthMeasureSpec);
        mHeight = View.MeasureSpec.getSize(iHeightMeasureSpec);

        locateImage();
    }

    protected void locateImage() {
        if ((Float.compare(mBitmapWidth, 0.0f) * Float.compare(mBitmapHeight, 0.0f)) == 0) {
            return;
        }

        // Fit to screen.
        float scaleX = mWidth / mBitmapWidth;
        float scaleY = mHeight / mBitmapHeight;
        float scale = Math.min(scaleX, scaleY);

        mMatrix.setScale(scale, scale);
        setImageMatrix(mMatrix);
        mSaveScale = 1.0f;

        // Center the image
        mRedundantYSpace = mHeight - (scale * mBitmapHeight);
        mRedundantXSpace = mWidth - (scale * mBitmapWidth);
        mRedundantYSpace /= 2.0f;
        mRedundantXSpace /= 2.0f;

        mMatrix.postTranslate(mRedundantXSpace, mRedundantYSpace);

        mOrigWidth = mWidth - 2.0f * mRedundantXSpace;
        mOrigHeight = mHeight - 2.0f * mRedundantYSpace;
        mRight = mWidth * mSaveScale - mWidth - (2.0f * mRedundantXSpace * mSaveScale);
        mBottom = mHeight * mSaveScale - mHeight - (2.0f * mRedundantYSpace * mSaveScale);

        setImageMatrix(mMatrix);
    }

    private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {

        @Override
        public boolean onScaleBegin(ScaleGestureDetector iDetector) {
            mState = STATE_ZOOM;
            return true;
        }

        @Override
        public boolean onScale(ScaleGestureDetector iDetector) {
            scale(iDetector.getScaleFactor(), iDetector.getFocusX(), iDetector.getFocusY());
            return true;
        }
    }
}
