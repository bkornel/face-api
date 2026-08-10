package com.face.common;

import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import timber.log.Timber;

public class Assets {

    /**
     * Copies an asset folder recursively to iDestination.
     * <p>
     * A file named ".copied" is written next to the copied tree, so the work is
     * only done once per install instead of on every launch. Pass iForce to copy
     * again regardless.
     */
    public static boolean copyAssetFolder(AssetManager iAssetManager, String iSource, String iDestination) {
        return copyAssetFolder(iAssetManager, iSource, iDestination, false);
    }

    public static boolean copyAssetFolder(AssetManager iAssetManager, String iSource, String iDestination, boolean iForce) {
        File marker = new File(iDestination, ".copied");

        if (!iForce && marker.exists()) {
            Timber.d("Assets are already in place at %s", iDestination);
            return true;
        }

        if (!copyFolder(iAssetManager, iSource, iDestination)) {
            return false;
        }

        try {
            if (!marker.createNewFile() && !marker.exists()) {
                Timber.w("Could not write the marker file, assets will be copied again next time.");
            }
        } catch (IOException e) {
            Timber.w(e, "Could not write the marker file, assets will be copied again next time.");
        }

        return true;
    }

    private static boolean copyFolder(AssetManager iAssetManager, String iSource, String iDestination) {
        try {
            String[] entries = iAssetManager.list(iSource);

            // list() returns null on error and an empty array for a plain file.
            if (entries == null) {
                Timber.e("Could not list the asset folder: %s", iSource);
                return false;
            }

            File destination = new File(iDestination);
            if (!destination.exists() && !destination.mkdirs()) {
                Timber.e("Could not create the directory: %s", iDestination);
                return false;
            }

            for (String entry : entries) {
                String sourceEntry = iSource + File.separator + entry;
                String destinationEntry = iDestination + File.separator + entry;

                // Ask the AssetManager instead of guessing from a dot in the name.
                // The old check treated "face3d.obj" as a file only by luck and any
                // extension-less file as a folder.
                if (isAssetFolder(iAssetManager, sourceEntry)) {
                    if (!copyFolder(iAssetManager, sourceEntry, destinationEntry)) {
                        return false;
                    }
                } else {
                    copyAsset(iAssetManager, sourceEntry, destinationEntry);
                }
            }

            return true;
        } catch (IOException e) {
            Timber.e(e, "Could not copy the asset folder: %s", iSource);
        }

        return false;
    }

    /**
     * AssetManager.list() returns an empty array both for a plain file and for an
     * empty folder, and only a file can be opened. The "output" asset folder is
     * empty in the APK, so the distinction matters.
     */
    private static boolean isAssetFolder(AssetManager iAssetManager, String iPath) {
        try {
            String[] children = iAssetManager.list(iPath);
            if (children != null && children.length > 0) {
                return true;
            }
        } catch (IOException e) {
            // Fall through to the open() probe below.
        }

        try (InputStream probe = iAssetManager.open(iPath)) {
            return false;
        } catch (IOException e) {
            return true;
        }
    }

    private static void copyAsset(AssetManager iAssetManager, String iSource, String iDestination) throws IOException {
        // try-with-resources: both streams leaked whenever the copy threw.
        try (InputStream in = iAssetManager.open(iSource);
             OutputStream out = new FileOutputStream(iDestination)) {
            copyFile(in, out);
        }
    }

    private static void copyFile(InputStream iInputStream, OutputStream iOutputStream) throws IOException {
        byte[] buffer = new byte[8192];
        int read;
        while ((read = iInputStream.read(buffer)) != -1) {
            iOutputStream.write(buffer, 0, read);
        }
    }
}
