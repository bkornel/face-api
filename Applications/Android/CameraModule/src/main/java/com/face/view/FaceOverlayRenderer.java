package com.face.view;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

import java.util.Locale;

/**
 * Draws the face overlay onto a Canvas from {@link FaceOverlayData}.
 *
 * Kept apart from the view so the same drawing can be used for a captured still image, where
 * the target is a bitmap-backed Canvas rather than the screen.
 */
public final class FaceOverlayRenderer {

    /** Edges of the face box, matching the corner order the native side sends. */
    private static final int[] BOX_EDGES = {
            0, 1, 1, 3, 3, 2, 2, 0,     // front face
            4, 5, 5, 7, 7, 6, 6, 4,     // rear face
            0, 4, 1, 5, 2, 6, 3, 7      // sides
    };

    private final Paint mLandmarkPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mBoxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mTextBackPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final float[] mLineBuffer = new float[BOX_EDGES.length * 2];
    private final float[] mPointBuffer = new float[68 * 2];

    public FaceOverlayRenderer(float iDensity) {
        final float scale = iDensity > 0.0F ? iDensity : 1.0F;

        mLandmarkPaint.setColor(Color.rgb(120, 255, 120));
        mLandmarkPaint.setStrokeWidth(2.0F * scale);
        mLandmarkPaint.setStrokeCap(Paint.Cap.ROUND);

        mBoxPaint.setColor(Color.rgb(150, 255, 240));
        mBoxPaint.setStyle(Paint.Style.STROKE);
        mBoxPaint.setStrokeWidth(1.5F * scale);

        mTextPaint.setColor(Color.WHITE);
        mTextPaint.setTextSize(12.0F * scale);

        mTextBackPaint.setColor(Color.argb(140, 0, 0, 0));
    }

    /**
     * @param iScaleX horizontal frame-pixels to canvas-pixels factor
     * @param iScaleY vertical frame-pixels to canvas-pixels factor
     */
    public void draw(Canvas iCanvas, FaceOverlayData iData, float iScaleX, float iScaleY) {
        if (iCanvas == null || iData == null) {
            return;
        }

        for (int face = 0; face < iData.getFaceCount(); face++) {
            drawFaceBox(iCanvas, iData, face, iScaleX, iScaleY);
            drawLandmarks(iCanvas, iData, face, iScaleX, iScaleY);
            drawLabel(iCanvas, iData, face, iScaleX, iScaleY);
        }
    }

    private void drawLandmarks(Canvas iCanvas, FaceOverlayData iData, int iFace, float iScaleX, float iScaleY) {
        final int count = Math.min(iData.getLandmarkCount(iFace), mPointBuffer.length / 2);
        if (count <= 0) {
            return;
        }

        for (int i = 0; i < count; i++) {
            mPointBuffer[i * 2] = iData.getLandmarkX(iFace, i) * iScaleX;
            mPointBuffer[(i * 2) + 1] = iData.getLandmarkY(iFace, i) * iScaleY;
        }

        // One drawPoints call instead of 68 drawCircle calls
        iCanvas.drawPoints(mPointBuffer, 0, count * 2, mLandmarkPaint);
    }

    private void drawFaceBox(Canvas iCanvas, FaceOverlayData iData, int iFace, float iScaleX, float iScaleY) {
        final int corners = iData.getBoxPointCount(iFace);
        if (corners < 8) {
            // No pose for this face yet, so there is no box to draw
            return;
        }

        int offset = 0;
        for (int edge = 0; edge < BOX_EDGES.length; edge += 2) {
            final int from = BOX_EDGES[edge];
            final int to = BOX_EDGES[edge + 1];

            if (from >= corners || to >= corners) {
                continue;
            }

            mLineBuffer[offset++] = iData.getBoxX(iFace, from) * iScaleX;
            mLineBuffer[offset++] = iData.getBoxY(iFace, from) * iScaleY;
            mLineBuffer[offset++] = iData.getBoxX(iFace, to) * iScaleX;
            mLineBuffer[offset++] = iData.getBoxY(iFace, to) * iScaleY;
        }

        if (offset > 0) {
            iCanvas.drawLines(mLineBuffer, 0, offset, mBoxPaint);
        }
    }

    private void drawLabel(Canvas iCanvas, FaceOverlayData iData, int iFace, float iScaleX, float iScaleY) {
        final String text = String.format(Locale.US, "User %d   yaw %.0f  pitch %.0f  roll %.0f   %.0f cm",
                iData.getUserId(iFace),
                Math.toDegrees(iData.getYaw(iFace)),
                Math.toDegrees(iData.getPitch(iFace)),
                Math.toDegrees(iData.getRoll(iFace)),
                iData.getPositionZ(iFace) / 10.0F);

        final float x = iData.getRectLeft(iFace) * iScaleX;
        final float y = iData.getRectTop(iFace) * iScaleY;

        final float textWidth = mTextPaint.measureText(text);
        final float textHeight = mTextPaint.getTextSize();
        final float baseline = Math.max(y - (textHeight * 0.4F), textHeight);

        iCanvas.drawRect(x - 2.0F, baseline - textHeight, x + textWidth + 4.0F, baseline + (textHeight * 0.3F), mTextBackPaint);
        iCanvas.drawText(text, x, baseline, mTextPaint);
    }
}
