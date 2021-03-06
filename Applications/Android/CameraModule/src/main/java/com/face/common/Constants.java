package com.face.common;

import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.os.Environment;

import java.io.File;

public final class Constants {
    public static final String APP_NAME = "FaceApp";

    public static final int CAMERA_ACTIVITY_REQUEST_CODE = 1111;
    public static final int PHOTO_ACTIVITY_REQUEST_CODE = 2222;
    public static final int PHOTO_GENERAL_RESULT_CODE = 3333;
    public static final int PHOTO_EDITED_RESULT_CODE = 4444;
    public static final int PHOTO_DELETED_RESULT_CODE = 5555;

    public static final int PERMISSION_CODE = 1234;

    public static final class Time {
        public static final String FORMAT = "yyyyMMdd_HHmmss";
    }

    public static final class ImWrite {
        public static final CompressFormat FORMAT = Bitmap.CompressFormat.JPEG;
        public static final int QUALITY = 90;
        public static final String PREFIX = "IMG_";
        public static final String POSTFIX = ".jpg";
    }

    public static final class Directories {
        public static final String EXTERNAL = Environment.getExternalStorageDirectory().getPath() + File.separator;
        public static final String WORKING = EXTERNAL + APP_NAME + File.separator;
        public static final String OUTPUT = WORKING + "output" + File.separator;
        public static final String GALLERY = EXTERNAL + Environment.DIRECTORY_DCIM + File.separator + APP_NAME + File.separator;
    }
}
