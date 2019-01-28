package com.face.event;

public class PhotoSavedArgs extends EventArgs {

    private String mPath;

    public PhotoSavedArgs(String iPath) {
        mPath = iPath;
    }

    public String getPath() {
        return mPath;
    }
}
