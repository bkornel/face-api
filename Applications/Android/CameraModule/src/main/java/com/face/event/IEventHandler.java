package com.face.event;

@FunctionalInterface
public interface IEventHandler<T extends EventArgs> {
    void handle(Object iSender, T iArgs);
}
