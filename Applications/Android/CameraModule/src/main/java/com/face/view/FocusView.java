package com.face.view;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageView;

import com.face.common.Configuration;
import com.face.common.camera.FocusMode;
import com.face.event.Event;
import com.face.event.FocusedArgs;
import com.face.event.IEvent;
import com.face.event.ZoomChangedArgs;

import java.math.BigDecimal;
import java.util.Collections;
import java.util.List;

import timber.log.Timber;

@SuppressWarnings("deprecation")
public class FocusView extends SurfaceView implements SurfaceHolder.Callback, Camera.AutoFocusCallback {

    private static final float FOCUS_AREA_SIZE = 75.0f;
    private static final float STROKE_WIDTH = 5.0f;
    private static final float FOCUS_AREA_FULL_SIZE = 2000.0f;
    private static final int ACCURACY = 3;
    public final IEvent<ZoomChangedArgs> ZoomChanged = new Event<>();
    public final IEvent<FocusedArgs> Focused = new Event<>();
    private Activity mActivity;
    private Camera mCamera;

    private ImageView mFocusImageView;
    private Canvas mFocusCanvas;
    private Paint mFocusPaint;

    private boolean mFocusing;
    private float mFocusCoeffW;
    private float mFocusCoeffH;

    private Rect mTapArea;

    private List<Integer> mZoomRatios;
    private int mZoomIndex;
    private int mMinZoomIndex;
    private int mMaxZoomIndex;
    private float mPrevScaleFactor;

    public FocusView(Context iContext) {
        super(iContext);
    }

    public FocusView(Context iContext, AttributeSet iAttributeSet) {
        super(iContext, iAttributeSet);
    }

    public FocusView(Activity iActivity, Camera iCamera) {
        super(iActivity);

        mActivity = iActivity;
        mCamera = iCamera;
        mFocusImageView = new ImageView(mActivity);

        // Install a SurfaceHolder.Callback so we get notified when the underlying surface is created and destroyed.
        SurfaceHolder holder = getHolder();
        if (holder != null) {
            holder.addCallback(this);
            holder.setKeepScreenOn(true);
        }
    }

    public void surfaceCreated(SurfaceHolder iHolder) {
    }

    public void surfaceDestroyed(SurfaceHolder iHolder) {
    }

    public void surfaceChanged(SurfaceHolder iHolder, int iFormat, int iWidth, int iHeight) {
        // preview surface does not exist
        if (iHolder.getSurface() == null) {
            return;
        }

        // Init focus drawing tools
        Bitmap bitmap = Bitmap.createBitmap(iWidth, iHeight, Bitmap.Config.ARGB_8888);
        mFocusCanvas = new Canvas(bitmap);
        mFocusPaint = new Paint();
        mFocusPaint.setColor(Color.GREEN);
        mFocusPaint.setStrokeWidth(STROKE_WIDTH);
        mFocusPaint.setStyle(Paint.Style.STROKE);
        mFocusImageView.setImageBitmap(bitmap);

        // Init focus coeffs
        mFocusCoeffW = iWidth / FOCUS_AREA_FULL_SIZE;
        mFocusCoeffH = iHeight / FOCUS_AREA_FULL_SIZE;

        // Init zoom
        Camera.Parameters parameters = mCamera.getParameters();
        mZoomRatios = parameters.getZoomRatios();
        mZoomIndex = mMinZoomIndex = 0;
        mMaxZoomIndex = parameters.getMaxZoom();

        setOnTouchListener(new CameraTouchListener());
    }

    public ImageView getFocusImageView() { return mFocusImageView; }

    private boolean hasAutoFocus() {
        if (mCamera == null) return false;
        return mCamera.getParameters().getSupportedFocusModes().contains(Camera.Parameters.FOCUS_MODE_AUTO);
    }

    public void startFocusing() {
        mFocusing = true;

        if (hasAutoFocus()) {
            Camera.Parameters parameters = mCamera.getParameters();
            if (parameters.getFocusMode() != Camera.Parameters.FOCUS_MODE_AUTO) {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                mCamera.setParameters(parameters);
            }

            try {
                mCamera.autoFocus(this);
            } catch (RuntimeException e) {
                Timber.e(e, "startFocusing");
            }

            return;
        }

        onAutoFocus(false, mCamera);
    }

    public void resetCameraFocus() {
        mFocusing = false;

        if (hasAutoFocus()) {
            mCamera.cancelAutoFocus();
        }

        if (mFocusCanvas != null) {
            mTapArea = null;
            mFocusCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
            mFocusImageView.draw(mFocusCanvas);
            mFocusImageView.invalidate();

            try {
                Camera.Parameters parameters = mCamera.getParameters();
                parameters.setFocusAreas(null);
                parameters.setMeteringAreas(null);
                mCamera.setParameters(parameters);
            } catch (Exception e) {
                Timber.e(e, "resetCameraFocus");
            }
        }
    }

    @Override
    public void onAutoFocus(boolean iSuccess, Camera iCamera) {
        mFocusing = false;
        Focused.raise(this, new FocusedArgs(iSuccess, iCamera));
        resetCameraFocus();
    }

    protected void focusOnTouch(MotionEvent iEvent) {
        mTapArea = calculateTapArea(iEvent.getX(), iEvent.getY(), 3.0f);

        mFocusCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        mFocusCanvas.drawRect(mTapArea, mFocusPaint);
        mFocusImageView.draw(mFocusCanvas);
        mFocusImageView.invalidate();

        Camera.Parameters parameters = mCamera.getParameters();
        if (parameters.getMaxNumFocusAreas() > 0) {
            Camera.Area area = new Camera.Area(convert(mTapArea), 100);
            parameters.setFocusAreas(Collections.singletonList(area));
        }

        if (parameters.getMaxNumMeteringAreas() > 0) {
            Rect rectMetering = calculateTapArea(iEvent.getX(), iEvent.getY(), 1.5f);
            Camera.Area area = new Camera.Area(convert(rectMetering), 100);
            parameters.setMeteringAreas(Collections.singletonList(area));
        }

        mCamera.setParameters(parameters);

        startFocusing();
    }

    private Rect calculateTapArea(float iX, float iY, float iCoefficient) {
        int areaSize = Float.valueOf(FOCUS_AREA_SIZE * iCoefficient).intValue();

        int left = Math.min(Math.max((int) iX - areaSize / 2, 0), getWidth() - areaSize);
        int top = Math.min(Math.max((int) iY - areaSize / 2, 0), getHeight() - areaSize);

        return round(new RectF(left, top, left + areaSize, top + areaSize));
    }

    private Rect round(RectF iRect) {
        return new Rect(Math.round(iRect.left), Math.round(iRect.top), Math.round(iRect.right), Math.round(iRect.bottom));
    }

    private Rect convert(Rect iRect) {
        return new Rect(
                normalize(iRect.left / mFocusCoeffW - 1000),
                normalize(iRect.top / mFocusCoeffH - 1000),
                normalize(iRect.right / mFocusCoeffW - 1000),
                normalize(iRect.bottom / mFocusCoeffH - 1000)
        );
    }

    private int normalize(float iValue) {
        return Math.round(Math.min(Math.max(iValue, -1000), 1000));
    }

    public void zoomIn() {
        if (++mZoomIndex > mMaxZoomIndex) {
            mZoomIndex = mMaxZoomIndex;
        }
        ZoomChanged.raise(this, new ZoomChangedArgs(mZoomIndex, mZoomRatios.get(mZoomIndex)));
    }

    public void zoomOut() {
        if (--mZoomIndex < mMinZoomIndex) {
            mZoomIndex = mMinZoomIndex;
        }
        ZoomChanged.raise(this, new ZoomChangedArgs(mZoomIndex, mZoomRatios.get(mZoomIndex)));
    }

    private void zoomByScale(float iScaleFactor) {
        iScaleFactor = BigDecimal.valueOf(iScaleFactor).setScale(ACCURACY, BigDecimal.ROUND_HALF_UP).floatValue();
        if (Float.compare(iScaleFactor, 1.0f) == 0 || Float.compare(iScaleFactor, mPrevScaleFactor) == 0) {
            return;
        }
        if (iScaleFactor > 1f) {
            zoomIn();
        }
        if (iScaleFactor < 1f) {
            zoomOut();
        }
        mPrevScaleFactor = iScaleFactor;
    }

    private class CameraTouchListener implements OnTouchListener {

        private ScaleGestureDetector mScaleDetector = new ScaleGestureDetector(mActivity, new ScaleListener());
        private GestureDetector mTapDetector = new GestureDetector(mActivity, new TapListener());

        @Override
        public boolean onTouch(View iView, MotionEvent iEvent) {
            resetCameraFocus();

            if (iEvent.getPointerCount() > 1) {
                mScaleDetector.onTouchEvent(iEvent);
                return true;
            }

            if (hasAutoFocus() && Configuration.i.getFocusMode() == FocusMode.TOUCH) {
                mTapDetector.onTouchEvent(iEvent);
                return true;
            }

            if (iEvent.getAction() == MotionEvent.ACTION_UP) {
                iView.performClick();
            }

            return true;
        }

        private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
            @Override
            public boolean onScale(ScaleGestureDetector iDetector) {
                zoomByScale(iDetector.getScaleFactor());
                return true;
            }
        }

        private class TapListener extends GestureDetector.SimpleOnGestureListener {
            @Override
            public boolean onSingleTapConfirmed(MotionEvent iEvent) {
                // mCamera.autoFocus is called only if it has auto focus
                if (!mFocusing && hasAutoFocus()) {
                    focusOnTouch(iEvent);
                }
                return true;
            }
        }
    }
}
