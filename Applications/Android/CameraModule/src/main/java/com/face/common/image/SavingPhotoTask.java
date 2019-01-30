package com.face.common.image;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import timber.log.Timber;

import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.os.Environment;

import com.face.common.Constants;
import com.face.event.Event;
import com.face.event.IEvent;
import com.face.event.PhotoSavedArgs;

public class SavingPhotoTask extends AsyncTask<Void, Void, File> {

    public final IEvent<PhotoSavedArgs> PhotoSaved = new Event<>();

    private Bitmap mBitmap;
    private String mPath;
    private boolean mIsSavingInProgress;

    public SavingPhotoTask(Bitmap iBitmap) {
        String timeStamp = new SimpleDateFormat(Constants.Time.FORMAT, Locale.getDefault()).format(new Date());

        mBitmap = iBitmap;
        mPath = Constants.Directories.GALLERY + Constants.ImWrite.PREFIX + timeStamp + Constants.ImWrite.POSTFIX;
    }

    @Override
    protected File doInBackground(Void... params) {
        mIsSavingInProgress = true;

        if (mBitmap == null) {
            return null;
        }

        File photo = getOutputFile();
        if (photo == null) {
            Timber.e("Error creating media file, check storage permissions");
            return null;
        }

        FileOutputStream stream = null;
        try {
            stream = new FileOutputStream(photo);

            mBitmap.compress(Bitmap.CompressFormat.JPEG, Constants.ImWrite.QUALITY, stream);
            mBitmap.recycle();

        } catch (FileNotFoundException e) {
            Timber.e(e, "File not found: " + e.getMessage());
        } finally {
            try {
                if (stream != null) {
                    stream.close();
                }
            } catch (IOException e) {
                Timber.e(e, e.getMessage());
            }
        }

        return photo;
    }

    @Override
    protected void onPostExecute(File iFile) {
        super.onPostExecute(iFile);
        PhotoSaved.raise(this, iFile != null ? new PhotoSavedArgs(mPath) : null);
        mIsSavingInProgress = false;
    }

    private File getOutputFile() {
        // To be safe, we should check that the SDCard is mounted
        if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            Timber.e("External storage state: " + Environment.getExternalStorageState());
            return null;
        }

        // Create the storage directory if it doesn't exist
        File directory = new File(Constants.Directories.GALLERY);
        if (!directory.exists()) {
            if (!directory.mkdirs()) {
                Timber.e("Failed to create directory: " + directory.getAbsolutePath());
                return null;
            }
        }

        return new File(mPath);
    }

    public boolean isSavingInProgress() { return mIsSavingInProgress; }
}
