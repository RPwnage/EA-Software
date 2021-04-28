package com.esn.sonar.master;

import com.esn.sonar.master.api.event.EventPublisher;
import com.esn.sonar.master.api.event.RoundRobinEventPublisher;
import com.esn.sonar.master.user.User;
import com.esn.sonar.master.voice.VoiceCache;
import com.esn.sonar.master.voice.VoiceChannelManager;

import java.util.concurrent.ConcurrentHashMap;

public class Operator
{
    private final VoiceChannelManager voiceChannelManager;
    private final VoiceCache voiceCache;
    private final String operatorId;

    private final ConcurrentHashMap<String, User> userMap;
    private final EventPublisher eventPublisher;

    public Operator(String operatorId)
    {
        eventPublisher = new RoundRobinEventPublisher();

        this.operatorId = operatorId;
        voiceChannelManager = new VoiceChannelManager();
        voiceCache = new VoiceCache(eventPublisher, operatorId);
        userMap = new ConcurrentHashMap<String, User>();
    }

    public VoiceCache getCache()
    {
        return voiceCache;
    }

    public VoiceChannelManager getChannelManager()
    {
        return voiceChannelManager;
    }

    public String getId()
    {
        return operatorId;
    }

    public ConcurrentHashMap<String, User> getUserMap()
    {
        return userMap;
    }

    public EventPublisher getEventPublisher()
    {
        return eventPublisher;
    }
}
