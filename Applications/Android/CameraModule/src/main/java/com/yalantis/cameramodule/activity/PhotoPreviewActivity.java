package com.yalantis.cameramodule.activity;

import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.fragment.PhotoPreviewFragment;

public class PhotoPreviewActivity extends BasePhotoActivity {

    private PhotoPreviewFragment previewFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.photo_preview_options, menu);
        return super.onCreateOptionsMenu(menu);
    }

    public void deletePhoto(MenuItem item) {
        deletePhoto();
    }

    public void rotateLeft(MenuItem item) {
        rotatePhoto(-90);
    }

    public void rotateRight(MenuItem item) {
        rotatePhoto(90);
    }

    @Override
    protected void showPhoto(Bitmap bitmap) {
        if (previewFragment == null) {
            previewFragment = PhotoPreviewFragment.newInstance(bitmap);
            setFragment(previewFragment);
        } else {
            previewFragment.setBitmap(bitmap);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == EXTRAS.REQUEST_PHOTO_EDIT) {
            if (resultCode == EXTRAS.RESULT_EDITED) {
                setResult(EXTRAS.RESULT_EDITED, setIntentData());
                loadPhoto();
            }
        }
    }
}
