package com.face.activity;

import android.app.Fragment;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;

import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;
import com.face.R;
import com.face.common.image.ImageManager;

public abstract class BasePhotoActivity extends BaseActivity {

    protected String mPath;
    protected Bitmap mBitmap;
    protected View mProgressBar;

    public static final class EXTRAS {
        public static final String PATH = "full_path";
        public static final int REQUEST_PHOTO_EDIT = 7338;
        public static final int RESULT_EDITED = 338;
        public static final int RESULT_DELETED = 3583;
    }

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        showActionBar();
        showBack();
        setContentView(R.layout.activity_photo);

        if (getIntent().hasExtra(EXTRAS.PATH)) {
            mPath = getIntent().getStringExtra(EXTRAS.PATH);
        } else {
            throw new RuntimeException("There is no path to image in extras");
        }

        mProgressBar = findViewById(R.id.progress);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mBitmap == null || mBitmap.isRecycled()) {
            loadPhoto();
        }
    }

    protected abstract void showPhoto(Bitmap iBitmap);

    protected void rotatePhoto(float iAngle) {
        synchronized (mBitmap) {
            mBitmap = ImageManager.i.rotatePhoto(mPath, iAngle);
            showPhoto(mBitmap);
        }
        Intent intent = new Intent();
        intent.putExtra(EXTRAS.PATH, mPath);
        setResult(EXTRAS.RESULT_EDITED, intent);
    }

    protected void deletePhoto() {
        Intent intent = new Intent();
        intent.putExtra(EXTRAS.PATH, mPath);
        setResult(EXTRAS.RESULT_DELETED, intent);
        finish();
    }

    protected void loadPhoto() {
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        ImageManager.i.loadPhoto(mPath, metrics.widthPixels, metrics.heightPixels, mLoadingTarget);
    }

    protected void setFragment(Fragment iFragment) {
        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_content, iFragment)
                .commit();
    }

    private Target mLoadingTarget = new Target() {
        @Override
        public void onBitmapLoaded(Bitmap iBitmap, Picasso.LoadedFrom iLoadedFrom) {
            mProgressBar.setVisibility(View.GONE);
            BasePhotoActivity.this.mBitmap = iBitmap;
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
    public void onBackPressed() {
        super.onBackPressed();
    }
}
