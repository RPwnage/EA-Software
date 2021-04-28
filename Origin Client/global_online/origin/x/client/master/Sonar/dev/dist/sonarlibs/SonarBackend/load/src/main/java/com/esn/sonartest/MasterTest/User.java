package com.esn.sonartest.MasterTest;

import com.esn.sonar.core.Token;
import com.esn.sonartest.UserEdgeClient;

/**
 * User: ronnie
 * Date: 2011-06-17
 * Time: 11:17
 */
public class User
{
    private final String userId;
    private final String userDesc;
    private final MasterTestDirector masterTestDirector;
    private VoiceServer currentVoiceServer;
    private volatile Channel currentChannel;
    private volatile Channel pendingChannel;
    private UserEdgeClient edgeClient;

    public User(String userId, String userDesc, MasterTestDirector masterTestDirector)
    {
        this.userId = userId;
        this.userDesc = userDesc;
        this.masterTestDirector = masterTestDirector;
    }

    public String getUserId()
    {
        return userId;
    }

    public String getUserDesc()
    {
        return userDesc;
    }

    public synchronized void partCurrentChannel(String reasonType, String reasonDesc)
    {
        currentVoiceServer.disconnectClient(this, getCurrentChannel(), reasonType, reasonDesc);
        currentVoiceServer = null;
        currentChannel = null;
    }

    public synchronized void joinChannel(VoiceServer voiceServer, Channel channel)
    {
        if (currentVoiceServer != null)
        {
            partCurrentChannel("LEAVING", "");
        }

        currentChannel = channel;
        currentVoiceServer = voiceServer;

        currentVoiceServer.connectClient(this, channel);
    }

    public synchronized void updateToken(Token newToken, String type)
    {
        if (currentVoiceServer != null)
        {
            throw new RuntimeException("Current voice server was not null");
        }

        currentChannel = pendingChannel;
        pendingChannel = null;

        String voiceAddress = newToken.getVoipAddress() + ":" + newToken.getVoipPort();
        currentVoiceServer = masterTestDirector.getVoiceServerByAddress(voiceAddress);
        currentVoiceServer.connectClient(this, currentChannel);
    }

    public String getRemoteAddress()
    {
        return "127.0.0.1";
    }

    public Channel getCurrentChannel()
    {
        return currentChannel;
    }

    public synchronized void setPendingChannel(Channel pendingChannel)
    {
        if (this.pendingChannel != null)
        {
            throw new IllegalStateException("");
        }

        this.pendingChannel = pendingChannel;
    }

    public UserEdgeClient getEdgeClient()
    {
        if (edgeClient == null)
        {
            throw new RuntimeException("edgeClient is null");
        }

        return edgeClient;
    }

    public void setEdgeClient(UserEdgeClient edgeClient)
    {
        this.edgeClient = edgeClient;
    }
}
