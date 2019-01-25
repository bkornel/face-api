package com.yalantis.cameramodule.fragment;

import android.content.res.Resources;
import android.graphics.ImageFormat;
import android.hardware.Camera;
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

import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.common.Configuration;
import com.yalantis.cameramodule.common.camera.FlashMode;
import com.yalantis.cameramodule.common.camera.FocusMode;
import com.yalantis.cameramodule.common.camera.PictureSize;
import com.yalantis.cameramodule.common.camera.Ratio;
import com.yalantis.cameramodule.control.CameraPreview;
import com.yalantis.cameramodule.control.FocusView;
import com.yalantis.cameramodule.event.EventArgs;
import com.yalantis.cameramodule.event.FlashModeArgs;
import com.yalantis.cameramodule.event.FocusModeArgs;
import com.yalantis.cameramodule.event.PictureSizeArgs;
import com.yalantis.cameramodule.interfaces.FocusCallback;
import com.yalantis.cameramodule.interfaces.KeyEventsListener;
import com.yalantis.cameramodule.interfaces.PhotoSavedListener;
import com.yalantis.cameramodule.interfaces.PhotoTakenCallback;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import timber.log.Timber;

@SuppressWarnings("deprecation")
public class CameraFragment
        extends com.yalantis.cameramodule.fragment.BaseFragment
        implements PhotoSavedListener, KeyEventsListener, FocusCallback {


    private PhotoTakenCallback mPhotoTakenCallback;
    private OrientationEventListener mOrientationEventListener;
    private int mScreenWidth;
    private int mScreenHeight;
    private int mNavigationBarHeight;
    private int mStatusBarHeight;
    private List<Integer> mZoomRatios;
    private int mZoomIndex;
    private int mMinZoomIndex;
    private int mMaxZoomIndex;
    private ArrayList<PictureSize> mPreviewSizes = new ArrayList<>();
    private PictureSize mPreviewSize;
    private Camera mCamera;
    private CameraPreview mCameraPreview;
    private FocusView mFocusView;
    private ViewGroup mViewGroup;
    private View mCaptureButton;
    private View mControlsLayout;
    private ProgressBar mProgressBar;
    private TextView mZoomRatioTextView;
    private int mCameraId;
    private int mDeviceOrientation;
    private Camera.PictureCallback mPictureCallback = new Camera.PictureCallback() {

        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            if (mPhotoTakenCallback != null) {
                // TODO: copy to following: mCameraPreview.mNativeTask.mBitmap
                mPhotoTakenCallback.photoTaken(data.clone(), mDeviceOrientation);
            }
            camera.startPreview();
            mFocusView.onPictureTaken();
        }

    };

    public static CameraFragment factory(PhotoTakenCallback callback) {
        CameraFragment fragment = new CameraFragment();
        fragment.mPhotoTakenCallback = callback;
        return fragment;
    }

    @SuppressWarnings("JniMissingFunction")
    private native int NativeReset();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initFragment();
    }

    private void initFragment() {
        mCamera = getCameraInstance(Configuration.i.useFrontCamera());
        if (mCamera == null) {
            return;
        }

        Camera.Parameters parameters = mCamera.getParameters();
        mZoomRatios = parameters.getZoomRatios();
        mZoomIndex = mMinZoomIndex = 0;
        mMaxZoomIndex = parameters.getMaxZoom();

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
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_camera, container, false);
        CameraSettingsDialogFragment settingsFragment = CameraSettingsDialogFragment.getInstance();

        mViewGroup = view.findViewById(R.id.camera_preview);
        mProgressBar = view.findViewById(R.id.progress);
        mZoomRatioTextView = view.findViewById(R.id.zoom_ratio);
        mControlsLayout = view.findViewById(R.id.controls_layout);

        mCaptureButton = view.findViewById(R.id.capture);
        if (mCaptureButton != null) {
            mCaptureButton.setOnClickListener(v -> takePhoto());
        }

        View cameraChangeBtn = view.findViewById(R.id.camera_change);
        if (cameraChangeBtn != null) {
            cameraChangeBtn.setOnClickListener(v -> changeCamera());
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
        mCameraPreview = new CameraPreview(mActivity, mCamera, mCameraId, nativeImageView);
        mFocusView = new FocusView(mActivity, mCamera, focusImageView, this, this);
        mViewGroup.addView(mCameraPreview);
        mViewGroup.addView(mFocusView);
        mViewGroup.addView(nativeImageView);
        mViewGroup.addView(focusImageView);

        setPreviewContainerSize();

        if (mZoomRatioTextView != null) {
            setZoomRatioText(mZoomIndex);
        }

        if (mControlsLayout != null) {
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);

            params.topMargin = mStatusBarHeight;
            params.bottomMargin = mNavigationBarHeight;

            mControlsLayout.setLayoutParams(params);
        }
    }

    private Camera getCameraInstance(boolean useFrontCamera) {
        try {
            return Camera.open(getCameraId(useFrontCamera));
        } catch (Exception e) {
            Timber.e(e, getString(R.string.camera_unavailable_caption));
        }
        return null;
    }

    private int getCameraId(boolean useFrontCamera) {
        int count = Camera.getNumberOfCameras();
        int result = -1;

        if (count > 0) {
            result = 0;

            Camera.CameraInfo info = new Camera.CameraInfo();
            for (int i = 0; i < count; i++) {
                Camera.getCameraInfo(i, info);

                if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK && !useFrontCamera) {
                    result = i;
                    break;
                } else if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT && useFrontCamera) {
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

    private int getPixelSizeByName(String name) {
        Resources resources = getResources();
        int resourceId = resources.getIdentifier(name, "dimen", "android");
        if (resourceId > 0) {
            return resources.getDimensionPixelSize(resourceId);
        }
        return 0;
    }

    @Override
    public void onFocused(Camera camera) {
        // TODO: should be another callback as the onfocused is called independently from taking picture
        //camera.takePicture(null, null, mPictureCallback);
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

    private void onPreviewSizeChanged(Object sender, EventArgs args) {
        if(!(args instanceof PictureSizeArgs)) return;

        mPreviewSize = ((PictureSizeArgs)args).getPictureSize();
        Configuration.i.setPreviewSizeId(mPreviewSize.getId());

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        mCamera.setParameters(parameters);

        setPreviewContainerSize();

        if (mViewGroup != null) {
            mViewGroup.removeAllViews();
        }

        initView();
    }

    public void onFlashModeChanged(Object sender, EventArgs args) {
        if(!(args instanceof FlashModeArgs)) return;

        FlashMode flashMode = ((FlashModeArgs)args).getFlashMode();
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

    public void onFocusModeChanged(Object sender, EventArgs args) {
        if(!(args instanceof FocusModeArgs)) return;

        FocusMode focusMode = ((FocusModeArgs)args).getFocusMode();

        Configuration.i.setFocusMode(focusMode);
        mFocusView.resetCameraFocus();
    }

    @Override
    public void zoomIn() {
        if (++mZoomIndex > mMaxZoomIndex) {
            mZoomIndex = mMaxZoomIndex;
        }
        setZoom(mZoomIndex);
    }

    @Override
    public void zoomOut() {
        if (--mZoomIndex < mMinZoomIndex) {
            mZoomIndex = mMinZoomIndex;
        }
        setZoom(mZoomIndex);
    }

    @Override
    public void changeCamera() {
        mCameraId = (mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT ? Camera.CameraInfo.CAMERA_FACING_BACK : Camera.CameraInfo.CAMERA_FACING_FRONT);

        Configuration.i.setUseFrontCamera(mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT);

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

    @Override
    public void takePhoto() {
        mCaptureButton.setEnabled(false);
        mCaptureButton.setVisibility(View.INVISIBLE);
        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.VISIBLE);
        }
        mFocusView.takePicture();
    }

    private void setZoom(int index) {
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setZoom(index);
        mCamera.setParameters(parameters);
        setZoomRatioText(index);
    }

    private void setZoomRatioText(int index) {
        if (mZoomRatioTextView != null) {
            float value = mZoomRatios.get(index) / 100.0f;
            mZoomRatioTextView.setText(getString(R.string.zoom_changed_caption, value));
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

    @Override
    public void photoSaved(String path) {
        mCaptureButton.setEnabled(true);
        mCaptureButton.setVisibility(View.VISIBLE);
        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.GONE);
        }
    }

    // This affects the pictures returned from JPEG Camera.PictureCallback
    private void initOrientationListener() {
        // The value from OrientationEventListener is relative to the natural orientation of the device
        mOrientationEventListener = new OrientationEventListener(mActivity) {

            @Override
            public void onOrientationChanged(int orientation) {
                if (mCamera == null || orientation == ORIENTATION_UNKNOWN) return;

                Camera.CameraInfo info = new Camera.CameraInfo();
                Camera.getCameraInfo(mCameraId, info);

                orientation = (orientation + 45) / 90 * 90;

                // CameraInfo.orientation is the angle between camera orientation and natural device orientation
                int deviceOri;
                if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                    deviceOri = (info.orientation - orientation + 360) % 360;
                } else {  // back-facing camera
                    deviceOri = (info.orientation + orientation) % 360;
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
