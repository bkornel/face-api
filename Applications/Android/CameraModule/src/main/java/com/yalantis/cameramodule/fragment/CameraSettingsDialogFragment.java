package com.yalantis.cameramodule.fragment;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Spinner;

import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.common.Configuration;
import com.yalantis.cameramodule.common.adapter.ObjectToStringAdapter;
import com.yalantis.cameramodule.common.camera.FlashMode;
import com.yalantis.cameramodule.common.camera.FocusMode;
import com.yalantis.cameramodule.common.camera.PictureSize;
import com.yalantis.cameramodule.event.Event;
import com.yalantis.cameramodule.event.FlashModeArgs;
import com.yalantis.cameramodule.event.FocusModeArgs;
import com.yalantis.cameramodule.event.IEvent;
import com.yalantis.cameramodule.event.PictureSizeArgs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CameraSettingsDialogFragment extends BaseDialogFragment {

    private static final CameraSettingsDialogFragment sInstance = new CameraSettingsDialogFragment();

    public final IEvent<PictureSizeArgs> PreviewSizeChanged = new Event<>();
    public final IEvent<FlashModeArgs> FlashModeChanged = new Event<>();
    public final IEvent<FocusModeArgs> FocusModeChanged = new Event<>();

    private FlashMode mFlashMode;
    private List<FlashMode> mFlashModes = Arrays.asList(FlashMode.values());

    private FocusMode mFocusMode;
    private List<FocusMode> mFocusModes = Arrays.asList(FocusMode.values());

    private PictureSize mPreviewSize;
    private ArrayList<PictureSize> mPreviewSizeList;

    public static CameraSettingsDialogFragment getInstance() {
        return sInstance;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mFocusMode = Configuration.i.getFocusMode();
        mFlashMode = Configuration.i.getFlashMode();

        mPreviewSizeList = Configuration.i.getPreviewSizeList();
        mPreviewSize = mPreviewSizeList.get(Configuration.i.getPreviewSizeId());
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.dialog_camera_params, container, false);

        Spinner focusSwitcher = view.findViewById(R.id.focus_modes);
        focusSwitcher.setAdapter(new ObjectToStringAdapter<>(mActivity, mFocusModes));
        focusSwitcher.setSelection(mFocusModes.indexOf(mFocusMode));
        focusSwitcher.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mFocusMode == mFocusModes.get(position)) {
                    return;
                }
                mFocusMode = mFocusModes.get(position);
                FocusModeChanged.raise(this, new FocusModeArgs(mFocusMode));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
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
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    if (mFlashMode == mFlashModes.get(position)) {
                        return;
                    }
                    mFlashMode = mFlashModes.get(position);
                    FlashModeChanged.raise(this, new FlashModeArgs(mFlashMode));
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
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
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mPreviewSize == mPreviewSizeList.get(position)) {
                    return;
                }
                mPreviewSize = mPreviewSizeList.get(position);
                PreviewSizeChanged.raise(this, new PictureSizeArgs(mPreviewSize));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        view.findViewById(R.id.close).setOnClickListener(v -> dismiss());

        return view;
    }

    @Override
    public String getFragmentTag() {
        return CameraSettingsDialogFragment.class.getSimpleName();
    }
}
