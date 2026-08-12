package com.face.view;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.View;

/**
 * Transparent view that draws the face overlay on top of the live camera preview.
 *
 * The native side no longer composites the overlay into the frame, so nothing but a couple of
 * kilobytes of geometry crosses the JNI boundary and the preview stays on its own surface.
 */
public class FaceOverlayView extends View {

    private final FaceOverlayRenderer mRenderer;

    private FaceOverlayData mData;
    private int mFrameWidth;
    private int mFrameHeight;

    public FaceOverlayView(Context iContext) {
        this(iContext, null);
    }

    public FaceOverlayView(Context iContext, AttributeSet iAttributeSet) {
        super(iContext, iAttributeSet);

        setWillNotDraw(false);

        final float density = getResources() != null ? getResources().getDisplayMetrics().density : 1.0F;
        mRenderer = new FaceOverlayRenderer(density);
    }

    /** Size of the frame the coordinates are expressed in, i.e. the rotated camera frame. */
    public void setFrameSize(int iWidth, int iHeight) {
        mFrameWidth = iWidth;
        mFrameHeight = iHeight;
    }

    /**
     * Hands over the data to draw. Called from the main thread by the processing task, the
     * data is owned by that task and is not read anywhere else.
     */
    public void setData(FaceOverlayData iData) {
        mData = iData;
        invalidate();
    }

    public FaceOverlayRenderer getRenderer() {
        return mRenderer;
    }

    @Override
    protected void onDraw(Canvas iCanvas) {
        super.onDraw(iCanvas);

        if (mData == null || mData.getFaceCount() <= 0) {
            return;
        }

        if (mFrameWidth <= 0 || mFrameHeight <= 0 || getWidth() <= 0 || getHeight() <= 0) {
            return;
        }

        final float scaleX = (float) getWidth() / (float) mFrameWidth;
        final float scaleY = (float) getHeight() / (float) mFrameHeight;

        mRenderer.draw(iCanvas, mData, scaleX, scaleY);
    }
}
