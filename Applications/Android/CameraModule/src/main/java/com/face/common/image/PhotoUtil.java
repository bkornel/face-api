package com.face.common.image;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import java.io.File;

public class PhotoUtil {

    public static void deletePhoto(String iPath) {
        if (iPath == null || iPath.isEmpty()) {
            return;
        }

        File file = new File(iPath);
        if (file.exists()) {
            file.delete();
        }
    }

    public static void addPhotoToGallery(Activity iActivity, String iPath) {
        if (iPath == null || iPath.isEmpty()) {
            return;
        }

        File file = new File(iPath);
        if (file.exists()) {
            Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
            intent.setData(Uri.fromFile(file));

            iActivity.sendBroadcast(intent);
        }
    }
}
