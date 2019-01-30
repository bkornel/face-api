package com.face.activity;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;

public abstract class BaseActivity extends Activity {

    protected Handler mHandler;

    @Override
    public void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);
        mHandler = new Handler(msg -> BaseActivity.this.handleMessage());
    }

    protected boolean handleMessage() {
        return false;
    }

    public void showActionBar() {
        if (getActionBar() != null) {
            getActionBar().show();
        }
    }

    public void hideActionBar() {
        if (getWindow() != null) {
            View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN);
        }

        if (getActionBar() != null) {
            getActionBar().hide();
        }
    }

    public void showBack() {
        if (getActionBar() != null) {
            getActionBar().setDisplayShowHomeEnabled(false);
            getActionBar().setDisplayHomeAsUpEnabled(true);
        }
    }

    public void hideBack() {
        if (getActionBar() != null) {
            getActionBar().setDisplayShowHomeEnabled(true);
            getActionBar().setDisplayHomeAsUpEnabled(false);
        }
    }
}
