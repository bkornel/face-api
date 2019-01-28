package com.face.common.image;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.os.AsyncTask;

import com.face.common.Constants;
import com.face.event.Event;
import com.face.event.IEvent;
import com.face.event.PhotoSavedArgs;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import timber.log.Timber;

public class RotatePhotoTask extends AsyncTask<Void, Void, File> {

    public final IEvent<PhotoSavedArgs> PhotoSaved = new Event<>();

    private String mPath;
    private float mAngle;

    public RotatePhotoTask(String iPath, float iAngle) {
        mPath = iPath;
        mAngle = iAngle;
    }

    @Override
    protected File doInBackground(Void... iParams) {
        File image = new File(mPath);
        if (!image.exists()) {
            return null;
        }

        FileOutputStream stream = null;

        try {
            Bitmap bitmap = BitmapFactory.decodeFile(mPath);
            if (bitmap == null) {
                return null;
            }

            Matrix matrix = new Matrix();
            matrix.postRotate(mAngle);
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);

            stream = new FileOutputStream(new File(mPath));
            bitmap.compress(Bitmap.CompressFormat.JPEG, Constants.ImWrite.QUALITY, stream);
            bitmap.recycle();

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

        return image;
    }

    @Override
    protected void onPostExecute(File iFile) {
        super.onPostExecute(iFile);
        if (iFile != null) {
            PhotoSaved.raise(this, new PhotoSavedArgs(mPath));
        }
    }
}
