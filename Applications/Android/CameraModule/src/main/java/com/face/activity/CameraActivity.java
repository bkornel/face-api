package com.face.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.face.R;
import com.face.common.AppDirectories;
import com.face.common.Assets;
import com.face.common.Constants;
import com.face.common.Native;
import com.face.common.image.PhotoUtil;
import com.face.event.EventArgs;
import com.face.event.PhotoSavedArgs;
import com.face.fragment.CameraFragment;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
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

        // Only CAMERA is needed now: everything the app writes lives in app private
        // storage, so the storage permissions are gone.
        List<String> checkPermissions = Collections.singletonList(Manifest.permission.CAMERA);
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

    @Override
    public void onRequestPermissionsResult(int iRequestCode, @NonNull String[] iPermissions, @NonNull int[] iGrantResults) {
        super.onRequestPermissionsResult(iRequestCode, iPermissions, iGrantResults);

        if (iRequestCode != Constants.PERMISSION_CODE) {
            return;
        }

        // The previous version called finish() inside the loop without returning and
        // then ran onRequestPermissionsGranted() unconditionally, so a denied
        // permission still went on to open the camera.
        for (int i = 0; i < iGrantResults.length; ++i) {
            if (iGrantResults[i] != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Permission denied: " + iPermissions[i], Toast.LENGTH_LONG).show();
                finish();
                return;
            }
        }

        // An empty result array means the request was cancelled.
        if (iGrantResults.length == 0) {
            Timber.w("The permission request was cancelled.");
            finish();
            return;
        }

        onRequestPermissionsGranted();
    }

    private void onRequestPermissionsGranted() {
        final String workingDirectory = AppDirectories.workingPath(this);

        // The native side writes its log and profiler output here and does not
        // create the directory itself.
        File outputDirectory = AppDirectories.output(this);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            Timber.w("Could not create the output directory: %s", outputDirectory.getAbsolutePath());
        }

        if (!Assets.copyAssetFolder(getAssets(), Constants.APP_NAME, workingDirectory)) {
            Toast.makeText(this, "Assets could not be copied.", Toast.LENGTH_LONG).show();
            finish();
            return;
        }

        if (Native.i.initialize(workingDirectory) != 0) {
            Toast.makeText(this, "Native side is not initialized.", Toast.LENGTH_LONG).show();
            finish();
            return;
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

        if (iRequestCode != Constants.PHOTO_ACTIVITY_REQUEST_CODE) {
            return;
        }

        // iIntent is null whenever the child activity finished without setting a
        // result, which used to throw here.
        String path = "";
        if (iIntent != null && iIntent.hasExtra(PhotoActivity.PATH_INTENT_KEY)) {
            path = iIntent.getStringExtra(PhotoActivity.PATH_INTENT_KEY);
        }

        if (iResultCode == Constants.PHOTO_DELETED_RESULT_CODE) {
            PhotoUtil.deletePhoto(path);
        } else {
            PhotoUtil.addPhotoToGallery(this, path);
        }

        // No relaunch of this very activity anymore: it stacked a new CameraActivity
        // on top of the current one every time a photo was reviewed. Returning here
        // simply resumes the camera fragment that is already running.
    }

    @Override
    public boolean onKeyDown(int iKeyCode, @NonNull KeyEvent iEvent) {
        // mCameraFragment stays null when the permission was denied.
        if (iKeyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            return true;
        }

        if (mCameraFragment == null) {
            return false;
        }

        switch (iKeyCode) {
            case KeyEvent.KEYCODE_VOLUME_UP:
                mCameraFragment.onZoomInPressed();
                return true;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                mCameraFragment.onZoomOutPressed();
                return true;
            case KeyEvent.KEYCODE_CAMERA:
                mCameraFragment.onCapturePressed();
                return true;
        }
        return false;
    }

    @Override
    public void onBackPressed() {
        // The guard has to run before super.onBackPressed(), which already finishes
        // the activity. Previously the check could never prevent anything.
        if (mCameraFragment != null && mCameraFragment.isSavingInProgress()) {
            Toast toast = Toast.makeText(getApplicationContext(), "Saving photo is in progress, try again to exit later", Toast.LENGTH_SHORT);
            toast.show();
            return;
        }

        super.onBackPressed();

        finishAffinity();
        finish();
    }
}
