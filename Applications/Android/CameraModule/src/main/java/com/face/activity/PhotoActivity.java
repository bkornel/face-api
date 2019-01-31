package com.face.activity;

import android.app.Fragment;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;

import com.face.R;
import com.face.common.Constants;
import com.face.common.image.ImageManager;
import com.face.fragment.PhotoFragment;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;

public class PhotoActivity extends BaseActivity {

    static final String PATH_INTENT_KEY = "";

    private PhotoFragment mPhotoFragment;
    private String mPath;
    private Bitmap mBitmap;
    private View mProgressBar;

    private Target mLoadingTarget = new Target() {
        @Override
        public void onBitmapLoaded(Bitmap iBitmap, Picasso.LoadedFrom iLoadedFrom) {
            mProgressBar.setVisibility(View.GONE);
            mBitmap = iBitmap;
            showPhoto(iBitmap);
        }

        @Override
        public void onBitmapFailed(Drawable iDrawable) {
            mProgressBar.setVisibility(View.GONE);
            mBitmap = null;
        }

        @Override
        public void onPrepareLoad(Drawable iDrawable) {
            mProgressBar.setVisibility(View.VISIBLE);
        }
    };

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        showActionBar();
        setContentView(R.layout.activity_photo);

        if (getIntent().hasExtra(PATH_INTENT_KEY)) {
            mPath = getIntent().getStringExtra(PATH_INTENT_KEY);
        } else {
            throw new RuntimeException("Path is not defined");
        }

        mProgressBar = findViewById(R.id.progress);
        setResults(Constants.PHOTO_GENERAL_RESULT_CODE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mBitmap == null || mBitmap.isRecycled()) {
            loadPhoto();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu iMenu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.photo_preview_options, iMenu);
        return super.onCreateOptionsMenu(iMenu);
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iIntent) {
        super.onActivityResult(iRequestCode, iResultCode, iIntent);
        if ((iRequestCode == Constants.PHOTO_ACTIVITY_REQUEST_CODE) && (iResultCode == Constants.PHOTO_EDITED_RESULT_CODE)) {
            setResults(Constants.PHOTO_EDITED_RESULT_CODE);
            loadPhoto();
        }
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        hideActionBar();
        finish();
    }

    public void rotateLeft(MenuItem iMenuItem) {
        rotate(-90);
    }

    public void rotateRight(MenuItem iMenuItem) {
        rotate(90);
    }

    private void rotate(float iAngle) {
        synchronized (mBitmap) {
            mBitmap = ImageManager.i.rotate(mPath, iAngle);
            showPhoto(mBitmap);
        }
        setResults(Constants.PHOTO_EDITED_RESULT_CODE);
    }

    public void deletePhoto(MenuItem iMenuItem) {
        setResults(Constants.PHOTO_DELETED_RESULT_CODE);
        finish();
    }

    public void loadPhoto() {
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        ImageManager.i.load(mPath, metrics.widthPixels, metrics.heightPixels, mLoadingTarget);
    }

    protected void showPhoto(Bitmap iBitmap) {
        if (mPhotoFragment == null) {
            mPhotoFragment = PhotoFragment.newInstance(iBitmap);
            setFragment(mPhotoFragment);
        } else {
            mPhotoFragment.setBitmap(iBitmap);
        }
    }

    protected void setFragment(Fragment iFragment) {
        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_photo, iFragment)
                .commit();
    }

    private void setResults(int iResultCode) {
        Intent intent = new Intent();
        intent.putExtra(PATH_INTENT_KEY, mPath);
        setResult(iResultCode, intent);
    }
}
