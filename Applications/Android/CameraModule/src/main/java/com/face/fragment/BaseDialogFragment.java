package com.face.fragment;

import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.View;
import android.view.Window;

public abstract class BaseDialogFragment extends DialogFragment {

    protected Activity mActivity;

    @Override
    public Dialog onCreateDialog(Bundle iSavedInstanceState) {
        Dialog dialog = super.onCreateDialog(iSavedInstanceState);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(0));
        return dialog;
    }

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        View decorView = mActivity.getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN);

        if (mActivity.getActionBar() != null) {
            mActivity.getActionBar().hide();
        }
    }

    @Override
    public void onAttach(Activity iActivity) {
        super.onAttach(iActivity);
        mActivity = iActivity;
    }

    public abstract String getFragmentTag();

    public void show(FragmentManager iFragmentManager) {
        super.show(iFragmentManager, getFragmentTag());
    }
}
