package com.face.control;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
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
public class CameraView extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private int mCameraId;
    private Activity mActivity;
    private Camera mCamera;
    private ProcessAsyncTask mNativeTask;
    private ImageView mNativeImageView;
    private Canvas mNativeCanvas;
    private int mDisplayOrientation;
    private int mImageOrientation;
    public CameraView(Context iContext) {
        super(iContext);
    }

    public CameraView(Context iContext, AttributeSet iAttributeSet) {
        super(iContext, iAttributeSet);
    }

    public CameraView(Activity iActivity, Camera iCamera, int iCameraId, ImageView iImageView) {
        super(iActivity);

        mCameraId = iCameraId;
        mActivity = iActivity;
        mCamera = iCamera;
        mNativeImageView = iImageView;

        // Install a SurfaceHolder.Callback so we get notified when the underlying surface is created and destroyed.
        SurfaceHolder holder = getHolder();
        if (holder != null) {
            holder.addCallback(this);
            holder.setKeepScreenOn(true);
        }
    }

    @SuppressWarnings("JniMissingFunction")
    private native int NativeProcess(int iRotation, int iWidth, int iHeight, byte iYUV[], int[] iRGBA);

    public void surfaceCreated(SurfaceHolder iHolder) {
        startPreview(iHolder);
    }

    public void surfaceDestroyed(SurfaceHolder iHolder) {
        stopPreview();

        if (mNativeTask != null) {
            cancel();
        }
    }

    public void surfaceChanged(SurfaceHolder iHolder, int iFormat, int iWidth, int iHeight) {
        // preview surface does not exist
        if (iHolder.getSurface() == null) {
            return;
        }

        Bitmap bitmap = Bitmap.createBitmap(iWidth, iHeight, Bitmap.Config.ARGB_8888);
        mNativeCanvas = new Canvas(bitmap);
        mNativeImageView.setImageBitmap(bitmap);

        stopPreview();
        startPreview(iHolder);
    }

    private void startPreview(SurfaceHolder iHolder) {
        Timber.d("startPreview");

        try {
            mCamera.setPreviewCallback(this);
            mCamera.setPreviewDisplay(iHolder);
            setDisplayOrientation();
            mCamera.startPreview();
        } catch (Exception e) {
            Timber.e(e, "Error starting preview: " + e.getMessage());
        }
    }

    private void stopPreview() {
        if (mCamera != null) {
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
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:
                degrees = 270;
                break;
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

        if (mDisplayOrientation != result) {
            mDisplayOrientation = result;
            mCamera.setDisplayOrientation(result);
        }
    }

    @Override
    public void onPreviewFrame(byte[] iBytes, Camera iCamera) {
        if (iBytes == null || iCamera == null) {
            return;
        }

        if (mNativeTask == null) {
            mNativeTask = new ProcessAsyncTask(this);
            mNativeTask.execute();
        }

        if (mNativeTask.getStatus() == AsyncTask.Status.RUNNING) {
            Camera.Size size = iCamera.getParameters().getPreviewSize();
            mNativeTask.setFrame(iBytes, mImageOrientation, size.width, size.height);
        }
    }

    public Bitmap getOutputBitmap() {
        if (mNativeTask != null) {
            return mNativeTask.getOutputBitmap();
        }
        return null;
    }

    public void cancel() {
        if (mNativeTask != null) {
            mNativeTask.cancel(false);

            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
                e.printStackTrace();
                Timber.e(e, "InterruptedException in cancel()");
            }
        }
    }

    private static class ProcessAsyncTask extends AsyncTask<Void, Void, Boolean> {
        private static final Object sOutputBitmapLock = new Object();
        private WeakReference<CameraView> mCameraPreview;
        private volatile boolean mIsInputSet = false;
        private volatile boolean mIsOutputSet = false;
        private NativeFrame mNativeFrame = new NativeFrame();
        private Bitmap mOutputBitmap;
        ProcessAsyncTask(CameraView cameraPreview) {
            mCameraPreview = new WeakReference<>(cameraPreview);
        }

        public Bitmap getOutputBitmap() {
            synchronized (sOutputBitmapLock) {
                if (mIsOutputSet) {
                    return mOutputBitmap.copy(mOutputBitmap.getConfig(), true);
                }
            }
            return null;
        }

        protected int NativeCall(int iRotation, int iWidth, int iHeight, byte iYUV[], int[] iRGBA) {
            CameraView cp = mCameraPreview.get();
            if (cp == null || cp.mActivity.isFinishing()) return -1;

            return cp.NativeProcess(iRotation, iWidth, iHeight, iYUV, iRGBA);
        }

        protected void setFrame(byte[] iYUV, int iRotation, int iWidth, int iHeight) {
            if (!isCancelled() && !mIsInputSet) {
                if (mNativeFrame.YUV == null || mNativeFrame.YUV.length != iYUV.length) {
                    mNativeFrame.YUV = new byte[iYUV.length];
                }

                if (mNativeFrame.RGBA == null || mNativeFrame.RGBA.length != iWidth * iHeight) {
                    mNativeFrame.RGBA = new int[iWidth * iHeight];
                }

                System.arraycopy(iYUV, 0, mNativeFrame.YUV, 0, iYUV.length);
                mNativeFrame.rotation = iRotation;
                mNativeFrame.width = iWidth;
                mNativeFrame.height = iHeight;
                mIsInputSet = true;
                mIsOutputSet = false;
            }
        }

        @Override
        protected Boolean doInBackground(Void... iParams) {
            while (!isCancelled()) {
                if (mIsInputSet) {
                    int argbW, argbH;

                    if (mNativeFrame.rotation == 90 || mNativeFrame.rotation == 270) {
                        argbW = mNativeFrame.height;
                        argbH = mNativeFrame.width;
                    } else {
                        argbW = mNativeFrame.width;
                        argbH = mNativeFrame.height;
                    }

                    int retVal = NativeCall(mNativeFrame.rotation, mNativeFrame.width, mNativeFrame.height, mNativeFrame.YUV, mNativeFrame.RGBA);
                    if (retVal == 0) {
                        synchronized (sOutputBitmapLock) {
                            if (mOutputBitmap == null || mOutputBitmap.getWidth() != argbW || mOutputBitmap.getHeight() != argbH) {
                                mOutputBitmap = Bitmap.createBitmap(argbW, argbH, Bitmap.Config.ARGB_8888);
                            }

                            mOutputBitmap.setPixels(mNativeFrame.RGBA, 0, argbW, 0, 0, argbW, argbH);
                            mIsOutputSet = true;
                        }
                        publishProgress();
                    } else {
                        Timber.w("[JNIFACE] FaceApp returned with error code: %d.", retVal);
                    }

                    mIsInputSet = false;
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
        protected void onProgressUpdate(Void... iProgress) {
            CameraView cp = mCameraPreview.get();

            if (!mIsOutputSet || cp == null || cp.mActivity.isFinishing()) return;

            int ivWidth = cp.mNativeImageView.getWidth();
            int ivHeight = cp.mNativeImageView.getHeight();
            Bitmap scaledBitmap = Bitmap.createScaledBitmap(mOutputBitmap, ivWidth, ivHeight, true);

            cp.mNativeCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
            cp.mNativeCanvas.drawBitmap(scaledBitmap, 0, 0, null);
            cp.mNativeImageView.draw(cp.mNativeCanvas);
            cp.mNativeImageView.invalidate();
        }

        @Override
        protected void onPostExecute(Boolean iResult) {
            super.onPostExecute(iResult);
        }

        private class NativeFrame {
            byte[] YUV;     // input frame
            int[] RGBA;     // output frame
            int rotation;
            int width;
            int height;
        }
    }
}
