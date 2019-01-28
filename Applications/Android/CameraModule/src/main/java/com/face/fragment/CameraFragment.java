package com.face.fragment;

import android.content.res.Resources;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.face.R;
import com.face.common.Configuration;
import com.face.common.camera.FlashMode;
import com.face.common.camera.FocusMode;
import com.face.common.camera.PictureSize;
import com.face.common.camera.Ratio;
import com.face.common.image.SavingPhotoTask;
import com.face.control.CameraView;
import com.face.control.FocusView;
import com.face.event.Event;
import com.face.event.EventArgs;
import com.face.event.FlashModeArgs;
import com.face.event.FocusModeArgs;
import com.face.event.FocusedArgs;
import com.face.event.IEvent;
import com.face.event.PhotoSavedArgs;
import com.face.event.PictureSizeArgs;
import com.face.event.ZoomChangedArgs;

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
    private int mNavigationBarHeight;
    private int mStatusBarHeight;
    private ArrayList<PictureSize> mPreviewSizes = new ArrayList<>();
    private PictureSize mPreviewSize;
    private Camera mCamera;
    private CameraView mCameraPreview;
    private FocusView mFocusView;
    private ViewGroup mViewGroup;
    private View mCaptureButton;
    private View mControlsLayout;
    private ProgressBar mProgressBar;
    private TextView mZoomRatioTextView;
    private int mCameraId;
    private int mDeviceOrientation;
    private SavingPhotoTask mSavingPhotoTask;

    @SuppressWarnings("JniMissingFunction")
    private native int NativeReset();

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);
        initFragment();
    }

    private void initFragment() {
        mCamera = getCameraInstance(Configuration.i.useFrontCamera());
        if (mCamera == null) {
            return;
        }

        Camera.Parameters parameters = mCamera.getParameters();

        initPreviewSize();
        initScreenParams();

        parameters.setPreviewSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        parameters.setPreviewFormat(ImageFormat.NV21);

        List<String> focusModes = parameters.getSupportedFocusModes();
        if (focusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
        }

        mCamera.setParameters(parameters);
    }

    @Override
    public View onCreateView(LayoutInflater iInflater, ViewGroup iContainer, Bundle iSavedInstanceState) {
        View view = iInflater.inflate(R.layout.fragment_camera, iContainer, false);
        SettingsFragment settingsFragment = SettingsFragment.getInstance();

        mViewGroup = view.findViewById(R.id.camera_preview);
        mProgressBar = view.findViewById(R.id.progress);
        mControlsLayout = view.findViewById(R.id.controls_layout);

        mZoomRatioTextView = view.findViewById(R.id.zoom_ratio);
        if (mZoomRatioTextView != null) {
            mZoomRatioTextView.setText(getString(R.string.zoom_changed_caption, 1.0f));
        }

        mCaptureButton = view.findViewById(R.id.capture);
        if (mCaptureButton != null) {
            mCaptureButton.setOnClickListener(v -> onCapturePressed());
        }

        View cameraChangeBtn = view.findViewById(R.id.camera_change);
        if (cameraChangeBtn != null) {
            cameraChangeBtn.setOnClickListener(v -> onCameraChangePressed());
        }

        View cameraSettingsBtn = view.findViewById(R.id.camera_settings);
        if (cameraSettingsBtn != null) {
            cameraSettingsBtn.setOnClickListener(v -> settingsFragment.show(getFragmentManager()));
        }

        View nativeResetBtn = view.findViewById(R.id.native_reset);
        if (nativeResetBtn != null) {
            nativeResetBtn.setOnClickListener(v -> {
                Toast toast = Toast.makeText(getActivity().getApplicationContext(), "Clearing users", Toast.LENGTH_SHORT);
                toast.show();
                NativeReset();
            });
        }

        settingsFragment.PreviewSizeChanged.addHandler(this::onPreviewSizeChanged);
        settingsFragment.FlashModeChanged.addHandler(this::onFlashModeChanged);
        settingsFragment.FocusModeChanged.addHandler(this::onFocusModeChanged);

        initView();

        return view;
    }

    private void initView() {
        ImageView focusImageView = new ImageView(mActivity);
        ImageView nativeImageView = new ImageView(mActivity);
        nativeImageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        nativeImageView.setAdjustViewBounds(true);
        mCameraPreview = new CameraView(mActivity, mCamera, mCameraId, nativeImageView);
        mFocusView = new FocusView(mActivity, mCamera, focusImageView);
        mViewGroup.addView(mCameraPreview);
        mViewGroup.addView(mFocusView);
        mViewGroup.addView(nativeImageView);
        mViewGroup.addView(focusImageView);

        setPreviewContainerSize();

        if (mControlsLayout != null) {
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);

            params.topMargin = mStatusBarHeight;
            params.bottomMargin = mNavigationBarHeight;

            mControlsLayout.setLayoutParams(params);
        }

        mFocusView.ZoomChanged.addHandler(this::onZoomChanged);
        mFocusView.Focused.addHandler(this::onFocused);
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

    private void initScreenParams() {
        DisplayMetrics metrics = new DisplayMetrics();
        mActivity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mScreenWidth = metrics.widthPixels;
        mScreenHeight = metrics.heightPixels;
        mNavigationBarHeight = getNavigationBarHeight();
        mStatusBarHeight = getStatusBarHeight();
    }

    private int getNavigationBarHeight() {
        return getPixelSizeByName("navigation_bar_height");
    }

    private int getStatusBarHeight() {
        return getPixelSizeByName("status_bar_height");
    }

    private int getPixelSizeByName(String iName) {
        Resources resources = getResources();
        int resourceId = resources.getIdentifier(iName, "dimen", "android");
        if (resourceId > 0) {
            return resources.getDimensionPixelSize(resourceId);
        }
        return 0;
    }

    private void onFocused(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FocusedArgs)) return;

        FocusedArgs arguments = ((FocusedArgs) iArgs);

        if (arguments.getInitiator() == FocusedArgs.Initiator.TAKE_PICTURE) {
            try {
                mSavingPhotoTask = new SavingPhotoTask(mCameraPreview.getOutputBitmap());
                mSavingPhotoTask.PhotoSaved.addHandler(this::onPhotoSaved);
                mSavingPhotoTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            } catch (IllegalStateException ex) {
                Timber.e(ex.getMessage());
            }
            mFocusView.resetCameraFocus();
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

        if (mCameraPreview != null) {
            mCameraPreview.cancel();
            NativeReset();
        }

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        mCamera.setParameters(parameters);

        setPreviewContainerSize();

        if (mViewGroup != null) {
            mViewGroup.removeAllViews();
        }

        initView();
    }

    private void onFlashModeChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FlashModeArgs)) return;

        FlashMode flashMode = ((FlashModeArgs) iArgs).getFlashMode();
        Configuration.i.setFlashMode(flashMode);

        Camera.Parameters parameters = mCamera.getParameters();

        switch (flashMode) {
            case ON:
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_ON);
                break;
            case OFF:
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
                break;
            case AUTO:
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
                break;
        }

        mCamera.setParameters(parameters);
    }

    private void onFocusModeChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof FocusModeArgs)) return;

        FocusMode focusMode = ((FocusModeArgs) iArgs).getFocusMode();

        Configuration.i.setFocusMode(focusMode);
        mFocusView.resetCameraFocus();
    }

    private void onCameraChangePressed() {
        mCameraId = (mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT ? Camera.CameraInfo.CAMERA_FACING_BACK : Camera.CameraInfo.CAMERA_FACING_FRONT);

        Configuration.i.setUseFrontCamera(mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT);

        if (mCameraPreview != null) {
            mCameraPreview.cancel();
            NativeReset();
        }

        if (mViewGroup != null) {
            mViewGroup.removeAllViews();
        }

        if (mCamera != null) {
            mCameraPreview.getHolder().removeCallback(mCameraPreview);
            mCamera.release();
            mCamera = null;
        }

        initFragment();
        initView();
    }

    public void onCapturePressed() {
        mCaptureButton.setEnabled(false);
        mCaptureButton.setVisibility(View.INVISIBLE);
        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.VISIBLE);
        }
        mFocusView.takePicture();
    }

    public void onZoomInPressed() {
        if (mFocusView != null) mFocusView.zoomIn();
    }

    public void onZoomOutPressed() {
        if (mFocusView != null) mFocusView.zoomOut();
    }

    private void onZoomChanged(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof ZoomChangedArgs)) return;

        int index = ((ZoomChangedArgs) iArgs).getIndex();
        float zoom = ((ZoomChangedArgs) iArgs).getValue() / 100.0f;

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setZoom(index);
        mCamera.setParameters(parameters);

        if (mZoomRatioTextView != null) {
            mZoomRatioTextView.setText(getString(R.string.zoom_changed_caption, zoom));
        }
    }

    private void setPreviewContainerSize() {
        Ratio ratio = mPreviewSize.getRatio();
        mScreenHeight = (mScreenWidth / ratio.h) * ratio.w;
        mViewGroup.setLayoutParams(new RelativeLayout.LayoutParams(mScreenWidth, mScreenHeight));
    }

    private void initPreviewSize() {
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
        int mpx = 307200;

        for (int i = 1; i < mPreviewSizes.size(); i++) {
            int a0 = mPreviewSizes.get(id).getArea();
            int ai = mPreviewSizes.get(i).getArea();

            if (Math.abs(ai - mpx) < Math.abs(a0 - mpx)) {
                id = i;
                mPreviewSize = mPreviewSizes.get(i);
            }
        }

        Configuration.i.setPreviewSizeId(id);
        Configuration.i.setPreviewSizeList(mPreviewSizes);
    }

    private void onPhotoSaved(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof PhotoSavedArgs)) return;

        PhotoSaved.raise(iSender, (PhotoSavedArgs) iArgs);

        mCaptureButton.setEnabled(true);
        mCaptureButton.setVisibility(View.VISIBLE);

        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.GONE);
        }
    }

    public boolean isSavingInProgress() {
        return mSavingPhotoTask != null && mSavingPhotoTask.isSavingInProgress();
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
