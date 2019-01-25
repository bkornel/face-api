package com.yalantis.cameramodule.control;

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

import com.yalantis.cameramodule.common.Configuration;
import com.yalantis.cameramodule.event.Event;
import com.yalantis.cameramodule.event.FocusedArgs;
import com.yalantis.cameramodule.event.IEvent;
import com.yalantis.cameramodule.event.ZoomChangedArgs;
import com.yalantis.cameramodule.common.camera.FocusMode;

import java.math.BigDecimal;
import java.util.Collections;
import java.util.List;

import timber.log.Timber;

@SuppressWarnings("deprecation")
public class FocusView extends SurfaceView implements SurfaceHolder.Callback, Camera.AutoFocusCallback {

    public final IEvent<ZoomChangedArgs> ZoomChanged = new Event<>();
    public final IEvent<FocusedArgs> Focused = new Event<>();

    private static final float FOCUS_AREA_SIZE = 75.0f;
    private static final float STROKE_WIDTH = 5.0f;
    private static final float FOCUS_AREA_FULL_SIZE = 2000.0f;
    private static final int ACCURACY = 3;

    private Activity mActivity;
    private Camera mCamera;

    private ImageView mFocusImageView;
    private Canvas mFocusCanvas;
    private Paint mFocusPaint;
    private FocusedArgs.Initiator mFocusInitiator;

    private boolean mFocusing;
    private float mFocusCoeffW;
    private float mFocusCoeffH;

    private Rect mTapArea;

    private List<Integer> mZoomRatios;
    private int mZoomIndex;
    private int mMinZoomIndex;
    private int mMaxZoomIndex;
    private float mPrevScaleFactor;

    public FocusView(Context context) {
        super(context);
    }

    public FocusView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public FocusView(Activity activity, Camera camera, ImageView focusImageView) {
        super(activity);

        mActivity = activity;
        mCamera = camera;
        mFocusImageView = focusImageView;
        mFocusInitiator = FocusedArgs.Initiator.TOUCH;

        // Install a SurfaceHolder.Callback so we get notified when the underlying surface is created and destroyed.
        SurfaceHolder holder = getHolder();
        if (holder != null) {
            holder.addCallback(this);
            holder.setKeepScreenOn(true);
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // preview surface does not exist
        if (holder.getSurface() == null) {
            return;
        }

        // Init focus drawing tools
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_4444);
        mFocusCanvas = new Canvas(bitmap);
        mFocusPaint = new Paint();
        mFocusPaint.setColor(Color.GREEN);
        mFocusPaint.setStrokeWidth(STROKE_WIDTH);
        mFocusImageView.setImageBitmap(bitmap);

        // Init focus coeffs
        mFocusCoeffW = width / FOCUS_AREA_FULL_SIZE;
        mFocusCoeffH = height / FOCUS_AREA_FULL_SIZE;

        // Init zoom
        Camera.Parameters parameters = mCamera.getParameters();
        mZoomRatios = parameters.getZoomRatios();
        mZoomIndex = mMinZoomIndex = 0;
        mMaxZoomIndex = parameters.getMaxZoom();

        setOnTouchListener(new CameraTouchListener());
    }

    private boolean hasAutoFocus()
    {
        if (mCamera == null) return false;
        return mCamera.getParameters().getSupportedFocusModes().contains(Camera.Parameters.FOCUS_MODE_AUTO);
    }

    private void startFocusing() {
        mFocusing = true;
        mCamera.autoFocus(this);
    }

    public void takePicture() {
        mFocusInitiator = FocusedArgs.Initiator.TAKE_PICTURE;

        if (hasAutoFocus()) {
            startFocusing();
        } else {
            mFocusing = false;
            Focused.raise(this, new FocusedArgs(false, mCamera, mFocusInitiator));
        }
    }

    public void resetCameraFocus() {
        mFocusing = false;

        if (hasAutoFocus()) {
            mCamera.cancelAutoFocus();

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
    }

    @Override
    public void onAutoFocus(boolean iSuccess, Camera iCamera) {
        mFocusing = false;
        Focused.raise(this, new FocusedArgs(iSuccess, iCamera, mFocusInitiator));
    }

    protected void focusOnTouch(MotionEvent event) {
        mTapArea = calculateTapArea(event.getX(), event.getY(), 1.0f);

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
            Rect rectMetering = calculateTapArea(event.getX(), event.getY(), 1.5f);
            Camera.Area area = new Camera.Area(convert(rectMetering), 100);
            parameters.setMeteringAreas(Collections.singletonList(area));
        }

        mCamera.setParameters(parameters);
        mFocusInitiator = FocusedArgs.Initiator.TAKE_PICTURE;

        startFocusing();
    }

    private Rect calculateTapArea(float x, float y, float coefficient) {
        int areaSize = Float.valueOf(FOCUS_AREA_SIZE * coefficient).intValue();

        int left = Math.min(Math.max((int) x - areaSize / 2, 0), getWidth() - areaSize);
        int top = Math.min(Math.max((int) y - areaSize / 2, 0), getHeight() - areaSize);

        return round(new RectF(left, top, left + areaSize, top + areaSize));
    }

    private Rect round(RectF rect) {
        return new Rect(Math.round(rect.left), Math.round(rect.top), Math.round(rect.right), Math.round(rect.bottom));
    }

    private Rect convert(Rect rect) {
        return new Rect(
                normalize(rect.left / mFocusCoeffW - 1000),
                normalize(rect.top / mFocusCoeffH - 1000),
                normalize(rect.right / mFocusCoeffW - 1000),
                normalize(rect.bottom / mFocusCoeffH - 1000)
        );
    }

    private int normalize(float value) {
        return Math.round(Math.min(Math.max(value, -1000), 1000));
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

    private void zoomByScale(float scaleFactor) {
        scaleFactor = BigDecimal.valueOf(scaleFactor).setScale(ACCURACY, BigDecimal.ROUND_HALF_UP).floatValue();
        if (Float.compare(scaleFactor, 1.0f) == 0 || Float.compare(scaleFactor, mPrevScaleFactor) == 0) {
            return;
        }
        if (scaleFactor > 1f) {
            zoomIn();
        }
        if (scaleFactor < 1f) {
            zoomOut();
        }
        mPrevScaleFactor = scaleFactor;
    }

    private class CameraTouchListener implements OnTouchListener {

        private ScaleGestureDetector mScaleDetector = new ScaleGestureDetector(mActivity, new ScaleListener());
        private GestureDetector mTapDetector = new GestureDetector(mActivity, new TapListener());

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            resetCameraFocus();

            if (event.getPointerCount() > 1) {
                mScaleDetector.onTouchEvent(event);
                return true;
            }

            if (hasAutoFocus() && Configuration.i.getFocusMode() == FocusMode.TOUCH) {
                mTapDetector.onTouchEvent(event);
                return true;
            }

            if (event.getAction() == MotionEvent.ACTION_UP) {
                v.performClick();
            }

            return true;
        }

        private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
            @Override
            public boolean onScale(ScaleGestureDetector detector) {
                zoomByScale(detector.getScaleFactor());
                return true;
            }
        }

        private class TapListener extends GestureDetector.SimpleOnGestureListener {
            @Override
            public boolean onSingleTapConfirmed(MotionEvent event) {
                // mCamera.autoFocus is called only if it has auto focus
                if (!mFocusing && hasAutoFocus()) {
                    focusOnTouch(event);
                }
                return true;
            }
        }
    }
}
