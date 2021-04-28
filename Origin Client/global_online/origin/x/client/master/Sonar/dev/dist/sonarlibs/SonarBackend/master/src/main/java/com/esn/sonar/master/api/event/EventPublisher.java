package com.esn.sonar.master.api.event;

import com.esn.sonar.core.Message;

public interface EventPublisher
{
    public void addConsumer(EventConsumer consumer);

    public void removeConsumer(EventConsumer consumer);

    public void publishEvent(Message event);

    public boolean hasConsumer(EventConsumer consumer);

}
