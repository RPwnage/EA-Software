package com.esn.sonar.master.api.event;

import com.esn.sonar.core.Message;

public interface EventConsumer
{
    public String getRoundRobinGroup();

    public void handleEvent(Message event);
}
