package com.esn.sonartest.MasterTest;

import com.esn.sonar.core.challenge.ChallengePacket;
import com.esn.sonartest.VoiceEdgeClient;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.channel.SimpleChannelUpstreamHandler;
import org.jboss.netty.channel.socket.DatagramChannelFactory;

import java.net.InetSocketAddress;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import static com.esn.sonar.core.util.Utils.createChallengeSocket;

public class VoiceServer
{
    private final String uuid;
    private final int voipPort;
    private final org.jboss.netty.channel.Channel challengeSocket;
    private final String location;
    private final String publicAddress;
    private final int maxClients;
    private final VoiceEdgeClient voiceEdgeClient;
    private final Map<String, User> users = new HashMap<String, User>();
    private final Map<String, Channel> channels = new HashMap<String, Channel>();

    public VoiceServer(DatagramChannelFactory datagramChannelFactory, String location, int maxClients, VoiceEdgeClient voiceEdgeClient)
    {
        UUID u = UUID.randomUUID();
        uuid = u.toString();
        challengeSocket = createChallengeSocket(new InetSocketAddress(0), new ChallengeHandler(), datagramChannelFactory);
        voipPort = ((InetSocketAddress) challengeSocket.getLocalAddress()).getPort();

        this.location = location;
        this.publicAddress = location;
        this.maxClients = maxClients;
        this.voiceEdgeClient = voiceEdgeClient;
    }

    public String getUUID()
    {
        return uuid;
    }

    public String getLocation()
    {
        return location;
    }

    public String getPublicAddress()
    {
        return publicAddress;
    }

    public int getVoipPort()
    {
        return voipPort;
    }

    public int getMaxClients()
    {
        return maxClients;
    }

    public synchronized void disconnectClient(User user, Channel channel, String reasonType, String reasonDesc)
    {
        Channel currentChannel = user.getCurrentChannel();
        currentChannel.removeUser(user);
        users.remove(user.getUserId());

        voiceEdgeClient.clientUnregistered(this, user, channel);

        if (currentChannel.getClientCount() == 0)
        {
            voiceEdgeClient.channelDestroyed(this, channel);
            channels.remove(channel.getChannelId());
        }
    }

    public synchronized void connectClient(User user, Channel channel)
    {
        if (channel == null)
        {
            throw new RuntimeException("Channel is null");
        }

        if (user == null)
        {
            throw new RuntimeException("User is null");
        }

        channel.addUser(user);
        users.put(user.getUserId(), user);

        if (users.size() > (maxClients * 2))
        {
            System.err.printf("WARNING: Maximum users reached at %s\n", this);
        }

        channels.put(channel.getChannelId(), channel);

        voiceEdgeClient.clientRegisteredToChannel(this, user, channel);
    }

    public void cmdClientUnregister(String operatorId, String userId, String reasonType, String reasonDesc)
    {
        User user = users.get(userId);

        user.partCurrentChannel(reasonType, reasonDesc);
    }

    public void cmdChannelDestroy(String operatorId, String channelId, String reasonType, String reasonDesc)
    {
        Channel channel = channels.get(channelId);

        User[] usersInChannel = channel.getUsers();

        for (User user : usersInChannel)
        {
            user.partCurrentChannel(reasonType, reasonDesc);
        }
    }

    public synchronized int getClientCount()
    {
        return users.size();
    }

    private class ChallengeHandler extends SimpleChannelUpstreamHandler
    {
        @Override
        public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
        {
            ChallengePacket packet = (ChallengePacket) e.getMessage();
            challengeSocket.write(packet, e.getRemoteAddress());
        }
    }


}
