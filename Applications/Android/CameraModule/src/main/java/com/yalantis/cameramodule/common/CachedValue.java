package com.yalantis.cameramodule.common;

import android.content.SharedPreferences;

public class CachedValue<T> {

    private static SharedPreferences sSharedPreferences;

    private SharedPreferences mSharedPreferences;

    private T mValue;
    private T mDefaultValue;
    private Class mType;
    private String mName;
    private boolean mIsLoaded;

    public CachedValue(String iName, T iDefaultValue, Class iType) {
        this(iName, null, iDefaultValue, iType);
    }

    public CachedValue(String iName, T iValue, T iDefaultValue, Class iType) {
        mSharedPreferences = sSharedPreferences;
        mName = iName;
        mType = iType;
        mIsLoaded = (iValue != null);
        mValue = iValue;
        mDefaultValue = iDefaultValue;
    }

    public void setValue(T iValue) {
        mIsLoaded = true;
        write(mValue = iValue);
    }

    public T getValue() {
        if (!mIsLoaded) {
            mValue = load();
            mIsLoaded = true;
        }
        return mValue;
    }

    public String getName() {
        return mName;
    }

    private void write(T iValue) {
        SharedPreferences.Editor editor = mSharedPreferences.edit();

        if (iValue instanceof String) {
            editor.putString(mName, (String) iValue);
        } else if (iValue instanceof Integer) {
            editor.putInt(mName, (Integer) iValue);
        } else if (iValue instanceof Float) {
            editor.putFloat(mName, (Float) iValue);
        } else if (iValue instanceof Long) {
            editor.putLong(mName, (Long) iValue);
        } else if (iValue instanceof Boolean) {
            editor.putBoolean(mName, (Boolean) iValue);
        }

        editor.commit();
    }

    @SuppressWarnings("unchecked")
    private T load() {
        if (mType == String.class) {
            return (T) mSharedPreferences.getString(mName, (String) mDefaultValue);
        } else if (mType == Integer.class) {
            return (T) Integer.valueOf(mSharedPreferences.getInt(mName, (Integer) mDefaultValue));
        } else if (mType == Float.class) {
            return (T) Float.valueOf(mSharedPreferences.getFloat(mName, (Float) mDefaultValue));
        } else if (mType == Long.class) {
            return (T) Long.valueOf(mSharedPreferences.getLong(mName, (Long) mDefaultValue));
        } else if (mType == Boolean.class) {
            return (T) Boolean.valueOf(mSharedPreferences.getBoolean(mName, (Boolean) mDefaultValue));
        }

        return null;
    }

    public void delete() {
        mSharedPreferences.edit().remove(mName).commit();
        clear();
    }

    public static void initialize(SharedPreferences iSharedPreferences) {
        CachedValue.sSharedPreferences = iSharedPreferences;
    }

    public void clear() {
        mIsLoaded = false;
        mValue = null;
    }
}
