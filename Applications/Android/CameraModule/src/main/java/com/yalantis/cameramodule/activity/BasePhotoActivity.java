package com.yalantis.cameramodule.activity;

import android.app.Fragment;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;

import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;
import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.common.image.ImageManager;

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
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

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

    protected abstract void showPhoto(Bitmap bitmap);

    protected void rotatePhoto(float angle) {
        synchronized (mBitmap) {
            mBitmap = ImageManager.i.rotatePhoto(mPath, angle);
            showPhoto(mBitmap);
        }
        setResult(EXTRAS.RESULT_EDITED, setIntentData());
    }

    protected void deletePhoto() {
        setResult(EXTRAS.RESULT_DELETED, setIntentData());
        finish();
    }

    protected void loadPhoto() {
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        ImageManager.i.loadPhoto(mPath, metrics.widthPixels, metrics.heightPixels, loadingTarget);
    }

    protected void setFragment(Fragment fragment) {
        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_content, fragment)
                .commit();
    }

    private Target loadingTarget = new Target() {
        @Override
        public void onBitmapLoaded(Bitmap bitmap, Picasso.LoadedFrom from) {
            mProgressBar.setVisibility(View.GONE);
            BasePhotoActivity.this.mBitmap = bitmap;
            showPhoto(bitmap);
        }

        @Override
        public void onBitmapFailed(Drawable errorDrawable) {
            mProgressBar.setVisibility(View.GONE);
            mBitmap = null;
        }

        @Override
        public void onPrepareLoad(Drawable placeHolderDrawable) {
            mProgressBar.setVisibility(View.VISIBLE);
        }
    };

    protected Intent setIntentData() {
        return setIntentData(null);
    }

    protected Intent setIntentData(Intent intent) {
        if (intent == null) {
            intent = new Intent();
        }
        intent.putExtra(EXTRAS.PATH, mPath);
        return intent;
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }
}
