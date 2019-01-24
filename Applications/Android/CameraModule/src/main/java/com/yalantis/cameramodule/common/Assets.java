package com.yalantis.cameramodule.common;

import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class Assets {
    public static boolean copyAssetFolder(AssetManager assetManager, String source, String destination) {
        try {
            String[] files = assetManager.list(source);
            new File(destination).mkdirs();

            for (String file : files) {
                String sourceFile = source + File.separator + file;
                String destinationFile = destination + File.separator + file;

                if (file.contains(".")) {
                    copyAsset(assetManager, sourceFile, destinationFile);
                }
                else {
                    copyAssetFolder(assetManager, sourceFile, destinationFile);
                }
            }
            return true;
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return false;
    }

    private static void copyAsset(AssetManager assetManager, String source, String destination) throws IOException {
        InputStream in = assetManager.open(source);
        new File(destination).createNewFile();

        OutputStream out = new FileOutputStream(destination);
        copyFile(in, out);

        in.close();
        out.flush();
        out.close();
    }

    private static void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while((read = in.read(buffer)) != -1){
            out.write(buffer, 0, read);
        }
    }
}
