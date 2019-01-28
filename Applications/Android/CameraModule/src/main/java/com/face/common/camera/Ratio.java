package com.face.common.camera;

public enum Ratio {
    R_4x3(0, 4, 3), R_16x9(1, 16, 9);

    private int id;

    public int w;

    public int h;

    Ratio(int id, int w, int h) {
        this.id = id;
        this.w = w;
        this.h = h;
    }

    public int getId() {
        return id;
    }

    public static Ratio getRatioById(int id) {
        for (Ratio ratio : values()) {
            if (ratio.id == id) {
                return ratio;
            }
        }
        return null;
    }

    public static Ratio pickRatio(int width, int height) {
        for (Ratio ratio : values()) {
            if (width / ratio.w == height / ratio.h) {
                return ratio;
            }
        }
        return null;
    }

    @Override
    public String toString() {
        return w + ":" + h;
    }

}
