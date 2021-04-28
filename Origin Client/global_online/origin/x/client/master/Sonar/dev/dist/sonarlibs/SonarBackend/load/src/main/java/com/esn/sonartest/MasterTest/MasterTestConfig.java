package com.esn.sonartest.MasterTest;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

public class MasterTestConfig extends AbstractConfig
{
    public MasterTestConfig(String path) throws IOException, ConfigurationException
    {
        super(path);
    }

    private InetSocketAddress userEdgeAddress = readFromSystem("userEdgeAddress", new InetSocketAddress("127.0.0.1", 30000));
    private InetSocketAddress voiceEdgeAddress = readFromSystem("voiceEdgeAddress", new InetSocketAddress("127.0.0.1", 30001));
    private InetSocketAddress operatorAddress = readFromSystem("operatorAddress", new InetSocketAddress("127.0.0.1", 30003));
    private InetSocketAddress eventAddress = readFromSystem("eventAddress", new InetSocketAddress("127.0.0.1", 30004));
    private String publicAddress = readFromSystem("publicAddress", guessLocalAddress());
    private String operatorId = readFromSystem("operatorId", "OPER");

    private Integer userStart = readFromSystem("userStart", 0);
    private Integer userEnd = readFromSystem("userEnd", 10);
    private Integer userEdgeCount = readFromSystem("userEdgeCount", 1);

    private Integer channelStart = readFromSystem("channelStart", 0);
    private Integer channelEnd = readFromSystem("channelEnd", 10000);

    private Integer voiceServerCount = readFromSystem("voiceServerCount", 1);
    private Integer voiceEdgeCount = readFromSystem("voiceEdgeCount", 1);

    private Integer operatorCount = readFromSystem("operatorCount", 10);

    private Integer eventClientCount = readFromSystem("eventClientCount", 10);
    private Integer verificationTimeout = readFromSystem("verificationTimeout", 10000);
    private Integer voiceServerMaxClients = readFromSystem("voiceServerMaxClients", 1000);


    private Double operatorConnects = readFromSystem("operatorConnects", 1.0);
    private Double eventConnects = readFromSystem("eventConnects", 1.0);
    private Double voiceConnects = readFromSystem("voiceConnects", 10.0);
    private Double userConnects = readFromSystem("userConnects", 1000.0);
    private Double userDisconnects = readFromSystem("userDisconnects", 10.0);
    private Double joinUserToChannel = readFromSystem("joinUserToChannel", 1.0);
    private Double partUserFromChannel = readFromSystem("partUserFromChannel", 10.0);
    private Double destroyChannel = readFromSystem("destroyChannel", 10.0);
    private Double getUserOnlineStatus = readFromSystem("getUsersOnlineStatus", 10.0);
    private Double getUsersInChannel = readFromSystem("getUsersInChannel", 10.0);
    private Double getUserControlToken = readFromSystem("getUserControlToken", 10.0);
    private Double disconnectUser = readFromSystem("disconnectUser", 10.0);

    private Integer noGeoip = readFromSystem("nogeoip", 0);

    public Integer getNoGeoip()
    {
        return noGeoip;
    }

    public Integer getUserStart()
    {
        return userStart;
    }

    public Integer getUserEnd()
    {
        return userEnd;
    }

    public Double getUserEdgeConnects()
    {
        return userConnects;
    }

    public Double getPartUserFromChannel()
    {
        return partUserFromChannel;
    }

    public Double getJoinUserToChannel()
    {
        return joinUserToChannel;
    }

    public Double getUsersInChannel()
    {
        return getUsersInChannel;
    }

    public Double getGetUserOnlineStatus()
    {
        return getUserOnlineStatus;
    }

    public Double getGetUserControlToken()
    {
        return getUserControlToken;
    }

    public Double getDestroyChannel()
    {
        return destroyChannel;
    }

    public Double getVoiceConnects()
    {
        return voiceConnects;
    }

    public Integer getChannelStart()
    {
        return channelStart;
    }

    public Integer getChannelEnd()
    {
        return channelEnd;
    }

    public InetSocketAddress getOperatorAddress()
    {
        return operatorAddress;
    }

    public InetSocketAddress getUserEdgeAddress()
    {
        return userEdgeAddress;
    }

    public InetSocketAddress getVoiceEdgeAddress()
    {
        return voiceEdgeAddress;
    }

    public String getPublicAddress()
    {
        return publicAddress;
    }

    public Integer getUserEdgeCount()
    {
        return userEdgeCount;
    }

    public Integer getVoiceEdgeCount()
    {
        return voiceEdgeCount;
    }

    public Integer getOperatorCount()
    {
        return operatorCount;
    }

    public Integer getVoiceServerCount()
    {
        return voiceServerCount;
    }

    public InetSocketAddress getEventAddress()
    {
        return eventAddress;
    }

    public Integer getEventClientCount()
    {
        return eventClientCount;
    }

    public Integer getVoiceServerMaxClients()
    {
        return voiceServerMaxClients;
    }

    public Integer getVerificationTimeout()
    {
        return verificationTimeout;
    }

    public Double getUserEdgeDisconnects()
    {
        return userDisconnects;
    }

    public Double getEventConnects()
    {
        return eventConnects;
    }

    public Double getOperatorConnects()
    {
        return operatorConnects;
    }

    public Double getDisconnectUser()
    {
        return disconnectUser;
    }

    public String getOperatorId()
    {
        return operatorId;
    }
}

