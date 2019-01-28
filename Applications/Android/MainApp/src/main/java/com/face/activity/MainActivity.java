package com.face.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.face.common.Configuration;

public class MainActivity extends Activity {
    private int mRequestCode = 1;

    @Override
    protected void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        Configuration.i.setUseFrontCamera(true);

        Intent intent = new Intent(this, CameraActivity.class);
        startActivityForResult(intent, mRequestCode);
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iData) {
        if(iRequestCode == mRequestCode) {
            finish();
        }
    }
}
