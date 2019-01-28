package com.face.event;

import java.util.ArrayList;
import java.util.List;

public class Event<T extends EventArgs> implements IEvent<T> {

    private List<IEventHandler<T>> mEventHandlers;

    public Event() {
        mEventHandlers = new ArrayList<>();
    }

    @Override
    public void addHandler(IEventHandler<T> iHandler) {
        mEventHandlers.add(iHandler);
    }

    @Override
    public void removeHandler(IEventHandler<T> iHandler) {
        mEventHandlers.remove(iHandler);
    }

    @Override
    public void raise(Object iSender, T iArgs) {
        for (IEventHandler<T> handler : mEventHandlers) {
            handler.handle(iSender, iArgs);
        }
    }
}
