package com.face.activity;

import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import com.face.R;
import com.face.fragment.PhotoFragment;

public class PhotoActivity extends BasePhotoActivity {

    private PhotoFragment mPhotoFragment;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu iMenu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.photo_preview_options, iMenu);
        return super.onCreateOptionsMenu(iMenu);
    }

    public void deletePhoto(MenuItem iMenuItem) {
        deletePhoto();
    }

    public void rotateLeft(MenuItem iMenuItem) {
        rotatePhoto(-90);
    }

    public void rotateRight(MenuItem iMenuItem) {
        rotatePhoto(90);
    }

    @Override
    protected void showPhoto(Bitmap iBitmap) {
        if (mPhotoFragment == null) {
            mPhotoFragment = PhotoFragment.newInstance(iBitmap);
            setFragment(mPhotoFragment);
        } else {
            mPhotoFragment.setBitmap(iBitmap);
        }
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iIntent) {
        super.onActivityResult(iRequestCode, iResultCode, iIntent);
        if (iRequestCode == EXTRAS.REQUEST_PHOTO_EDIT) {
            if (iResultCode == EXTRAS.RESULT_EDITED) {
                Intent intent = new Intent();
                intent.putExtra(EXTRAS.PATH, mPath);
                setResult(EXTRAS.RESULT_EDITED, intent);
                loadPhoto();
            }
        }
    }
}
