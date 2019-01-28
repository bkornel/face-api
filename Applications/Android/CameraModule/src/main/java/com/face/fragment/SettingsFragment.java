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

    public static SettingsFragment getInstance() {
        return sInstance;
    }

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
        View view = iInflater.inflate(R.layout.dialog_camera_params, iContainer, false);

        Spinner focusSwitcher = view.findViewById(R.id.focus_modes);
        focusSwitcher.setAdapter(new ObjectToStringAdapter<>(mActivity, mFocusModes));
        focusSwitcher.setSelection(mFocusModes.indexOf(mFocusMode));
        focusSwitcher.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

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

        Spinner flashSwitcher = view.findViewById(R.id.flash_modes);

        if (Configuration.i.getFlashModeEnabled()) {
            flashSwitcher.setEnabled(true);
            flashSwitcher.setClickable(true);
            flashSwitcher.setAdapter(new ObjectToStringAdapter<>(mActivity, mFlashModes));
            flashSwitcher.setSelection(mFlashModes.indexOf(mFlashMode));

            flashSwitcher.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

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
            List<String> disabledFlashModes = new ArrayList<>();
            disabledFlashModes.add("Off");

            flashSwitcher.setAdapter(new ObjectToStringAdapter<>(mActivity, disabledFlashModes));
            flashSwitcher.setSelection(0);
            flashSwitcher.setEnabled(false);
            flashSwitcher.setClickable(false);
        }

        Spinner psSwitcher = view.findViewById(R.id.preview_sizes);
        psSwitcher.setAdapter(new ObjectToStringAdapter<>(mActivity, mPreviewSizeList));
        psSwitcher.setSelection(mPreviewSizeList.indexOf(mPreviewSize));
        psSwitcher.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

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
