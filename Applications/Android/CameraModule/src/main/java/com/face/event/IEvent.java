package com.face.event;

public interface IEvent<T extends EventArgs> {

    void addHandler(IEventHandler<T> iHandler);

    void removeHandler(IEventHandler<T> iHandler);

    void raise(Object iSender, T iArgs);
}
