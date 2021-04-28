package com.esn.sonar.master;

import com.esn.sonar.core.AbstractConfig;

import java.io.IOException;
import java.net.InetSocketAddress;

public class MasterConfig extends AbstractConfig
{
    private InetSocketAddress voiceBindAddress = readFromSystem("sms.voiceAddress", new InetSocketAddress(30001));
    private InetSocketAddress operatorServiceBindAddress = readFromSystem("sms.operatorAddress", new InetSocketAddress(30003));
    private InetSocketAddress httpServiceBindAddress = readFromSystem("sms.httpAddress", new InetSocketAddress(30002));
    private InetSocketAddress userBindAddress = readFromSystem("sms.userAddress", new InetSocketAddress(30000));
    private InetSocketAddress eventServiceBindAddress = readFromSystem("sms.eventAddress", new InetSocketAddress(30004));
    private Integer joinChannelExpire = readFromSystem("sms.joinChannelExpire", 60);
    private String geoipPath = readFromSystem("sms.geoipPath", System.getProperty("user.home") + "/sonar-geoip");
    private String geoipProvider = readFromSystem("sms.geoipProvider", "GeoLiteCity");
    private String logLevel = readFromSystem("sms.logLevel", "");
    private Integer orphanTaskInterval = readFromSystem("sms.orphanTaskInterval", 60);
    private Integer maxOrphanCount = readFromSystem("sms.maxOrphanCount", 60);

    private String debugOperatorId = readFromSystem("debug.operator", "");
    private Integer debugEventsPerSecond = readFromSystem("debug.eventsPerSecond", 100);
    private Integer debugChannelCount = readFromSystem("debug.channelCount", 1000);
    private Long debugClientStart = readFromSystem("debug.clientStart", 0L);
    private Long debugClientEnd = readFromSystem("debug.clientEnd", 1000L);
    private Long debugClientsOnline = readFromSystem("debug.clientOnline", 100L);
    private Long debugStayTime = readFromSystem("debug.stayTime", (20 * 60L));
    private String debugUserDescPrefix = readFromSystem("debug.userDescPrefix", "");

    public Long getDebugClientsOnline()
    {
        return debugClientsOnline;
    }

    public Long getDebugStayTime()
    {
        return debugStayTime;
    }


    public Integer getDebugChannelCount()
    {
        return debugChannelCount;
    }

    public Long getDebugClientEnd()
    {
        return debugClientEnd;
    }

    public Long getDebugClientStart()
    {
        return debugClientStart;
    }

    public String getDebugUserDescPrefix()
    {
        return debugUserDescPrefix;
    }


    public Integer getDebugEventsPerSecond()
    {
        return debugEventsPerSecond;
    }

    public String getDebugOperatorId()
    {
        return debugOperatorId;
    }


    public MasterConfig(String path)
            throws IOException, ConfigurationException
    {
        super(path);
    }

    public String getLogLevel()
    {
        return logLevel;
    }

    public InetSocketAddress getVoiceBindAddress()
    {
        return voiceBindAddress;
    }

    public InetSocketAddress getOperatorServiceBindAddress()
    {
        return operatorServiceBindAddress;
    }

    public InetSocketAddress getHttpServiceBindAddress()
    {
        return httpServiceBindAddress;
    }

    public InetSocketAddress getUserBindAddress()
    {
        return userBindAddress;
    }

    public InetSocketAddress getEventServiceBindAddress()
    {
        return eventServiceBindAddress;
    }

    public int getJoinChannelExpire()
    {
        return joinChannelExpire;
    }

    public void setVoiceBindAddress(InetSocketAddress voiceBindAddress)
    {
        this.voiceBindAddress = voiceBindAddress;
    }

    public void setUserBindAddress(InetSocketAddress userBindAddress)
    {
        this.userBindAddress = userBindAddress;
    }

    public String getGeoipProvider()
    {
        return geoipProvider;
    }

    public void setOperatorServiceBindAddress(InetSocketAddress operatorServiceBindAddress)
    {
        this.operatorServiceBindAddress = operatorServiceBindAddress;
    }

    public void setHttpServiceBindAddress(InetSocketAddress httpServiceBindAddress)
    {
        this.httpServiceBindAddress = httpServiceBindAddress;
    }

    public void setEventServiceBindAddress(InetSocketAddress eventServiceBindAddress)
    {
        this.eventServiceBindAddress = eventServiceBindAddress;
    }

    public void setJoinChannelExpire(int joinChannelExpire)
    {
        this.joinChannelExpire = joinChannelExpire;
    }

    public String getGeoipPath()
    {
        return geoipPath;
    }

    public void setGeoipPath(String geoipPath)
    {
        this.geoipPath = geoipPath;
    }

    public int getOrphanTaskInterval()
    {
        return orphanTaskInterval;
    }

    public int getMaxOrphanCount()
    {
        return maxOrphanCount;
    }

    public void setMaxOrphanCount(Integer maxOrphanCount)
    {
        this.maxOrphanCount = maxOrphanCount;
    }

    public void setOrphanTaskInterval(Integer orphanTaskInterval)
    {
        this.orphanTaskInterval = orphanTaskInterval;
    }

}
