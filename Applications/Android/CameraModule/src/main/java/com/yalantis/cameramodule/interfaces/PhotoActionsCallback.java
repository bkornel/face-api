package com.yalantis.cameramodule.interfaces;

public interface PhotoActionsCallback {

    void onOpenPhotoPreview(String path, String name);

    void onAddNote(int zpid, String name);

    void onRetake(String name);

    void onDeletePhoto(String name);

    void onDeleteHomePhotos(int zpid);

    void onDeleteAddressPhotos(String address);
}
