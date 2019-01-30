package com.face.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.face.common.Configuration;
import com.face.common.Constants;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle iSavedInstanceState) {
        super.onCreate(iSavedInstanceState);

        Configuration.i.setUseFrontCamera(true);

        Intent intent = new Intent(this, CameraActivity.class);
        startActivityForResult(intent, Constants.CAMERA_ACTIVITY_REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int iRequestCode, int iResultCode, Intent iData) {
        if(iRequestCode == Constants.CAMERA_ACTIVITY_REQUEST_CODE) {
            finish();
        }
    }
}
