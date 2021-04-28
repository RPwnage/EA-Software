package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.challenge.ChallengePacket;
import com.esn.sonar.core.util.Base64;
import org.apache.log4j.Logger;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.*;
import org.jboss.netty.channel.socket.DatagramChannelFactory;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.spec.EncodedKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import static com.esn.sonar.core.util.Utils.createChallengeSocket;

public class FakeVoiceServerClient extends SonarClientHandler
{
    private final Logger log = Logger.getLogger(FakeVoiceServerClient.class);
    private static final Map<Integer, FakeVoiceServerClient> voiceServers = new ConcurrentHashMap<Integer, FakeVoiceServerClient>();

    private final int voipPort;

    private final Channel challengeSocket;
    private final boolean invalidChallenge;
    private final boolean dropChallenge;
    private final boolean skipRegistration;
    private final boolean doubleRegistration;
    private final boolean ignoreKeepalive;
    private DatagramChannelFactory datagramChannelFactory;
    private PublicKey publicKey;
    private int maxClients;
    private UUID uuid;


    public int getVoipPort()
    {
        return voipPort;
    }

    public int getMaxClients()
    {
        return maxClients;
    }

    public int getClientCount()
    {
        int clients = 0;

        for (FakeOperator operator : operatorMap.values())
        {
            clients += operator.clientMap.size();
        }

        return clients;
    }

    static class FakeChannel
    {
        final ConcurrentHashMap<String, FakeUserClient> clients = new ConcurrentHashMap<String, FakeUserClient>();
    }

    static class FakeOperator
    {
        final ConcurrentHashMap<String, FakeUserClient> clientMap = new ConcurrentHashMap<String, FakeUserClient>();
        final ConcurrentHashMap<String, FakeChannel> channelMap = new ConcurrentHashMap<String, FakeChannel>();
    }

    final ConcurrentHashMap<String, FakeOperator> operatorMap = new ConcurrentHashMap<String, FakeOperator>();


    private class ChallengeHandler extends SimpleChannelUpstreamHandler
    {
        @Override
        public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
        {
            if (FakeVoiceServerClient.this.dropChallenge)
            {
                return;
            }

            ChallengePacket packet = (ChallengePacket) e.getMessage();

            log.info("Got challenge " + Integer.toString(packet.getChallengeId()));

            if (FakeVoiceServerClient.this.invalidChallenge)
            {
                challengeSocket.write(new ChallengePacket(packet.getChallengeId() - 31337), e.getRemoteAddress());
            } else
            {
                challengeSocket.write(packet, e.getRemoteAddress());
            }
        }
    }

    public FakeVoiceServerClient(DatagramChannelFactory factory, ClientBootstrap bootstrap, AllInOneConfig config, boolean invalidChallenge, boolean dropChallenge, boolean skipRegistration, boolean doubleRegistration, boolean ignoreKeepalive, int maxClients)
    {
        super(bootstrap, config.userEdgeConfig.getLocalAddress(), config.voiceEdgeConfig.getBindAddress().getPort(), config);
        challengeSocket = createChallengeSocket(new InetSocketAddress(0), new ChallengeHandler(), factory);

        voipPort = ((InetSocketAddress) challengeSocket.getLocalAddress()).getPort();

        this.invalidChallenge = invalidChallenge;
        this.dropChallenge = dropChallenge;
        this.skipRegistration = skipRegistration;
        this.doubleRegistration = doubleRegistration;
        this.ignoreKeepalive = ignoreKeepalive;
        this.maxClients = maxClients;

        this.uuid = UUID.randomUUID();
    }

    public UUID getUuid()
    {
        return uuid;
    }

    public void setUuid(UUID uuid)
    {
        this.uuid = uuid;
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelConnected(ctx, e);
        log.info("Connected");

        sendRegister();
    }

    private void sendState()
    {
        for (FakeOperator operator : operatorMap.values())
        {
            for (FakeUserClient client : operator.clientMap.values())
            {
                channel.write(new Message(getMessageId(), Protocol.VoiceServerEvents.StateClient, uuid.toString(), client.getOperatorId(), client.getUserId(), client.getUserDescription(), client.getCurrentChannel(), ""));
            }
        }
    }

    @Override
    public SonarClientHandler sendRegister()
    {
        String version = String.format("%d.%d", Protocol.Versions.voiceServerMajor, 1);

        if (!skipRegistration)
        {
            channel.write(new Message(getMessageId(), Protocol.Commands.Register, uuid.toString(), hostname + ":" + voipPort, Integer.toString(voipPort), version, Integer.toString(maxClients), ""));

            if (doubleRegistration)
            {
                channel.write(new Message(getMessageId(), Protocol.Commands.Register, uuid.toString(), hostname + ":" + voipPort, Integer.toString(voipPort), version, Integer.toString(maxClients), ""));
            }
        }
        return this;
    }

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelDisconnected(ctx, e);
        log.info("Disconnected");
    }

    @Override
    public void onMessage(Message message)
    {
        if (message.getName().equals(Protocol.VoiceServerCommands.ChannelDestroy))
        {
            //TODO: Validate serverID!
            String serverId = message.getArgument(0);
            String operatorId = message.getArgument(1);
            String channelId = message.getArgument(2);

            log.info(String.format("Got %s, destroying channel %s:%s", message.getName(), operatorId, channelId));

            FakeChannel channel = operatorMap.get(operatorId).channelMap.get(channelId);

            if (channel != null)
            {
                ConcurrentHashMap<String, FakeUserClient> clients = channel.clients;

                for (FakeUserClient userClient : clients.values())
                {
                    disconnectUser(userClient);
                }
            }
        } else if (message.getName().equals(Protocol.VoiceServerCommands.ClientUnregister))
        {
            log.info("Disconnecting user");
            //TODO: Validate serverID!
            String serverId = message.getArgument(0);
            String operatorId = message.getArgument(1);
            String userId = message.getArgument(2);

            FakeUserClient userClient = operatorMap.get(operatorId).clientMap.get(userId);
            disconnectUser(userClient);
        } else if (message.getName().equals(Protocol.Commands.Keepalive))
        {
            if (ignoreKeepalive)
            {
                return;
            }

            channel.write(new Message(message.getId(), Protocol.Commands.Reply, "OK"));

            keepaliveLatch.countDown();
        }

        log.info(message);
    }

    @Override
    public void onRegister(Message message)
    {
        voiceServers.put(voipPort, this);

        try
        {
            String pubKey = message.getArgument(1);

            byte[] pubKeyBytes = Base64.decode(pubKey);

            KeyFactory keyFactory = KeyFactory.getInstance("RSA");
            EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(pubKeyBytes);
            this.publicKey = keyFactory.generatePublic(pubKeySpec);
        } catch (IOException e)
        {
        } catch (NoSuchAlgorithmException e)
        {
        } catch (InvalidKeySpecException e)
        {
        }

        sendState();
    }

    synchronized public void connectUser(FakeUserClient user, String rawToken) throws Token.InvalidToken
    {
        FakeOperator operator;

        Token token = new Token(rawToken, publicKey, -1);

        operator = operatorMap.get(user.getOperatorId());

        if (operator == null)
        {
            operator = new FakeOperator();
            operatorMap.put(user.getOperatorId(), operator);
        }

        log.info("User " + user.getUserId() + " connected to voice server");
        Token t = user.getCurrentToken();

        operator.clientMap.put(user.getUserId(), user);

        FakeChannel fakeChannel = operator.channelMap.get(t.getChannelId());

        if (fakeChannel == null)
        {
            fakeChannel = new FakeChannel();
            operator.channelMap.put(t.getChannelId(), fakeChannel);
        }

        fakeChannel.clients.put(user.getUserId(), user);
        if (channel != null)
        {
            channel.write(new Message(getMessageId(), Protocol.VoiceServerEvents.ClientRegisteredToChannel, uuid.toString(), t.getOperatorId(), t.getUserId(), t.getUserDescription(), t.getChannelId(), t.getChannelDescription(), "localhost", "12345", Integer.toString(getClientCount())));
        }
        user.onVoiceServerConnect();
    }

    synchronized public void disconnectUser(FakeUserClient user)
    {
        FakeOperator fakeOperator = operatorMap.get(user.getOperatorId());

        log.info("User " + user.getUserId() + " disconnected from voice server");

        fakeOperator.clientMap.remove(user.getUserId());
        FakeChannel fakeChannel = fakeOperator.channelMap.get(user.getCurrentChannel());

        boolean isChannelDestroyed = false;

        fakeChannel.clients.remove(user.getUserId());

        if (fakeChannel.clients.size() == 0)
        {
            log.info(String.format("Removing user %s from channel %s", user.getUserId(), user.getCurrentChannel()));
            fakeOperator.channelMap.remove(user.getCurrentChannel());
            isChannelDestroyed = true;
        }


        String userCurrentChannel = user.getCurrentChannel();

        user.onVoiceServerDisconnect();
        channel.write(new Message(getMessageId(), Protocol.VoiceServerEvents.ClientUnregistered, uuid.toString(), user.getOperatorId(), user.getUserId(), userCurrentChannel, Integer.toString(fakeChannel.clients.size())));

        if (isChannelDestroyed)
            channel.write(new Message(getMessageId(), Protocol.VoiceServerEvents.ChannelDestroyed, uuid.toString(), user.getOperatorId(), userCurrentChannel));
    }

    public static FakeVoiceServerClient getServer(int port)
    {
        return voiceServers.get(port);
    }

    @Override
    public void close()
    {
        super.close();    //To change body of overridden methods use File | Settings | File Templates.
        challengeSocket.close().awaitUninterruptibly();

    }

    @Override
    public SonarClientHandler waitForRegister() throws InterruptedException
    {
        SonarClientHandler ret = super.waitForRegister();    //To change body of overridden methods use File | Settings | File Templates.

        Thread.sleep(1000);
        return ret;

    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        log.error(e.getCause().toString());
    }
}


