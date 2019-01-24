package com.yalantis.cameramodule.fragment;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.yalantis.cameramodule.R;
import com.yalantis.cameramodule.control.PinchImageView;

public class PhotoPreviewFragment extends BaseFragment {

    private Bitmap bitmap;
    private PinchImageView imageView;

    public static PhotoPreviewFragment newInstance(Bitmap bitmap) {
        PhotoPreviewFragment fragment = new PhotoPreviewFragment();
        fragment.bitmap = bitmap;

        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_photo_preview, container, false);
    }

    @Override
    public void onViewCreated(final View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        imageView = (PinchImageView) view.findViewById(R.id.photo);

        if (bitmap != null && !bitmap.isRecycled()) {
            imageView.setImageBitmap(bitmap);
        } else {
            imageView.setImageResource(R.drawable.no_image);
        }
    }

    public void setBitmap(Bitmap bitmap) {
        this.bitmap = bitmap;
        imageView.setImageBitmap(bitmap);
    }

}
