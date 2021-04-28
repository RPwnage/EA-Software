package com.esn.sonartest.VoiceEdgeTest;

public class VoiceChannel
{
    private final String channelId;
    private final String channelDesc;

    public VoiceChannel(String channelId, String channelDesc)
    {
        this.channelDesc = channelDesc;
        this.channelId = channelId;
    }

    public String getChannelId()
    {
        return channelId;
    }

    public String getChannelDesc()
    {
        return channelDesc;
    }
}
