package com.yalantis.cameramodule.common.image;

import java.io.File;

public class PhotoUtil {

    public static void deletePhoto(String path) {
        File file = new File(path);
        file.delete();
    }
}
