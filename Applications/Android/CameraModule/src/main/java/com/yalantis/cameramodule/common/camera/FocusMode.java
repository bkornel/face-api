package com.yalantis.cameramodule.common.camera;

public enum FocusMode {

    AUTO(0, "Auto"), TOUCH(1, "Touch");

    private int id;

    private String name;

    FocusMode(int id, String name) {
        this.id = id;
        this.name = name;
    }

    public int getId() {
        return id;
    }

    public static FocusMode getFocusModeById(int id) {
        for (FocusMode mode : values()) {
            if (mode.id == id) {
                return mode;
            }
        }
        return null;
    }

    @Override
    public String toString() {
        return name;
    }
}
