package com.esn.sonartest.VoiceEdgeTest;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-07-06
 * Time: 09:01
 * To change this template use File | Settings | File Templates.
 */
public class VoiceEdgeTestConfig extends AbstractConfig
{
    private Double connectServer = readFromSystem("connectServer", 2.0);
    private Double disconnectServer = readFromSystem("disconnectServer", 1.0);
    private Double reviveServer = readFromSystem("reviveServer", 1.0);
    private Double stealServer = readFromSystem("stealServer", 1.0);

    private Integer channelCount = readFromSystem("channelCount", 1000);
    private Integer usersPerChannel = readFromSystem("usersPerChannel", 2);
    private Integer voiceServerCount = readFromSystem("voiceServerCount", 100);
    private InetSocketAddress voiceEdgeAddress = readFromSystem("voiceEdgeAddress", new InetSocketAddress("127.0.0.1", 22991));


    public VoiceEdgeTestConfig(String path) throws IOException, ConfigurationException
    {
        super(path);
    }

    public Double getConnectServer()
    {
        return connectServer;
    }

    public Integer getChannelCount()
    {
        return channelCount;
    }

    public Double getDisconnectServer()
    {
        return disconnectServer;
    }

    public Integer getVoiceServerCount()
    {
        return voiceServerCount;
    }

    public InetSocketAddress getVoiceEdgeAddress()
    {
        return voiceEdgeAddress;
    }

    public Integer getUsersPerChannel()
    {
        return usersPerChannel;
    }

    public Double getReviveServer()
    {
        return reviveServer;
    }

    public Double getStealServer()
    {
        return stealServer;
    }
}
