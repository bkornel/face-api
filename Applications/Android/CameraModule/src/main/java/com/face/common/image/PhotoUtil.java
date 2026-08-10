package com.face.common.image;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;

import com.face.common.Constants;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import timber.log.Timber;

public class PhotoUtil {

    public static void deletePhoto(String iPath) {
        if (iPath == null || iPath.isEmpty()) {
            return;
        }

        File file = new File(iPath);
        if (file.exists() && !file.delete()) {
            Timber.w("Could not delete %s", iPath);
        }
    }

    /**
     * Publishes a photo from app private storage into the shared image collection.
     * <p>
     * The previous version broadcast ACTION_MEDIA_SCANNER_SCAN_FILE with a file://
     * URI. That intent is deprecated, and a file:// URI pointing into app private
     * storage is not readable by the media scanner, so nothing was ever added.
     */
    public static void addPhotoToGallery(Context iContext, String iPath) {
        if (iContext == null || iPath == null || iPath.isEmpty()) {
            return;
        }

        File source = new File(iPath);
        if (!source.exists()) {
            Timber.w("There is no file to publish: %s", iPath);
            return;
        }

        ContentValues values = new ContentValues();
        values.put(MediaStore.Images.Media.DISPLAY_NAME, source.getName());
        values.put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            values.put(MediaStore.Images.Media.RELATIVE_PATH,
                    Environment.DIRECTORY_PICTURES + File.separator + Constants.APP_NAME);
        }

        ContentResolver resolver = iContext.getContentResolver();
        Uri target = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);

        if (target == null) {
            Timber.e("Could not create a MediaStore entry for %s", iPath);
            return;
        }

        try (InputStream in = new FileInputStream(source);
             OutputStream out = resolver.openOutputStream(target)) {

            if (out == null) {
                Timber.e("Could not open the MediaStore entry for writing.");
                return;
            }

            byte[] buffer = new byte[8192];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            Timber.e(e, "Could not publish %s to the gallery", iPath);
            resolver.delete(target, null, null);
        }
    }
}
