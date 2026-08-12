package com.face.common;

import android.content.Context;
import android.os.Environment;

import java.io.File;

/**
 * Every directory the app writes to.
 * <p>
 * All of them live in app private storage, which needs no runtime permission and
 * keeps working under scoped storage. The previous version built these paths from
 * {@code Environment.getExternalStorageDirectory()}, which is deprecated and no
 * longer writable from API 29 on, so nothing could be stored at all there.
 * <p>
 * These are methods rather than static constants on purpose: the paths depend on
 * a Context and must not be resolved while the class is being loaded.
 */
public final class AppDirectories {

    private AppDirectories() {
    }

    /** Working directory handed to the native side: settings, cascades, shape models. */
    public static File working(Context iContext) {
        return new File(iContext.getFilesDir(), Constants.APP_NAME);
    }

    /** Native output directory: logs and profiler dumps. */
    public static File output(Context iContext) {
        return new File(working(iContext), "output");
    }

    /** Where captured photos are written before they are published to the gallery. */
    public static File pictures(Context iContext) {
        // Can be null when external storage is not available right now.
        File directory = iContext.getExternalFilesDir(Environment.DIRECTORY_PICTURES);
        return directory != null ? directory : new File(iContext.getFilesDir(), "pictures");
    }

    /**
     * Working directory as a path with a trailing separator, which is the form the
     * native side expects.
     */
    public static String workingPath(Context iContext) {
        String path = working(iContext).getAbsolutePath();
        return path.endsWith(File.separator) ? path : path + File.separator;
    }
}
