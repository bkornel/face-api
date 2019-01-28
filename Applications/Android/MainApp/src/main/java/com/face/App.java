package com.face;

import android.app.Application;
import com.face.common.Configuration;
import com.face.common.image.ImageManager;

public class App extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        Configuration.i.init(getApplicationContext());
        ImageManager.i.init(getApplicationContext());
    }
}
