package com.face.common.adapter;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.face.R;

import java.util.List;

public class ObjectToStringAdapter<T> extends BaseListAdapter<T> {

    public ObjectToStringAdapter(Context iContext, List<T> iList) {
        super(iContext, iList, R.layout.object_to_string_list_item);
    }

    @Override
    public View getView(int iPosition, View ioView, ViewGroup iParent) {
        return createView(iPosition, ioView, iParent, false);
    }

    @Override
    public View getDropDownView(int iPosition, View ioView, ViewGroup iParent) {
        return createView(iPosition, ioView, iParent, true);
    }

    @SuppressWarnings(value = "unchecked")
    private View createView(int iPosition, View ioView, ViewGroup iParent, boolean iDropDown) {
        @SuppressWarnings("unchecked")
        TextView text;

        if (ioView == null) {
            ioView = (iDropDown ? getDropDownLayout(iParent) : getLayout(iParent));
            text = ioView.findViewById(R.id.title);
            ioView.setTag(text);
        } else {
            text = (TextView) ioView.getTag();
        }

        T item = getItem(iPosition);
        if (item != null) {
            text.setText(item.toString());
        }

        return ioView;
    }

    private View getDropDownLayout(ViewGroup iParent) {
        return getInflater().inflate(R.layout.object_to_string_dropdown_list_item, iParent, false);
    }
}
