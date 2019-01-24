package com.yalantis.cameramodule.common.adapter;

import java.util.List;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

public abstract class BaseListAdapter<T> extends BaseAdapter {

    private Context mContext;
    private List<T> mList;
    private int mLayoutId;

    BaseListAdapter(Context context, List<T> list, int layout) {
        mContext = context;
        setList(list);
        mLayoutId = layout;
    }

    @Override
    public int getCount() {
        if (mList != null) {
            return mList.size();
        }
        return 0;
    }

    public void addItems(List<T> items) {
        mList.addAll(items);
        notifyDataSetChanged();
    }

    public void addItem(T type) {
        mList.add(type);
        notifyDataSetChanged();
    }

    @Override
    public T getItem(int position) {
        return position >= 0 && position < mList.size() ? mList.get(position) : null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public Context getContext() {
        return mContext;
    }

    public Resources getResources() {
        return getContext().getResources();
    }

    public List<T> getList() {
        return mList;
    }

    public void setList(List<T> list) {
        this.mList = list;
    }

    public void removeItem(int position) {
        mList.remove(position);
        notifyDataSetChanged();
    }

    public View getLayout(ViewGroup parent) {
        return getInflater().inflate(mLayoutId, parent, false);
    }

    LayoutInflater getInflater() { return LayoutInflater.from(mContext); }

    public void clear() {
        mList.clear();
        notifyDataSetChanged();
    }
}
