package com.face.common.image;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;

import com.face.common.AppDirectories;
import com.face.common.Constants;
import com.face.event.Event;
import com.face.event.IEvent;
import com.face.event.PhotoSavedArgs;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

import timber.log.Timber;

/**
 * Writes a bitmap to app private picture storage on a background thread and
 * raises PhotoSaved on the main thread.
 * <p>
 * This used to be an AsyncTask writing to the public DCIM folder. AsyncTask is
 * deprecated, and the public folder is not writable under scoped storage.
 */
public class SavePhotoTask {

    private static final Executor sExecutor = Executors.newSingleThreadExecutor();

    public final IEvent<PhotoSavedArgs> PhotoSaved = new Event<>();

    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    private final AtomicBoolean mIsSavingInProgress = new AtomicBoolean(false);

    private final Bitmap mBitmap;
    private final File mFile;

    public SavePhotoTask(Context iContext, Bitmap iBitmap) {
        String timeStamp = new SimpleDateFormat(Constants.Time.FORMAT, Locale.getDefault()).format(new Date());

        mBitmap = iBitmap;
        mFile = new File(AppDirectories.pictures(iContext),
                Constants.ImWrite.PREFIX + timeStamp + Constants.ImWrite.POSTFIX);
    }

    public void execute() {
        mIsSavingInProgress.set(true);
        sExecutor.execute(this::save);
    }

    public boolean isSavingInProgress() {
        return mIsSavingInProgress.get();
    }

    private void save() {
        final File saved = write();

        mMainHandler.post(() -> {
            mIsSavingInProgress.set(false);
            PhotoSaved.raise(this, saved != null ? new PhotoSavedArgs(saved.getAbsolutePath()) : null);
        });
    }

    private File write() {
        if (mBitmap == null) {
            Timber.e("There is no bitmap to save.");
            return null;
        }

        File directory = mFile.getParentFile();
        if (directory != null && !directory.exists() && !directory.mkdirs()) {
            Timber.e("Failed to create directory: %s", directory.getAbsolutePath());
            return null;
        }

        // try-with-resources: the stream was leaked on every failure path before.
        try (OutputStream stream = new FileOutputStream(mFile)) {
            if (!mBitmap.compress(Constants.ImWrite.FORMAT, Constants.ImWrite.QUALITY, stream)) {
                Timber.e("Failed to compress the bitmap.");
                return null;
            }
        } catch (IOException e) {
            Timber.e(e, "Could not write %s", mFile.getAbsolutePath());
            return null;
        } finally {
            mBitmap.recycle();
        }

        return mFile;
    }
}
