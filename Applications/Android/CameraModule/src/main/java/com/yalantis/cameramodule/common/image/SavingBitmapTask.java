package com.yalantis.cameramodule.common.image;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import timber.log.Timber;
import android.graphics.Bitmap;
import android.os.AsyncTask;

import com.yalantis.cameramodule.common.Constants;
import com.yalantis.cameramodule.interfaces.PhotoSavedListener;

public class SavingBitmapTask extends AsyncTask<Void, Void, Void> {

    private Bitmap bitmap;
    private String path;
    private PhotoSavedListener callback;

    public SavingBitmapTask(Bitmap bitmap, String path, PhotoSavedListener callback) {
        this.bitmap = bitmap;
        this.path = path;
        this.callback = callback;
    }

    @Override
    protected Void doInBackground(Void... params) {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(new File(path));

            if (bitmap != null && !bitmap.isRecycled()) {
                bitmap.compress(Constants.ImWrite.FORMAT, Constants.ImWrite.QUALITY, fos);
            }

        } catch (FileNotFoundException e) {
            Timber.e(e, "File not found: " + e.getMessage());
        } finally {
            try {
                if (fos != null) {
                    fos.close();
                }
            } catch (IOException e) {
                Timber.e(e, e.getMessage());
            }
        }

        return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
        super.onPostExecute(aVoid);
        if (callback != null) {
            callback.photoSaved(path);
        }
    }
}
