package com.yalantis.cameramodule.common.image;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import timber.log.Timber;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.support.media.ExifInterface;
import android.os.AsyncTask;
import android.os.Environment;

import com.yalantis.cameramodule.common.Constants;
import com.yalantis.cameramodule.interfaces.PhotoSavedListener;

public class SavingPhotoTask extends AsyncTask<Void, Void, File> {

    private byte[] mData;
    private String mPath;
    private int mOrientation;
    private PhotoSavedListener mCallback;

    public SavingPhotoTask(byte[] data, int orientation, PhotoSavedListener callback) {
        String timeStamp = new SimpleDateFormat(Constants.Time.FORMAT, Locale.getDefault()).format(new Date());

        mData = data;
        mPath = Constants.Directories.OUTPUT + Constants.ImWrite.PREFIX + timeStamp + Constants.ImWrite.POSTFIX;
        mOrientation = orientation;
        mCallback = callback;
    }

    @Override
    protected File doInBackground(Void... params) {
        File photo = getOutputMediaFile();
        if (photo == null) {
            Timber.e("Error creating media file, check storage permissions");
            return null;
        }

        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(photo);
            if (mOrientation == ExifInterface.ORIENTATION_UNDEFINED) {
                saveByteArray(fos, mData);
            } else {
                saveByteArrayWithOrientation(fos, mData, mOrientation);
            }

        } catch (FileNotFoundException e) {
            Timber.e(e, "File not found: " + e.getMessage());
        } catch (IOException e) {
            Timber.e(e, "File write failure: " + e.getMessage());
        } finally {
            try {
                if (fos != null) {
                    fos.close();
                }
            } catch (IOException e) {
                Timber.e(e, e.getMessage());
            }
        }

        return photo;
    }

    private void saveByteArray(FileOutputStream fos, byte[] data) throws IOException {
        fos.write(data);
    }

    private void saveByteArrayWithOrientation(FileOutputStream fos, byte[] data, int orientation) {
        Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);

        if (orientation != 0 && bitmap.getWidth() > bitmap.getHeight()) {
            Matrix matrix = new Matrix();
            matrix.postRotate(orientation);
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
        }
        bitmap.compress(Bitmap.CompressFormat.JPEG, Constants.ImWrite.QUALITY, fos);
        bitmap.recycle();
    }

    @Override
    protected void onPostExecute(File file) {
        super.onPostExecute(file);

        if (file != null && mCallback != null) {
            mCallback.photoSaved(mPath);
        }
    }

    /**
     * Create a File for saving an image
     */
    private File getOutputMediaFile() {
        // To be safe, we should check that the SDCard is mounted
        if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            Timber.e("External storage " + Environment.getExternalStorageState());
            return null;
        }

        // Create the storage directory if it doesn't exist
        File dir = new File(Constants.Directories.OUTPUT);
        if (!dir.exists()) {
            if (!dir.mkdirs()) {
                Timber.e("Failed to create directory");
                return null;
            }
        }

        return new File(mPath);
    }
}
