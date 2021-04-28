package com.esn.sonar.master.voice;

import java.util.concurrent.atomic.AtomicLong;

public class Statistics
{
    public long tsStart;
    public final AtomicLong messagesEncoded = new AtomicLong(0);
    public final AtomicLong messagesDecoded = new AtomicLong(0);
    public final AtomicLong eventsPublished = new AtomicLong(0);
    public final AtomicLong eventsCreated = new AtomicLong(0);
    public final AtomicLong commandsHandled = new AtomicLong(0);
    public int sphereCount;
    public int userCount;
    public int channelCount;
    public int cacheChannelCount;
    public int cacheUserCount;
    public int cacheUserChannelCount;
    public int edgeUserCount;
    public int edgeSphereCount;
    public int edgeVoiceServerCount;
    public int edgeVoiceClientCount;
    public int edgeVoiceChallengeCount;

    @Override
    protected Object clone()
    {
        Statistics ret = new Statistics();
        ret.tsStart = tsStart;
        ret.messagesEncoded.set(messagesEncoded.get());
        ret.messagesDecoded.set(messagesDecoded.get());
        ret.eventsPublished.set(eventsPublished.get());
        ret.eventsCreated.set(eventsCreated.get());
        ret.commandsHandled.set(commandsHandled.get());


        return ret;
    }

}
