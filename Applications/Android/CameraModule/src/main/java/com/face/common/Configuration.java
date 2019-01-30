package com.face.common;

import android.content.Context;
import android.content.SharedPreferences;

import com.face.common.camera.FlashMode;
import com.face.common.camera.FocusMode;
import com.face.common.camera.PictureSize;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public enum Configuration {
    i;

    private static final String NAME = "sharedPrefs";

    private static final String USE_FRONT_CAMERA = "use_front_camera";
    private static final String PREVIEW_SIZE_ID = "preview_size_id";
    private static final String PREVIEW_SIZE_LIST = "preview_size_list";
    private static final String FLASH_SUPPORTED = "flash_supported";
    private static final String FLASH_MODE = "flash_mode";
    private static final String FOCUS_MODE = "focus_mode";
    private static final String FOCUS_ON_TOUCH_SUPPORTED = "focus_on_touch_supported";

    private static SharedPreferences sSharedPreferences;

    private Set<CachedValue> mCachedValues;

    private CachedValue<Integer> mPreviewSizeId;
    private CachedValue<String> mPreviewSizeList;
    private CachedValue<Boolean> mFlashSupported;
    private CachedValue<Integer> mFlashMode;
    private CachedValue<Integer> mFocusMode;
    private CachedValue<Boolean> mFocusOnTouchSupported;
    private CachedValue<Boolean> mUseFrontCamera;

    public void init(Context iContext) {
        sSharedPreferences = iContext.getSharedPreferences(NAME, Context.MODE_PRIVATE);
        CachedValue.initialize(sSharedPreferences);

        mCachedValues = new HashSet<>();
        mCachedValues.add(mPreviewSizeId = new CachedValue<>(PREVIEW_SIZE_ID, 0, Integer.class));
        mCachedValues.add(mPreviewSizeList = new CachedValue<>(PREVIEW_SIZE_LIST, "", String.class));
        mCachedValues.add(mFlashSupported = new CachedValue<>(FLASH_SUPPORTED, false, Boolean.class));
        mCachedValues.add(mFlashMode = new CachedValue<>(FLASH_MODE, FlashMode.AUTO.getId(), Integer.class));
        mCachedValues.add(mFocusMode = new CachedValue<>(FOCUS_MODE, FocusMode.AUTO.getId(), Integer.class));
        mCachedValues.add(mFocusOnTouchSupported = new CachedValue<>(FOCUS_ON_TOUCH_SUPPORTED, false, Boolean.class));
        mCachedValues.add(mUseFrontCamera = new CachedValue<>(USE_FRONT_CAMERA, true, Boolean.class));
    }

    public PictureSize getPreviewSize() {
        int id = getPreviewSizeId();
        ArrayList<PictureSize> list = getPreviewSizeList();
        return (id >= 0 && id < list.size() ? list.get(id) : null);
    }

    public int getPreviewSizeId() {
        return mPreviewSizeId.getValue();
    }

    public void setPreviewSizeId(int iPreviewSizeId) {
        mPreviewSizeId.setValue(iPreviewSizeId);
    }

    public ArrayList<PictureSize> getPreviewSizeList() {
        ArrayList<String> previewSizeListStr = new ArrayList<>();

        try {
            JSONArray jsonArray = new JSONArray(mPreviewSizeList.getValue());
            for (int i = 0; i < jsonArray.length(); i++) {
                previewSizeListStr.add(jsonArray.getString(i));
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return PictureSize.toPictureSizeArrayList(previewSizeListStr);
    }

    public void setPreviewSizeList(ArrayList<PictureSize> iPreviewSizeList) {
        mPreviewSizeList.clear();
        JSONArray jsonArray = new JSONArray();
        for (PictureSize ps : iPreviewSizeList) {
            jsonArray.put(ps.toString());
        }
        mPreviewSizeList.setValue(jsonArray.toString());
    }

    public boolean getFlashSupported() {
        return mFlashSupported.getValue();
    }

    public void setFlashSupported(boolean iFlashModeEnabled) {
        mFlashSupported.setValue(iFlashModeEnabled);
        if (!iFlashModeEnabled) {
            setFlashMode(FlashMode.OFF);
        }
    }

    public FlashMode getFlashMode() {
        return FlashMode.getFlashModeById(mFlashMode.getValue());
    }

    public void setFlashMode(FlashMode iFlashMode) {
        mFlashMode.setValue(iFlashMode.getId());
    }

    public FocusMode getFocusMode() {
        return FocusMode.getFocusModeById(mFocusMode.getValue());
    }

    public void setFocusMode(FocusMode iFocusMode) {
        mFocusMode.setValue(iFocusMode.getId());
    }

    public boolean getFocusOnTouchSupported() {
        return mFocusOnTouchSupported.getValue();
    }

    public void setFocusOnTouchSupported(boolean iFocusOnTouchSupported) {
        mFocusOnTouchSupported.setValue(iFocusOnTouchSupported);
        if (!iFocusOnTouchSupported) {
            setFocusMode(FocusMode.AUTO);
        }
    }

    public boolean useFrontCamera() {
        return mUseFrontCamera.getValue();
    }

    public void setUseFrontCamera(boolean iUseFrontCamera) {
        mUseFrontCamera.setValue(iUseFrontCamera);
    }

    public void clear() {
        for (CachedValue value : mCachedValues) {
            value.clear();
        }
        sSharedPreferences.edit().clear().commit();
    }
}
