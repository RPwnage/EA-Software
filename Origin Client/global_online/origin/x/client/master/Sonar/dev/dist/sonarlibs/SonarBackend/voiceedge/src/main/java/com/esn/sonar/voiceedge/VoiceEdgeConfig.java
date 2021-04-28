package com.esn.sonar.voiceedge;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

public class VoiceEdgeConfig extends AbstractConfig
{
    private InetSocketAddress challengeBindAddress = readFromSystem("sve.challengeAddress", new InetSocketAddress(22991));
    private InetSocketAddress bindAddress = readFromSystem("sve.bindAddress", new InetSocketAddress(22991));
    private InetSocketAddress masterAddress = readFromSystem("sve.masterAddress", new InetSocketAddress("localhost", 30001));
    private Integer challengeTimeout = readFromSystem("sve.challengeTimeout", 5);
    private Integer registrationTimeout = readFromSystem("sve.registrationTimeout", 10);
    private final Integer keepaliveInterval = readFromSystem("sve.keepaliveInterval", 10);
    private InetSocketAddress httpAddress = readFromSystem("sve.httpAddress", new InetSocketAddress(30006));

    public VoiceEdgeConfig(String path)
            throws IOException, ConfigurationException
    {
        super(path);
    }

    public InetSocketAddress getChallengeBindAddress()
    {
        return challengeBindAddress;
    }

    public InetSocketAddress getMasterAddress()
    {
        return masterAddress;
    }

    public int getChallengeTimeout()
    {
        return challengeTimeout;
    }

    public InetSocketAddress getBindAddress()
    {
        return bindAddress;
    }

    public int getRegistrationTimeout()
    {
        return registrationTimeout;
    }

    public int getKeepaliveInterval()
    {
        return keepaliveInterval;
    }

    public void setBindAddress(InetSocketAddress bindAddress)
    {
        this.bindAddress = bindAddress;
    }

    public void setRegistrationTimeout(int registrationTimeout)
    {
        this.registrationTimeout = registrationTimeout;
    }

    public void setChallengeTimeout(int challengeTimeout)
    {
        this.challengeTimeout = challengeTimeout;
    }

    public void setChallengeBindAddress(InetSocketAddress challengeBindAddress)
    {
        this.challengeBindAddress = challengeBindAddress;
    }

    public void setMasterAddress(InetSocketAddress masterAddress)
    {
        this.masterAddress = masterAddress;
    }

    public InetSocketAddress getHttpAddress()
    {
        return this.httpAddress;
    }
}
