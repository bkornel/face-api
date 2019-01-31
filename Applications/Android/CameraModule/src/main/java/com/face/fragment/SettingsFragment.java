package com.face.fragment;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Spinner;

import com.face.R;
import com.face.common.Configuration;
import com.face.common.adapter.ObjectToStringAdapter;
import com.face.common.camera.FlashMode;
import com.face.common.camera.FocusMode;
import com.face.common.camera.PictureSize;
import com.face.event.Event;
import com.face.event.FlashModeArgs;
import com.face.event.FocusModeArgs;
import com.face.event.IEvent;
import com.face.event.PictureSizeArgs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SettingsFragment extends BaseDialogFragment {

    private static final SettingsFragment sInstance = new SettingsFragment();

    public final IEvent<PictureSizeArgs> PreviewSizeChanged = new Event<>();
    public final IEvent<FlashModeArgs> FlashModeChanged = new Event<>();
    public final IEvent<FocusModeArgs> FocusModeChanged = new Event<>();

    private FlashMode mFlashMode;
    private List<FlashMode> mFlashModes = Arrays.asList(FlashMode.values());

    private FocusMode mFocusMode;
    private List<FocusMode> mFocusModes = Arrays.asList(FocusMode.values());

    private PictureSize mPreviewSize;
    private ArrayList<PictureSize> mPreviewSizeList;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        mFocusMode = Configuration.i.getFocusMode();
        mFlashMode = Configuration.i.getFlashMode();

        mPreviewSizeList = Configuration.i.getPreviewSizeList();
        mPreviewSize = mPreviewSizeList.get(Configuration.i.getPreviewSizeId());
    }

    @Override
    public View onCreateView(LayoutInflater iInflater, ViewGroup iContainer, Bundle iSavedInstanceState) {
        View view = iInflater.inflate(R.layout.dialog_settings, iContainer, false);

        // ******************** FOCUS ******************** //
        Spinner focusSpinner = view.findViewById(R.id.focus_modes);
        focusSpinner.setAdapter(new ObjectToStringAdapter<>(mActivity, mFocusModes));
        focusSpinner.setSelection(mFocusModes.indexOf(mFocusMode));

        if (Configuration.i.getFocusOnTouchSupported()) {
            focusSpinner.setEnabled(true);
            focusSpinner.setClickable(true);
            focusSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> iParent, View iView, int iPosition, long iId) {
                    if (mFocusMode == mFocusModes.get(iPosition)) {
                        return;
                    }
                    mFocusMode = mFocusModes.get(iPosition);
                    FocusModeChanged.raise(this, new FocusModeArgs(mFocusMode));
                }

                @Override
                public void onNothingSelected(AdapterView<?> iParent) {
                }
            });
        } else {
            focusSpinner.setEnabled(false);
            focusSpinner.setClickable(false);
        }

        // ******************** FLASH ******************** //
        Spinner flashSpinner = view.findViewById(R.id.flash_modes);
        flashSpinner.setAdapter(new ObjectToStringAdapter<>(mActivity, mFlashModes));
        flashSpinner.setSelection(mFlashModes.indexOf(mFlashMode));

        if (Configuration.i.getFlashSupported()) {
            flashSpinner.setEnabled(true);
            flashSpinner.setClickable(true);
            flashSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> iParent, View iView, int iPosition, long iId) {
                    if (mFlashMode == mFlashModes.get(iPosition)) {
                        return;
                    }
                    mFlashMode = mFlashModes.get(iPosition);
                    FlashModeChanged.raise(this, new FlashModeArgs(mFlashMode));
                }

                @Override
                public void onNothingSelected(AdapterView<?> iParent) {
                }
            });
        } else {
            flashSpinner.setEnabled(false);
            flashSpinner.setClickable(false);
        }

        // ******************** PREVIEW SIZE ******************** //
        Spinner previewSizeSpinner = view.findViewById(R.id.preview_sizes);
        previewSizeSpinner.setAdapter(new ObjectToStringAdapter<>(mActivity, mPreviewSizeList));
        previewSizeSpinner.setSelection(mPreviewSizeList.indexOf(mPreviewSize));

        previewSizeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> iParent, View iView, int iPosition, long iId) {
                if (mPreviewSize == mPreviewSizeList.get(iPosition)) {
                    return;
                }
                mPreviewSize = mPreviewSizeList.get(iPosition);
                PreviewSizeChanged.raise(this, new PictureSizeArgs(mPreviewSize));
            }

            @Override
            public void onNothingSelected(AdapterView<?> iParent) {
            }
        });

        view.findViewById(R.id.close).setOnClickListener(v -> dismiss());

        return view;
    }

    @Override
    public String getFragmentTag() {
        return SettingsFragment.class.getSimpleName();
    }
}
