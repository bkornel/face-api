package com.yalantis.cameramodule.control;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.hardware.Camera;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.ImageView;

import java.lang.ref.WeakReference;

import timber.log.Timber;

@SuppressWarnings("deprecation")
public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    @SuppressWarnings("JniMissingFunction")
    private native int NativeProcess(int rotation, int width, int height, byte yuv[], int[] rgba);

    private static final float STROKE_WIDTH = 5.0f;

    private int mCameraId;
    private Activity mActivity;
    private Camera mCamera;
    private ProcessAsyncTask mNativeTask;

    private ImageView mNativeImageView;
    private Canvas mNativeCanvas;
    private Paint mNativePaint;

    private int mDisplayOrientation;
    private int mImageOrientation;

    public CameraPreview(Context context) {
        super(context);
    }

    public CameraPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CameraPreview(Activity activity, Camera camera, int cameraId, ImageView nativeImageView) {
        super(activity);

        mCameraId = cameraId;
        mActivity = activity;
        mCamera = camera;
        mNativeImageView = nativeImageView;

        // Install a SurfaceHolder.Callback so we get notified when the underlying surface is created and destroyed.
        SurfaceHolder holder = getHolder();
        if (holder != null) {
            holder.addCallback(this);
            holder.setKeepScreenOn(true);
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
        startPreview(holder);
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();

        if (mNativeTask != null) {
            mNativeTask.cancel(false);
        }
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // preview surface does not exist
        if (holder.getSurface() == null) {
            return;
        }

        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_4444);
        mNativeCanvas = new Canvas(bitmap);
        mNativePaint = new Paint();
        mNativePaint.setColor(Color.GREEN);
        mNativePaint.setStrokeWidth(STROKE_WIDTH);
        mNativeImageView.setImageBitmap(bitmap);

        stopPreview();
        startPreview(holder);
    }

    private void startPreview(SurfaceHolder holder) {
        Timber.d("startPreview");

        try {
            mCamera.setPreviewCallback(this);
            mCamera.setPreviewDisplay(holder);
            setDisplayOrientation();
            mCamera.startPreview();
        } catch (Exception e) {
            Timber.e(e, "Error starting preview: " + e.getMessage());
        }
    }

    private void stopPreview() {
        if(mCamera != null) {
            try {
                mCamera.setPreviewCallback(null);
                mCamera.stopPreview();
            } catch (Exception e) {
                Timber.e(e, "Error stopping preview: " + e.getMessage());
            }
        }
    }

    public void setDisplayOrientation() {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(mCameraId, info);

        int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;

        switch (rotation) {
            case Surface.ROTATION_0: degrees = 0; break;
            case Surface.ROTATION_90: degrees = 90; break;
            case Surface.ROTATION_180: degrees = 180; break;
            case Surface.ROTATION_270: degrees = 270; break;
        }

        degrees = (degrees + 45) / 90 * 90;

        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
            mImageOrientation = (info.orientation - degrees + 360) % 360;
        } else {  // back-facing
            result = (info.orientation - degrees + 360) % 360;
            mImageOrientation = (info.orientation + degrees) % 360;
        }

        if(mDisplayOrientation != result) {
            mDisplayOrientation = result;
            mCamera.setDisplayOrientation(result);
        }
    }

    @Override
    public void onPreviewFrame(byte[] bytes, Camera camera) {
        if (bytes == null || camera == null) {
            return;
        }

        if (mNativeTask == null) {
            mNativeTask = new ProcessAsyncTask(this);
            mNativeTask.execute();
        }

        if (mNativeTask.getStatus() == AsyncTask.Status.RUNNING) {
            Camera.Size size = camera.getParameters().getPreviewSize();
            mNativeTask.setFrame(bytes, mImageOrientation, size.width, size.height);
        }
    }

    /**
     * Asynchronous task for processing
     */
    private static class ProcessAsyncTask extends AsyncTask<Void, Void, Boolean> {
        private WeakReference<CameraPreview> mCameraPreview;

        private class Frame {
            byte[] yuv;
            int[] rgba;
            int rotation;
            int width;
            int height;
        }

        private volatile boolean mIsFrameSet = false;
        private Frame mFrame = new Frame();
        private Bitmap mBitmap;

        ProcessAsyncTask(CameraPreview cameraPreview) {
            mCameraPreview = new WeakReference<>(cameraPreview);
        }


        int NativeCall(int rotation, int width, int height, byte yuv[], int[] rgba)
        {
            CameraPreview cp = mCameraPreview.get();
            if (cp == null || cp.mActivity.isFinishing()) return -1;

            return cp.NativeProcess(rotation, width, height, yuv, rgba);
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            while(!isCancelled()) {
                if(mIsFrameSet) {
                    int argbW, argbH;

                    if(mFrame.rotation == 90 || mFrame.rotation == 270) {
                        argbW = mFrame.height;
                        argbH = mFrame.width;
                    }
                    else {
                        argbW = mFrame.width;
                        argbH = mFrame.height;
                    }

                    int retVal = NativeCall(mFrame.rotation, mFrame.width, mFrame.height, mFrame.yuv, mFrame.rgba);
                    if (retVal == 0) {
                        if (mBitmap == null || mBitmap.getWidth() != argbW || mBitmap.getHeight() != argbH) {
                            mBitmap = Bitmap.createBitmap(argbW, argbH, Bitmap.Config.ARGB_8888);
                        }

                        mBitmap.setPixels(mFrame.rgba, 0, argbW, 0, 0, argbW, argbH);

                        publishProgress();
                    } else {
                        Timber.w("[JNIFACE] FaceApp returned with error code: %d.", retVal);
                    }

                    mIsFrameSet = false;
                }

                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                    Timber.e(e, "InterruptedException in doInBackground(...)");
                    return false;
                }
            }

            return true;
        }

        @Override
        protected void onProgressUpdate(Void... progress) {
            CameraPreview cp = mCameraPreview.get();

            if (cp == null || cp.mActivity.isFinishing()) return;

            int ivWidth = cp.mNativeImageView.getWidth();
            int ivHeight = cp.mNativeImageView.getHeight();

            mBitmap = Bitmap.createScaledBitmap(mBitmap, ivWidth, ivHeight, true);

            cp.mNativeCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
            cp.mNativeCanvas.drawBitmap(mBitmap, 0, 0, cp.mNativePaint);
            cp.mNativeImageView.draw(cp.mNativeCanvas);
            cp.mNativeImageView.invalidate();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            super.onPostExecute(result);
        }

        void setFrame(byte[] yuv, int rotation, int width, int height) {
            if (!isCancelled() && !mIsFrameSet) {
                if (mFrame.yuv == null || mFrame.yuv.length != yuv.length) {
                    mFrame.yuv = new byte[yuv.length];
                }

                if (mFrame.rgba == null || mFrame.rgba.length != width * height) {
                    mFrame.rgba = new int[width * height];
                }

                System.arraycopy(yuv, 0, mFrame.yuv, 0, yuv.length);
                mFrame.rotation = rotation;
                mFrame.width = width;
                mFrame.height = height;
                mIsFrameSet = true;
            }
        }
    }
}
