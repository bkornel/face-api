package com.yalantis.cameramodule.common.adapter;

import java.util.List;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.yalantis.cameramodule.R;

public class ObjectToStringAdapter<T> extends BaseListAdapter<T> {

    public ObjectToStringAdapter(Context context, List<T> list) {
        super(context, list, R.layout.object_to_string_list_item);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        return createView(position, convertView, parent, false);
    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        return createView(position, convertView, parent, true);
    }

    @SuppressWarnings (value="unchecked")
    private View createView(int position, View convertView, ViewGroup parent, boolean dropDown) {
        @SuppressWarnings("unchecked")
        TextView text;

        if (convertView == null) {
            convertView = (dropDown ? getDropDownLayout(parent) : getLayout(parent));
            text = convertView.findViewById(R.id.title);
            convertView.setTag(text);
        } else {
            text = (TextView) convertView.getTag();
        }

        T item = getItem(position);
        if (item != null) {
            text.setText(item.toString());
        }

        return convertView;
    }

    private View getDropDownLayout(ViewGroup parent) {
        return getInflater().inflate(R.layout.object_to_string_dropdown_list_item, parent, false);
    }
}
