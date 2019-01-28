package com.face.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.view.KeyEvent;
import android.widget.Toast;

import com.face.common.Constants;
import com.face.R;
import com.face.event.EventArgs;
import com.face.event.PhotoSavedArgs;
import com.face.fragment.CameraFragment;
import com.face.common.Assets;
import com.face.common.image.PhotoUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import timber.log.Timber;

public class CameraActivity extends BaseActivity {

    static {
        System.loadLibrary("face_native");
    }

    @SuppressWarnings("JniMissingFunction")
    private native int NativeInitialize(String iPath);

    private CameraFragment mCameraFragment;
    private int mPermissionCode = 1234;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

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

    public void onRequestPermissionsResult(int iRequestCode, @NonNull String iPermissions[], @NonNull int[] iGrantResults) {
        if (iRequestCode == mPermissionCode) {
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

        if (NativeInitialize(Constants.Directories.WORKING) != 0) {
            Toast.makeText(this, "Native side is not initialized.", Toast.LENGTH_LONG).show();
            finish();
        }

        mCameraFragment = new CameraFragment();
        mCameraFragment.PhotoSaved.addHandler(this::onPhotoSaved);

        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_content, mCameraFragment)
                .commit();
    }

    private void onPhotoSaved(Object iSender, EventArgs iArgs) {
        if (!(iArgs instanceof PhotoSavedArgs)) return;

        String path = ((PhotoSavedArgs) iArgs).getPath();
        Timber.d("Photo was saved to: " + path);

        Intent intent = new Intent(this, PhotoActivity.class);
        intent.putExtra(BasePhotoActivity.EXTRAS.PATH, path);
        startActivityForResult(intent, BasePhotoActivity.EXTRAS.REQUEST_PHOTO_EDIT);
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iData) {
        super.onActivityResult(iRequestCode, iResultCode, iData);
        if (iRequestCode == BasePhotoActivity.EXTRAS.REQUEST_PHOTO_EDIT) {
            switch (iResultCode) {
                case BasePhotoActivity.EXTRAS.RESULT_DELETED:
                    String path = iData.getStringExtra(BasePhotoActivity.EXTRAS.PATH);
                    PhotoUtil.deletePhoto(path);
                    break;
            }
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
        if (mCameraFragment == null || !mCameraFragment.isSavingInProgress()) {
            super.onBackPressed();
        }
    }
}
