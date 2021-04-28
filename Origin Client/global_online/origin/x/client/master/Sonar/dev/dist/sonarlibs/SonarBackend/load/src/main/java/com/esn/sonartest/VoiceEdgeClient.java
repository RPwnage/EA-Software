package com.esn.sonartest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.MasterTest.*;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.socket.DatagramChannelFactory;

import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.CountDownLatch;

public class VoiceEdgeClient extends OutboundConnection
{
    private final HashMap<String, VoiceServer> voiceServerMap = new HashMap<String, VoiceServer>();
    private long msgId;
    private CountDownLatch stateLatch = new CountDownLatch(1);

    public VoiceEdgeClient(MasterTestConfig config, ClientBootstrap bootstrap, DatagramChannelFactory datagramChannelFactory)
    {
        super(-1, 60, false, bootstrap, config.getVoiceEdgeAddress());

    }

    public void setVoiceServers(Collection<VoiceServer> voiceServers)
    {
        for (VoiceServer voiceServer : voiceServers)
        {
            voiceServerMap.put(voiceServer.getUUID(), voiceServer);
        }
    }


    @Override
    protected void onSendRegister()
    {
    }

    protected boolean onCommand(Message msg) throws Exception
    {
        if (msg.getName().equals(Protocol.Commands.Keepalive))
        {
            Message replyMsg = new Message(msg.getId(), Protocol.Commands.Result, "OK");
            write(replyMsg);
            return true;
        } else if (msg.getName().equals(Protocol.Commands.EdgeUpdateKeys))
        {
            return true;
        } else if (msg.getName().equals(Protocol.VoiceServerCommands.ClientUnregister))
        {
            String serverId = msg.getArgument(0);
            String operatorId = msg.getArgument(1);
            String userId = msg.getArgument(2);
            String reasonType = msg.getArgument(3);
            String reasonDesc = msg.getArgument(4);

            VoiceServer voiceServer = this.voiceServerMap.get(serverId);

            voiceServer.cmdClientUnregister(operatorId, userId, reasonType, reasonDesc);
            return true;
        } else if (msg.getName().equals(Protocol.VoiceServerCommands.ChannelDestroy))
        {
            String serverId = msg.getArgument(0);
            String operatorId = msg.getArgument(1);
            String channelId = msg.getArgument(2);
            String reasonType = msg.getArgument(3);
            String reasonDesc = msg.getArgument(4);

            VoiceServer voiceServer = this.voiceServerMap.get(serverId);

            voiceServer.cmdChannelDestroy(operatorId, channelId, reasonType, reasonDesc);

            return true;
        }

        throw new RuntimeException("Unexpected command " + msg.getName());
    }

    @Override
    protected void onRegisterError(Message msg)
    {
    }

    @Override
    protected void onRegisterSuccess(Message msg)
    {
    }

    protected boolean onResult(Message msg)
    {
        if (msgId == msg.getId())
        {
            stateLatch.countDown();
        }

        return true;
    }

    private void sendVoiceServerRegistered(String serverId, String location, String publicAddress, int voipPort, int maxClients)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeServerRegistered, serverId, location, publicAddress, Integer.toString(voipPort), Integer.toString(maxClients)));
    }

    private void sendVoiceServerUnregistered(String serverId)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeServerUnregistered, serverId));
    }


    @Override
    protected void onChannelConnected()
    {
        for (VoiceServer voiceServer : voiceServerMap.values())
        {
            sendVoiceServerRegistered(voiceServer.getUUID(), voiceServer.getLocation(), voiceServer.getPublicAddress(), voiceServer.getVoipPort(), voiceServer.getMaxClients());
        }

        msgId = getMessageId();
        write(new Message(msgId, Protocol.Commands.Keepalive));

    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    public void clientRegisteredToChannel(VoiceServer voiceServer, User user, Channel channel)
    {
        write(new Message(getMessageId(),
                Protocol.VoiceServerEvents.ClientRegisteredToChannel,
                voiceServer.getUUID(),
                MasterTestDirector.operatorId, user.getUserId(),
                user.getUserDesc(),
                channel.getChannelId(),
                channel.getChannelDesc(),
                user.getRemoteAddress(),
                "31337",
                Integer.toString(voiceServer.getClientCount())));
    }

    public void clientUnregistered(VoiceServer voiceServer, User user, Channel channel)
    {
        write(new Message(getMessageId(), Protocol.VoiceServerEvents.ClientUnregistered, voiceServer.getUUID(), MasterTestDirector.operatorId, user.getUserId(), channel.getChannelId()));
    }

    public void waitForStateSync() throws InterruptedException
    {
        stateLatch.await();
    }

    public void channelDestroyed(VoiceServer voiceServer, Channel channel)
    {
        /*
        String serverId = msg.getArgument(0);
        String operatorId = msg.getArgument(1);
        String channelId = msg.getArgument(2);

         */
        write(new Message(getMessageId(), Protocol.VoiceServerEvents.ChannelDestroyed, voiceServer.getUUID(), MasterTestDirector.operatorId, channel.getChannelId()));

    }
}
