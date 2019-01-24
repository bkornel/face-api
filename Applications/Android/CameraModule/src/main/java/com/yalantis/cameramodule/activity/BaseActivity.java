package com.yalantis.cameramodule.activity;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;

public abstract class BaseActivity extends Activity {

    protected Handler handler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        handler = new Handler(msg -> BaseActivity.this.handleMessage());
    }

    protected boolean handleMessage() {
        return false;
    }

    protected void showActionBar() {
        if (getActionBar() != null) {
            getActionBar().show();
        }
    }

    protected void hideActionBar() {
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
