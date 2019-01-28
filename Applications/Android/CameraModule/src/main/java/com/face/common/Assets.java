package com.face.common;

import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class Assets {
    public static boolean copyAssetFolder(AssetManager iAssetManager, String iSource, String iDestination) {
        try {
            String[] files = iAssetManager.list(iSource);
            new File(iDestination).mkdirs();

            for (String file : files) {
                String sourceFile = iSource + File.separator + file;
                String destinationFile = iDestination + File.separator + file;

                if (file.contains(".")) {
                    copyAsset(iAssetManager, sourceFile, destinationFile);
                } else {
                    copyAssetFolder(iAssetManager, sourceFile, destinationFile);
                }
            }
            return true;
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return false;
    }

    private static void copyAsset(AssetManager iAssetManager, String iSource, String iDestination) throws IOException {
        InputStream in = iAssetManager.open(iSource);
        new File(iDestination).createNewFile();

        OutputStream out = new FileOutputStream(iDestination);
        copyFile(in, out);

        in.close();
        out.flush();
        out.close();
    }

    private static void copyFile(InputStream iInputStream, OutputStream iOutputStream) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = iInputStream.read(buffer)) != -1) {
            iOutputStream.write(buffer, 0, read);
        }
    }
}
