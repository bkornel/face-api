package com.face.fragment;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.view.View;

public class BaseFragment extends Fragment {

    protected Activity mActivity;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);
    }

    @Override
    public void onAttach(Activity iActivity) {
        super.onAttach(iActivity);
        mActivity = iActivity;
    }

    public void showActionBar() {
        if (mActivity.getActionBar() != null) {
            mActivity.getActionBar().show();
        }
    }

    public void hideActionBar() {
        if (mActivity.getWindow() != null) {
            View decorView = mActivity.getWindow().getDecorView();
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN);
        }

        if (mActivity.getActionBar() != null) {
            mActivity.getActionBar().hide();
        }
    }
}
