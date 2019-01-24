package com.yalantis.cameramodule.event;

@FunctionalInterface
public interface IEventHandler<T extends EventArgs> {
    void handle(Object iSender, T iArgs);
}
