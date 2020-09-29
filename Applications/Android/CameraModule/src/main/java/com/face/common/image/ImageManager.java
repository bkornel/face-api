package com.face.common.image;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.text.TextUtils;

import com.squareup.picasso.MemoryPolicy;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.PicassoTools;
import com.squareup.picasso.Target;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;

public enum ImageManager implements ImageTargetCallback {
    i;

    private Context mContext;
    private Picasso mPicasso;
    private HashSet<ImageTarget> mTargets;
    private Map<String, WeakReference<Bitmap>> mBitmaps;

    public void init(Context iContext) {
        mContext = iContext;
        mPicasso = Picasso.get();
        mBitmaps = new HashMap<>();
        mTargets = new HashSet<>();
    }

    public void load(String iPath, int iWidth, int iHeight, Target iTarget) {
        if (iPath == null) {
            iTarget.onBitmapFailed(null,null);
            return;
        }

        Bitmap bitmap = getBitmap(iPath);

        if (bitmap != null && !bitmap.isRecycled()) {
            iTarget.onBitmapLoaded(bitmap, Picasso.LoadedFrom.MEMORY);
        } else {
            File file = !TextUtils.isEmpty(iPath) ? new File(iPath) : null;
            ImageTarget imageTarget = new ImageTarget(iTarget, iPath, this);

            assert file != null;
            Picasso.get()
                    .load(file)
                    .memoryPolicy(MemoryPolicy.NO_CACHE)
                    .config(Bitmap.Config.ARGB_8888)
                    .transform(new ScaleTransformation(iWidth, iHeight))
                    .into(imageTarget);
        }
    }

    public Bitmap rotate(String iPath, float iAngle) {
        Bitmap bitmap = getBitmap(iPath);

        if (bitmap != null && !bitmap.isRecycled()) {
            Matrix matrix = new Matrix();
            matrix.postRotate(iAngle);
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
        }

        setBitmap(iPath, bitmap);

        return bitmap;
    }

    private Bitmap getBitmap(String iPath) {
        return iPath != null && mBitmaps.get(iPath) != null ? Objects.requireNonNull(mBitmaps.get(iPath)).get() : null;
    }

    public void clear() {
        synchronized (mBitmaps) {
            for (WeakReference<Bitmap> reference : mBitmaps.values()) {
                if (reference != null) {
                    Bitmap bitmap = reference.get();
                    if (bitmap != null && !bitmap.isRecycled()) {
                        bitmap.recycle();
                    }
                }
            }
            mBitmaps.clear();
        }

        PicassoTools.clearCache(mPicasso);
    }

    @Override
    public void setBitmap(String iPath, Bitmap iBitmap) {
        mBitmaps.put(iPath, new WeakReference<>(iBitmap));
    }

    @Override
    public void addTarget(ImageTarget iTarget) {
        removeTarget(iTarget);
        mTargets.add(iTarget);
    }

    @Override
    public void removeTarget(ImageTarget iTarget) {
        mTargets.remove(iTarget);
    }
}
