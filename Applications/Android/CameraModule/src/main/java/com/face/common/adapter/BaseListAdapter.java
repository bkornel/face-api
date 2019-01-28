package com.face.common.adapter;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import java.util.List;

public abstract class BaseListAdapter<T> extends BaseAdapter {

    private Context mContext;
    private List<T> mList;
    private int mLayoutId;

    BaseListAdapter(Context iContext, List<T> iList, int iLayout) {
        mContext = iContext;
        setList(iList);
        mLayoutId = iLayout;
    }

    @Override
    public int getCount() {
        if (mList != null) {
            return mList.size();
        }
        return 0;
    }

    public void addItems(List<T> iItems) {
        mList.addAll(iItems);
        notifyDataSetChanged();
    }

    public void addItem(T iType) {
        mList.add(iType);
        notifyDataSetChanged();
    }

    @Override
    public T getItem(int iPosition) {
        return iPosition >= 0 && iPosition < mList.size() ? mList.get(iPosition) : null;
    }

    @Override
    public long getItemId(int iPosition) {
        return iPosition;
    }

    private Context getContext() {
        return mContext;
    }

    public Resources getResources() {
        return getContext().getResources();
    }

    public List<T> getList() {
        return mList;
    }

    public void setList(List<T> iList) {
        mList = iList;
    }

    public void removeItem(int iPosition) {
        mList.remove(iPosition);
        notifyDataSetChanged();
    }

    public View getLayout(ViewGroup iParent) {
        return getInflater().inflate(mLayoutId, iParent, false);
    }

    LayoutInflater getInflater() {
        return LayoutInflater.from(mContext);
    }

    public void clear() {
        mList.clear();
        notifyDataSetChanged();
    }
}
