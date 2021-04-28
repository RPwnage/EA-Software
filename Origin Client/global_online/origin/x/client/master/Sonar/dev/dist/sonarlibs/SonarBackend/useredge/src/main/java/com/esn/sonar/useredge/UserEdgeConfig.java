package com.esn.sonar.useredge;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

public class UserEdgeConfig extends AbstractConfig
{

    public UserEdgeConfig(String path)
            throws IOException, ConfigurationException
    {
        super(path);
    }

    private InetSocketAddress bindAddress = readFromSystem("sue.bindAddress", new InetSocketAddress(22990));
    private InetSocketAddress masterAddress = readFromSystem("sue.masterAddress", new InetSocketAddress("localhost", 30000));
    private Integer controlTokenExpire = readFromSystem("sue.controlTokenExpire", 3600);
    private Integer registrationTimeout = readFromSystem("sue.registrationTimeout", 10);
    private final Integer keepaliveInterval = readFromSystem("sue.keepaliveInterval", 60);
    private String localAddress = readFromSystem("sue.publicAddress", guessLocalAddress());
    private int masterConnectionRetry = readFromSystem("sue.masterRetry", 1);
    private InetSocketAddress httpAddress = readFromSystem("sue.httpAddress", new InetSocketAddress(30005));
    private String logLevel = readFromSystem("sue.logLevel", "ERROR");

    public InetSocketAddress getBindAddress()
    {
        return bindAddress;
    }

    public int getControlTokenExpire()
    {
        return controlTokenExpire;
    }

    public int getRegistrationTimeout()
    {
        return registrationTimeout;
    }

    public int getKeepaliveInterval()
    {
        return keepaliveInterval;
    }

    public String getLocalAddress()
    {
        return localAddress;
    }

    public InetSocketAddress getMasterAddress()
    {
        return masterAddress;
    }

    public void setBindAddress(InetSocketAddress bindAddress)
    {
        this.bindAddress = bindAddress;
    }

    public void setRegistrationTimeout(int registrationTimeout)
    {
        this.registrationTimeout = registrationTimeout;
    }

    public void setLocalAddress(String localAddress)
    {
        this.localAddress = localAddress;
    }

    public void setMasterAddress(InetSocketAddress masterAddress)
    {
        this.masterAddress = masterAddress;
    }


    public void setControlTokenExpire(int controlTokenExpire)
    {
        this.controlTokenExpire = controlTokenExpire;
    }

    public void setMasterConnectionRetry(int masterConnectionRetry)
    {
        this.masterConnectionRetry = masterConnectionRetry;
    }

    public int getMasterConnectionRetry()
    {
        return masterConnectionRetry;
    }

    public InetSocketAddress getHttpAddress()
    {
        return httpAddress;
    }

    public String getLogLevel()
    {
        return logLevel;
    }
}
