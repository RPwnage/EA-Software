package com.esn.sonar.master.voice;


import com.esn.geoip.Position;
import com.esn.geoip.Positionable;

public class VoiceServer implements Positionable
{
    private volatile int userCount;
    private final String serverId;
    private final VoiceEdgeConnection connection;
    private final String voipAddress;
    private final int voipPort;
    private final int maxClients;
    private final Position position;

    public VoiceServer(VoiceEdgeConnection connection, Position position, String serverId, String voipAddress, int voipPort, int maxClients)
    {
        this.connection = connection;
        this.serverId = serverId;
        this.voipAddress = voipAddress;
        this.voipPort = voipPort;
        this.position = position;
        this.maxClients = maxClients;
        this.userCount = 0;
    }

    public Position getPosition()
    {
        return position;
    }

    public int getUserCount()
    {
        return userCount;
    }

    public String getId()
    {
        return serverId;
    }

    public void sendClientUnregister(String operatorId, String userId, String reasonType, String reasonDesc)
    {
        connection.sendClientUnregister(serverId, operatorId, userId, reasonType, reasonDesc);
    }

    public String getVoipAddress()
    {
        return voipAddress;
    }

    public int getVoipPort()
    {
        return voipPort;
    }

    public void sendChannelDestroy(String operatorId, String channelId, String reasonType, String reasonDesc)
    {
        connection.sendChannelDestroy(serverId, operatorId, channelId, reasonType, reasonDesc);
    }

    public void sendEdgeUnregisterClient(String reasonType, String reasonDesc)
    {
        connection.sendEdgeUnregisterClient(serverId, reasonType, reasonDesc);
    }

    public int getMaximumUsers()
    {
        return maxClients;
    }

    public synchronized void increaseUserCount()
    {
        userCount++;
    }

    public synchronized void setUserCount(int count)
    {
        userCount = count;
    }


    @Override
    public String toString()
    {
        return serverId;
    }

    public VoiceEdgeConnection getConnection()
    {
        return connection;
    }
}
