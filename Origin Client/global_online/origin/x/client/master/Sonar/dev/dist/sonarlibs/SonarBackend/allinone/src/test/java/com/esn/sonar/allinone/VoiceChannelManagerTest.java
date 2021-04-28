package com.esn.sonar.allinone;

import org.junit.Test;

public class VoiceChannelManagerTest
{
    @Test
    public void dummy()
    {

    }

    /*
    private VoiceChannelManager channelManager;
    private VoiceManager serverManager;

    @Before
    public void start()
    {
        OperatorManager sphereManager = new OperatorManager(new Statistics());

        channelManager = new VoiceChannelManager();
        serverManager = new VoiceManager(sphereManager, new MasterConfig());
    }

    @Before
    public void stop()
    {
        channelManager = null;
    }

    @Test
    public void createChannelNotExist() throws Throwable
    {
        VoiceEdgeConnection voiceServer = new VoiceEdgeConnection("server1", "");
        assertEquals (voiceServer, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer));
    }

    @Test
    public void createChannelThatExist() throws Throwable
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        VoiceEdgeConnection voiceServer2 = new VoiceEdgeConnection("server2", "");
        serverManager.registerClient(voiceServer1);
        serverManager.registerClient(voiceServer2);

        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1));
        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer2));
        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer2));
    }

    @Test
    public void getChannelStateNotFound()
    {
        assertEquals (VoiceChannelManager.ChannelState.NOT_FOUND, channelManager.validateChannel("server1", "channelId"));
    }

    @Test
    public void getChannelStateOtherServer() throws Throwable
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1));
        assertEquals (VoiceChannelManager.ChannelState.OTHER_SERVER, channelManager.validateChannel("server2", "channelId"));
    }

    @Test
    public void getChannelStateDestroyed() throws Throwable
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1));

        channelManager.setChannelAsDestroyed ("channelId");
        assertEquals (VoiceChannelManager.ChannelState.DESTROYED, channelManager.validateChannel("server1", "channelId"));
    }

    @Test
    public void getChannelStateAccepted() throws Throwable
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        assertEquals (voiceServer1, channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1));
        assertEquals (VoiceChannelManager.ChannelState.ACCEPTED, channelManager.validateChannel("server1", "channelId"));
    }

    @Test
    public void validateChannelTestAccepted()
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1);

        assertEquals(VoiceChannelManager.ChannelState.ACCEPTED, channelManager.validateChannel("server1", "channelId"));
    }

    @Test
    public void validateChannelTestNotFound()
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1);

        assertEquals(VoiceChannelManager.ChannelState.NOT_FOUND, channelManager.validateChannel("server1", "channelId2"));
        assertEquals(VoiceChannelManager.ChannelState.NOT_FOUND, channelManager.validateChannel("server2", "channelId3"));
    }

    @Test
    public void validateChannelTestOtherServer()
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1);

        assertEquals(VoiceChannelManager.ChannelState.OTHER_SERVER, channelManager.validateChannel("server2", "channelId"));
    }

    @Test
    public void validateChannelTestDestroyed()
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1);
        channelManager.setChannelAsDestroyed("channelId");
        assertEquals(VoiceChannelManager.ChannelState.DESTROYED, channelManager.validateChannel("server1", "channelId"));
    }

    @Test
    public void unlinkChannel()
    {
        VoiceEdgeConnection voiceServer1 = new VoiceEdgeConnection("server1", "");
        channelManager.createChannel("channelId", "channelDesc", serverManager, voiceServer1);
        channelManager.unlinkChannel("channelId");
        assertEquals(VoiceChannelManager.ChannelState.NOT_FOUND, channelManager.validateChannel("server1", "channelId"));
      
    }
    */
}
