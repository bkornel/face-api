package com.face.fragment;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.face.R;
import com.face.view.PhotoView;

public class PhotoFragment extends BaseFragment {

    private Bitmap mBitmap;
    private PhotoView mPhotoView;

    public static PhotoFragment newInstance(Bitmap iBitmap) {
        PhotoFragment fragment = new PhotoFragment();
        fragment.mBitmap = iBitmap;

        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater iInflater, ViewGroup iContainer, Bundle iSavedInstanceState) {
        return iInflater.inflate(R.layout.fragment_photo, iContainer, false);
    }

    @Override
    public void onViewCreated(final View iView, Bundle iSavedInstanceState) {
        super.onViewCreated(iView, iSavedInstanceState);

        mPhotoView = iView.findViewById(R.id.photo);

        if (mBitmap != null && !mBitmap.isRecycled()) {
            mPhotoView.setImageBitmap(mBitmap);
        } else {
            mPhotoView.setImageResource(R.drawable.no_image);
        }
    }

    public void setBitmap(Bitmap iBitmap) {
        mBitmap = iBitmap;
        mPhotoView.setImageBitmap(mBitmap);
    }
}
