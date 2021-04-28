package com.esn.sonartest.VoiceEdgeTest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.challenge.ChallengePacket;
import com.esn.sonartest.verifier.EventVerifier;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.*;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.spec.InvalidKeySpecException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

import static com.esn.sonar.core.util.Utils.createChallengeSocket;

public class VoiceServer extends OutboundConnection
{
    private final int maxUsers;
    private final String uuid;
    private final Channel challengeSocket;
    private final int voipPort;
    private final String publicAddress;
    private long stateMsgId = -1;

    private MessageCallback registrationSuccess;
    private MessageCallback registrationError;
    private Map<String, UserState> userMap = new ConcurrentHashMap<String, UserState>();
    private final String operatorId;

    EventVerifier verifier = new EventVerifier();
    private Map<String, Runnable> expectationMap = new ConcurrentHashMap<String, Runnable>();

    public VoiceServer(ClientBootstrap bootstrap, InetSocketAddress masterAddress, ChannelFactory datagramChannelFactory, String publicAddress, int maxUsers, String operatorId)
    {
        super(-1, 60, true, bootstrap, masterAddress);

        this.uuid = UUID.randomUUID().toString();
        this.maxUsers = maxUsers;
        this.challengeSocket = createChallengeSocket(new InetSocketAddress(0), new ChallengeHandler(), datagramChannelFactory);
        this.voipPort = ((InetSocketAddress) challengeSocket.getLocalAddress()).getPort();
        this.publicAddress = publicAddress;
        this.operatorId = operatorId;
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        if (msg.getName().equals(Protocol.Commands.Keepalive))
        {
            write(new Message(msg.getId(), Protocol.Commands.Result, "OK"));
            return true;
        } else if (msg.getName().equals(Protocol.VoiceServerCommands.ClientUnregister))
        {
            String userId = msg.getArgument(2);
            String channelId = msg.getArgument(4);

            String expectation = createClientUnregisterExpectation(userId);
            Runnable runnable = expectationMap.remove(expectation);

            if (runnable == null)
            {
                throw new RuntimeException();
            }

            runnable.run();
            return true;
        }

        throw new RuntimeException();
    }

    @Override
    protected boolean onResult(Message msg)
    {
        if (msg.getId() == stateMsgId)
        {
            this.registrationSuccess.onMessage(msg);
            return true;
        }

        throw new RuntimeException();
    }

    @Override
    protected void onChannelConnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onSendRegister()
    {
        String version = String.format("%d.%d", Protocol.Versions.voiceServerMajor, 1);
        write(new Message(getMessageId(), Protocol.Commands.Register, uuid, publicAddress + ":" + voipPort, Integer.toString(voipPort), version, Integer.toString(maxUsers), ""));
    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        this.registrationError.onMessage(msg);
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        for (UserState userState : userMap.values())
        {
            write(new Message(getMessageId(), Protocol.VoiceServerEvents.StateClient,
                    uuid,
                    operatorId,
                    userState.getUser().getUserId(),
                    userState.getUser().getUserDesc(),
                    userState.getChannel().getChannelId(),
                    userState.getChannel().getChannelDesc()));
        }

        stateMsgId = getMessageId();
        write(new Message(stateMsgId, Protocol.Commands.Keepalive));

    }

    public void setRegistrationError(MessageCallback registrationError)
    {
        this.registrationError = registrationError;
    }

    public void setRegistrationSuccess(MessageCallback registrationSuccess)
    {
        this.registrationSuccess = registrationSuccess;
    }

    public void setupState(User user, VoiceChannel channel)
    {
        UserState state = new UserState(user, channel);
        userMap.put(state.getUser().getUserId(), state);
    }

    public void clearState(Collection<User> outUsers, Collection<VoiceChannel> outChannels)
    {
        Collection<UserState> values = userMap.values();

        Set<VoiceChannel> channelSet = new HashSet<VoiceChannel>();

        for (UserState value : values)
        {
            outUsers.add(value.getUser());
            channelSet.add(value.getChannel());
        }

        outChannels.addAll(channelSet);

        userMap.clear();
    }

    public VoiceChannel getVoiceChannel()
    {
        return userMap.values().iterator().next().getChannel();
    }

    public void writeClientRegisteredToChannel(String userId, String userDesc, String channelId, String channelDesc, int count)
    {
        write(new Message(getMessageId(), Protocol.VoiceServerEvents.ClientRegisteredToChannel, uuid.toString(), operatorId, userId, userDesc, channelId, channelDesc, "localhost", "12345", Integer.toString(count)));
    }

    private static String createClientUnregisterExpectation(String userId)
    {
        return Protocol.VoiceServerEvents.ClientUnregistered + "|" + userId;
    }

    public void expectClientUnregister(String userId, String channelId, Runnable callback)
    {
        String expectation = createClientUnregisterExpectation(userId);
        expectationMap.put(expectation, callback);
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

    private class UserState
    {
        private VoiceChannel channel;
        private User user;

        public UserState(User user, VoiceChannel channel)
        {
            this.user = user;
            this.channel = channel;
        }

        public User getUser()
        {
            return user;
        }

        public VoiceChannel getChannel()
        {
            return channel;
        }
    }

    public interface MessageCallback
    {
        void onMessage(Message msg);
    }
}
