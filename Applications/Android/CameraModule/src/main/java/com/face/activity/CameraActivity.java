package com.face.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import android.view.KeyEvent;
import android.widget.Toast;

import com.face.R;
import com.face.common.Assets;
import com.face.common.Constants;
import com.face.common.Native;
import com.face.common.image.PhotoUtil;
import com.face.event.EventArgs;
import com.face.event.PhotoSavedArgs;
import com.face.fragment.CameraFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import timber.log.Timber;

public class CameraActivity extends BaseActivity {

    static {
        System.loadLibrary("FaceNative");
    }

    private CameraFragment mCameraFragment;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        hideActionBar();
        setContentView(R.layout.activity_camera);

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
            requestPermissions(grantPermissions.toArray(new String[0]), Constants.PERMISSION_CODE);
        }
    }

    public void onRequestPermissionsResult(int iRequestCode, @NonNull String[] iPermissions, @NonNull int[] iGrantResults) {
        if (iRequestCode == Constants.PERMISSION_CODE) {
            for (int i = 0; i < iGrantResults.length; ++i) {
                if (iGrantResults[i] != PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "Permission denied: " + iPermissions[i], Toast.LENGTH_LONG).show();
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

        if (Native.i.initialize(Constants.Directories.WORKING) != 0) {
            Toast.makeText(this, "Native side is not initialized.", Toast.LENGTH_LONG).show();
            finish();
        }

        mCameraFragment = new CameraFragment();
        mCameraFragment.PhotoSaved.addHandler(this::onPhotoSaved);

        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_camera, mCameraFragment)
                .commit();
    }

    private void onPhotoSaved(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof PhotoSavedArgs)) return;

        String path = ((PhotoSavedArgs) iArgs).getPath();
        Timber.d("Photo was saved to: " + path);

        Intent intent = new Intent(this, PhotoActivity.class);
        intent.putExtra(PhotoActivity.PATH_INTENT_KEY, path);
        startActivityForResult(intent, Constants.PHOTO_ACTIVITY_REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iIntent) {
        super.onActivityResult(iRequestCode, iResultCode, iIntent);
        if (iRequestCode == Constants.PHOTO_ACTIVITY_REQUEST_CODE) {
            boolean updateGallery = true;

            String path = "";
            if (iIntent.hasExtra(PhotoActivity.PATH_INTENT_KEY)) {
                path = iIntent.getStringExtra(PhotoActivity.PATH_INTENT_KEY);
            }

            switch (iResultCode) {
                case Constants.PHOTO_DELETED_RESULT_CODE:
                    PhotoUtil.deletePhoto(path);
                    updateGallery = false;
                    break;
            }

            if (updateGallery) {
                PhotoUtil.addPhotoToGallery(this, path);
            }

            Intent intent = new Intent(this, CameraActivity.class);
            startActivityForResult(intent, Constants.CAMERA_ACTIVITY_REQUEST_CODE);
        }
    }

    @Override
    public boolean onKeyDown(int iKeyCode, @NonNull KeyEvent iEvent) {
        switch (iKeyCode) {
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
        super.onBackPressed();

        if (mCameraFragment != null && mCameraFragment.isSavingInProgress()) {
            Toast toast = Toast.makeText(getApplicationContext(), "Saving photo is in progress, try again to exit later", Toast.LENGTH_SHORT);
            toast.show();
            return;
        }

        finishAffinity();
        finish();
    }
}
