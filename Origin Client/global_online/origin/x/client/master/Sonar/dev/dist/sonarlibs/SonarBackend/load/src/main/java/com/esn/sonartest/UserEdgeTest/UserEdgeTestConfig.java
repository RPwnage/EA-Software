package com.esn.sonartest.UserEdgeTest;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

public class UserEdgeTestConfig extends AbstractConfig
{


    public UserEdgeTestConfig(String path) throws IOException, ConfigurationException
    {
        super(path);
    }

    private Integer userStart = readFromSystem("userStart", 0);
    private Integer userEnd = readFromSystem("userEnd", 35000);
    private Integer userConnects = readFromSystem("userConnects", 1000);
    private Integer userDisconnect = readFromSystem("userDisconnects", 500);
    private Integer joinUser = readFromSystem("joinUser", 100);
    private InetSocketAddress userEdgeAddress = readFromSystem("userEdgeAddress", new InetSocketAddress("127.0.0.1", 22990));
    private Integer operatorCount = readFromSystem("operatorCount", 10);
    private InetSocketAddress operatorAddress = readFromSystem("operatorAddress", new InetSocketAddress("127.0.0.1", 30003));

    public Integer getUserEnd()
    {
        return userEnd;
    }

    public Integer getUserStart()
    {
        return userStart;
    }

    public InetSocketAddress getUserEdgeAddress()
    {
        return userEdgeAddress;
    }

    public Integer getUserConnects()
    {
        return userConnects;
    }

    public Integer getUserDisconnect()
    {
        return userDisconnect;
    }

    public Integer getOperatorCount()
    {
        return operatorCount;
    }

    public InetSocketAddress getOperatorAddress()
    {
        return operatorAddress;
    }

    public Integer getJoinUser()
    {
        return joinUser;
    }
}
