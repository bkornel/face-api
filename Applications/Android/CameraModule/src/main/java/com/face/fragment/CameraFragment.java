package com.face.fragment;

import android.hardware.Camera;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.face.R;
import com.face.common.Configuration;
import com.face.common.camera.FlashMode;
import com.face.common.camera.FocusMode;
import com.face.common.camera.PictureSize;
import com.face.common.camera.Ratio;
import com.face.common.image.SavePhotoTask;
import com.face.event.Event;
import com.face.event.EventArgs;
import com.face.event.FlashModeArgs;
import com.face.event.FocusModeArgs;
import com.face.event.FocusedArgs;
import com.face.event.IEvent;
import com.face.event.PhotoSavedArgs;
import com.face.event.PictureSizeArgs;
import com.face.event.ZoomChangedArgs;
import com.face.view.CameraView;
import com.face.view.FocusView;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import timber.log.Timber;

@SuppressWarnings("deprecation")
public class CameraFragment extends BaseFragment {

    public final IEvent<PhotoSavedArgs> PhotoSaved = new Event<>();

    private OrientationEventListener mOrientationEventListener;
    private int mScreenWidth;
    private int mScreenHeight;
    private ArrayList<PictureSize> mPreviewSizes = new ArrayList<>();
    private PictureSize mPreviewSize;
    private Camera mCamera;
    private CameraView mCameraPreview;
    private FocusView mFocusView;
    private SettingsFragment mSettingsFragment;
    private ViewGroup mViewGroup;
    private View mCaptureButton;
    private View mCameraSettingsButton;
    private ProgressBar mProgressBar;
    private int mCameraId;
    private int mDeviceOrientation;
    private SavePhotoTask mSavePhotoTask;
    private boolean mCapturePressed;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        mCapturePressed = false;
        mCamera = getCameraInstance(Configuration.i.useFrontCamera());

        initializeFocusMode();
        initializeFlashMode();
        initializePreviewSize();
        initializeScreenParams();
    }

    @Override
    public View onCreateView(LayoutInflater iInflater, ViewGroup iContainer, Bundle iSavedInstanceState) {
        View view = iInflater.inflate(R.layout.fragment_camera, iContainer, false);

        mViewGroup = view.findViewById(R.id.camera_preview);
        mProgressBar = view.findViewById(R.id.progress);
        mCameraSettingsButton = view.findViewById(R.id.camera_settings);

        mCaptureButton = view.findViewById(R.id.capture);
        if (mCaptureButton != null) {
            mCaptureButton.setOnClickListener(v -> onCapturePressed());
        }

        View cameraChangeBtn = view.findViewById(R.id.camera_change);
        if (cameraChangeBtn != null) {
            cameraChangeBtn.setOnClickListener(v -> onCameraChangePressed());
        }

        View nativeResetBtn = view.findViewById(R.id.native_reset);
        if (nativeResetBtn != null) {
            nativeResetBtn.setOnClickListener(v -> {
                Toast toast = Toast.makeText(getActivity().getApplicationContext(), "Clearing users", Toast.LENGTH_SHORT);
                toast.show();

                if (mCameraPreview != null) {
                    mCameraPreview.clearUsers();
                }
            });
        }

        initializeViewGroup();

        return view;
    }

    private void initializeViewGroup() {
        if (mViewGroup != null) {
            mViewGroup.removeAllViews();
        }

        // Set the camera preview and focus view
        mCameraPreview = new CameraView(mActivity, mCamera, mCameraId);
        mFocusView = new FocusView(mActivity, mCamera);
        mViewGroup.addView(mCameraPreview);
        mViewGroup.addView(mFocusView);
        mViewGroup.addView(mCameraPreview.getNativeImageView());
        mViewGroup.addView(mFocusView.getFocusImageView());

        // Set preview container size
        Ratio ratio = mPreviewSize.getRatio();
        mScreenHeight = (mScreenWidth / ratio.mHeight) * ratio.mWidth;
        mViewGroup.setLayoutParams(new RelativeLayout.LayoutParams(mScreenWidth, mScreenHeight));

        // Setup the settings dialog
        mSettingsFragment = new SettingsFragment();
        if (mCameraSettingsButton != null) {
            mCameraSettingsButton.setOnClickListener(v -> mSettingsFragment.show(getFragmentManager()));
        }

        // Register event handlers
        mFocusView.ZoomChanged.addHandler(this::onZoomChanged);
        mFocusView.Focused.addHandler(this::onFocused);
        mSettingsFragment.PreviewSizeChanged.addHandler(this::onPreviewSizeChanged);
        mSettingsFragment.FlashModeChanged.addHandler(this::onFlashModeChanged);
        mSettingsFragment.FocusModeChanged.addHandler(this::onFocusModeChanged);

        hideActionBar();

        mFocusView.startFocusing();
    }

    private Camera getCameraInstance(boolean iUseFrontCamera) {
        try {
            return Camera.open(getCameraId(iUseFrontCamera));
        } catch (Exception e) {
            Timber.e(e, getString(R.string.camera_unavailable_caption));
        }
        return null;
    }

    private int getCameraId(boolean iUseFrontCamera) {
        int count = Camera.getNumberOfCameras();
        int result = -1;

        if (count > 0) {
            result = 0;

            Camera.CameraInfo info = new Camera.CameraInfo();
            for (int i = 0; i < count; i++) {
                Camera.getCameraInfo(i, info);

                if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK && !iUseFrontCamera) {
                    result = i;
                    break;
                } else if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT && iUseFrontCamera) {
                    result = i;
                    break;
                }
            }
        }

        mCameraId = result;

        return result;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mCamera != null) {
            try {
                mCamera.reconnect();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        if (mOrientationEventListener == null) {
            initOrientationListener();
        }
        mOrientationEventListener.enable();
    }

    private void initializeScreenParams() {
        DisplayMetrics metrics = new DisplayMetrics();
        mActivity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mScreenWidth = metrics.widthPixels;
        mScreenHeight = metrics.heightPixels;
    }

    private void onFocused(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FocusedArgs)) return;

        if (mCapturePressed) {
            try {
                mSavePhotoTask = new SavePhotoTask(mCameraPreview.getOutputBitmap());
                mSavePhotoTask.PhotoSaved.addHandler(this::onPhotoSaved);
                mSavePhotoTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            } catch (IllegalStateException ex) {
                Timber.e(ex.getMessage());
            }
            mCapturePressed = false;
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mOrientationEventListener != null) {
            mOrientationEventListener.disable();
            mOrientationEventListener = null;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mCamera != null) {
            mCamera.release();
            mCamera = null;
        }
    }

    private void onPreviewSizeChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof PictureSizeArgs)) return;

        mPreviewSize = ((PictureSizeArgs) iArgs).getPictureSize();
        Configuration.i.setPreviewSizeId(mPreviewSize.getId());
        initializeViewGroup();
    }

    private void onFlashModeChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FlashModeArgs)) return;

        String cameraFlashMode = ((FlashModeArgs) iArgs).getCameraFlashMode();
        Camera.Parameters parameters = mCamera.getParameters();
        List<String> supportedFlashModes = parameters.getSupportedFlashModes();

        if (cameraFlashMode != null && supportedFlashModes.contains(cameraFlashMode)) {
            Configuration.i.setFlashMode(((FlashModeArgs) iArgs).getFlashMode());
            parameters.setFlashMode(cameraFlashMode);
            mCamera.setParameters(parameters);
        }
    }

    private void onFocusModeChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FocusModeArgs)) return;

        FocusMode focusMode = ((FocusModeArgs) iArgs).getFocusMode();

        Configuration.i.setFocusMode(focusMode);
        mFocusView.resetCameraFocus();
        mFocusView.startFocusing();
    }

    private void onCameraChangePressed() {
        mCameraId = (mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT ? Camera.CameraInfo.CAMERA_FACING_BACK : Camera.CameraInfo.CAMERA_FACING_FRONT);

        Configuration.i.setUseFrontCamera(mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT);

        if (mViewGroup != null) {
            mViewGroup.removeAllViews();
        }

        if (mCamera != null) {
            mCamera.release();
            mCamera = null;
        }

        mCamera = getCameraInstance(Configuration.i.useFrontCamera());

        initializeFocusMode();
        initializeFlashMode();
        initializePreviewSize();
        initializeScreenParams();
        initializeViewGroup();
    }

    public void onCapturePressed() {
        if (!mCapturePressed) {
            mCaptureButton.setEnabled(false);
            mCaptureButton.setVisibility(View.INVISIBLE);

            if (mProgressBar != null) {
                mProgressBar.setVisibility(View.VISIBLE);
            }

            mCapturePressed = true;
            mFocusView.startFocusing();
        }
    }

    public void onZoomInPressed() {
        if (mFocusView != null) mFocusView.zoomIn();
    }

    public void onZoomOutPressed() {
        if (mFocusView != null) mFocusView.zoomOut();
    }

    private void onZoomChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof ZoomChangedArgs)) return;

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setZoom(((ZoomChangedArgs) iArgs).getIndex());
        mCamera.setParameters(parameters);
    }

    private void initializeFocusMode() {
        Camera.Parameters parameters = mCamera.getParameters();
        Configuration.i.setFocusMode(FocusMode.AUTO);
        Configuration.i.setFocusOnTouchSupported(parameters.getMaxNumFocusAreas() > 0);

        List<String> supportedFocusModes = parameters.getSupportedFocusModes();
        if (supportedFocusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
            mCamera.setParameters(parameters);
        }
    }

    private void initializeFlashMode() {
        Camera.Parameters parameters = mCamera.getParameters();
        List<String> flashModes = parameters.getSupportedFlashModes();
        Configuration.i.setFlashMode(FlashMode.AUTO);
        Configuration.i.setFlashSupported(!(flashModes.size() == 0 || (flashModes.size() == 1 && flashModes.get(0).equalsIgnoreCase("off"))));

        List<String> supportedFlashModes = parameters.getSupportedFlashModes();
        if (supportedFlashModes.contains(Camera.Parameters.FLASH_MODE_AUTO)) {
            parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
            mCamera.setParameters(parameters);
        }
    }

    private void initializePreviewSize() {
        Camera.Parameters parameters = mCamera.getParameters();
        List<Camera.Size> spsizes = parameters.getSupportedPreviewSizes();
        mPreviewSizes.clear();

        // Fill preview sizes
        int idx = 0;
        for (Camera.Size s : spsizes) {
            Ratio r = Ratio.pickRatio(s.width, s.height);
            if (r != null) {
                mPreviewSizes.add(new PictureSize(idx++, s.width, s.height));
            }
        }

        // Sort preview sizes
        int count = mPreviewSizes.size();
        while (count > 2) {
            for (int i = 0; i < count - 1; i++) {
                PictureSize current = mPreviewSizes.get(i);
                PictureSize next = mPreviewSizes.get(i + 1);

                if (current.getArea() < next.getArea()) {
                    mPreviewSizes.set(i, next);
                    mPreviewSizes.set(i + 1, current);
                }
            }
            count--;
        }

        // Get the initial preview size
        int id = 0;
        int id_16_9 = -1;
        int mpx = 307200;

        for (int i = 1; i < mPreviewSizes.size(); i++) {
            int a0 = mPreviewSizes.get(id).getArea();
            int ai = mPreviewSizes.get(i).getArea();

            if (Math.abs(ai - mpx) < Math.abs(a0 - mpx)) {
                if (mPreviewSizes.get(i).getRatio() == Ratio.R_16x9) {
                    id_16_9 = i;
                }
                id = i;
            }
        }

        // if there is a 16:9 ratio then prefer that one
        if (id_16_9 != -1) {
            id = id_16_9;
        }

        mPreviewSize = mPreviewSizes.get(id);

        Configuration.i.setPreviewSizeId(id);
        Configuration.i.setPreviewSizeList(mPreviewSizes);
    }

    private void onPhotoSaved(Object iSender, EventArgs iArgs) {
        if (iArgs != null) {
            if (!(iArgs instanceof PhotoSavedArgs)) return;

            PhotoSaved.raise(iSender, (PhotoSavedArgs) iArgs);
        }

        mCaptureButton.setEnabled(true);
        mCaptureButton.setVisibility(View.VISIBLE);

        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.GONE);
        }
    }

    public boolean isSavingInProgress() {
        return mSavePhotoTask != null && mSavePhotoTask.isSavingInProgress();
    }

    // This affects the pictures returned from JPEG Camera.PictureCallback
    private void initOrientationListener() {
        // The value from OrientationEventListener is relative to the natural orientation of the device
        mOrientationEventListener = new OrientationEventListener(mActivity) {

            @Override
            public void onOrientationChanged(int iOrientation) {
                if (mCamera == null || iOrientation == ORIENTATION_UNKNOWN) return;

                Camera.CameraInfo info = new Camera.CameraInfo();
                Camera.getCameraInfo(mCameraId, info);

                iOrientation = (iOrientation + 45) / 90 * 90;

                // CameraInfo.orientation is the angle between camera orientation and natural device orientation
                int deviceOri;
                if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                    deviceOri = (info.orientation - iOrientation + 360) % 360;
                } else {  // back-facing camera
                    deviceOri = (info.orientation + iOrientation) % 360;
                }

                if (deviceOri != mDeviceOrientation) {
                    mDeviceOrientation = deviceOri;

                    Camera.Parameters params = mCamera.getParameters();
                    params.setRotation(mDeviceOrientation);

                    try {
                        mCamera.setParameters(params);
                    } catch (Exception e) {
                        Timber.e(e, "Exception updating camera parameters in onOrientationChanged()");
                    }
                }
            }
        };
    }
}
