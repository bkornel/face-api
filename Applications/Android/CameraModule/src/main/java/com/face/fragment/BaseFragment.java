package com.face.fragment;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.os.Handler;

public class BaseFragment extends Fragment {

    protected Activity mActivity;
    private Handler mHandler;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);
        mHandler = new Handler();
    }

    @Override
    public void onAttach(Activity iActivity) {
        super.onAttach(iActivity);
        mActivity = iActivity;
    }
}
