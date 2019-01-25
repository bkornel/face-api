package com.yalantis.cameramodule.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.view.KeyEvent;
import android.widget.Toast;

import com.yalantis.cameramodule.common.Constants;
import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.fragment.CameraFragment;
import com.yalantis.cameramodule.interfaces.PhotoSavedListener;
import com.yalantis.cameramodule.interfaces.PhotoTakenCallback;
import com.yalantis.cameramodule.common.Assets;
import com.yalantis.cameramodule.common.image.PhotoUtil;
import com.yalantis.cameramodule.common.image.SavingPhotoTask;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import timber.log.Timber;

public class CameraActivity
        extends BaseActivity
        implements PhotoTakenCallback, PhotoSavedListener {

    static {
        System.loadLibrary("face_native");
    }

    @SuppressWarnings("JniMissingFunction")
    private native int NativeInitialize(String path);

    private CameraFragment mCameraFragment;
    private boolean mPhotoSavingInProgress;
    private int mPermissionCode = 1234;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        setContentView(R.layout.activity_with_fragment);

        // Check if the permissions are already available.
        List<String> checkPermissions = Arrays.asList(
                Manifest.permission.CAMERA,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE);
        List<String> grantPermissions = new ArrayList<>();

        for (String permission : checkPermissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                grantPermissions.add(permission);
            }
        }

        if (grantPermissions.isEmpty()) {
            onRequestPermissionsGranted();
        } else {
            requestPermissions(grantPermissions.toArray(new String[0]), mPermissionCode);
        }
    }

    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        if (requestCode == mPermissionCode) {
            for (int i = 0; i < grantResults.length; ++i) {
                if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "Permission denied: " + permissions[i], Toast.LENGTH_LONG).show();
                    finish();
                }
            }
        }

        onRequestPermissionsGranted();
    }

    private void onRequestPermissionsGranted() {
        if (!Assets.copyAssetFolder(getAssets(), Constants.APP_NAME, Constants.Directories.WORKING)) {
            Toast.makeText(this, "Assests are not copied.", Toast.LENGTH_LONG).show();
            finish();
        }

        if (NativeInitialize(Constants.Directories.WORKING) != 0) {
            Toast.makeText(this, "Native side is not initialized.", Toast.LENGTH_LONG).show();
            finish();
        }

        mCameraFragment = CameraFragment.factory(this);

        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_content, mCameraFragment)
                .commit();
    }

    @Override
    public void photoTaken(byte[] data, int orientation) {
        mPhotoSavingInProgress = true;

        try {
            new SavingPhotoTask(data, orientation, this).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } catch (IllegalStateException ex) {
            Timber.e(ex.getMessage());
        }
    }

    @Override
    public void photoSaved(String path) {
        mPhotoSavingInProgress = false;
        Timber.d("Photo was saved to: " + path);

        Intent intent = new Intent(this, PhotoPreviewActivity.class);
        intent.putExtra(BasePhotoActivity.EXTRAS.PATH, path);
        startActivityForResult(intent, BasePhotoActivity.EXTRAS.REQUEST_PHOTO_EDIT);

        mCameraFragment.photoSaved(path);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == BasePhotoActivity.EXTRAS.REQUEST_PHOTO_EDIT) {
            switch (resultCode) {
                case BasePhotoActivity.EXTRAS.RESULT_DELETED:
                    String path = data.getStringExtra(BasePhotoActivity.EXTRAS.PATH);
                    PhotoUtil.deletePhoto(path);
                    break;
            }
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, @NonNull KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOLUME_UP:
                mCameraFragment.onZoomInPressed();
                return true;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                mCameraFragment.onZoomOutPressed();
                return true;
            case KeyEvent.KEYCODE_BACK:
                onBackPressed();
                return true;
            case KeyEvent.KEYCODE_CAMERA:
                mCameraFragment.onCapturePressed();
                return true;
        }
        return false;
    }

    @Override
    public void onBackPressed() {
        if (!mPhotoSavingInProgress) {
            super.onBackPressed();
        }
    }
}
