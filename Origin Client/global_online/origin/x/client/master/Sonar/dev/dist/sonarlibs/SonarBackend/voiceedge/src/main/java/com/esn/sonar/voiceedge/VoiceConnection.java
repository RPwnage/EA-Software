package com.esn.sonar.voiceedge;

import com.esn.sonar.core.*;
import com.esn.sonar.core.util.Base64;
import com.esn.sonar.core.util.Utils;
import com.twitter.common.util.Clock;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;

import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.security.KeyPair;
import java.security.PublicKey;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

import static com.esn.sonar.core.util.Utils.tokenize;

public class VoiceConnection extends InboundConnection
{
    private String serverId;
    private String location;
    private String voipAddress;
    private int voipPort;
    private int maxClients;

    private long registerReplyId = -1;
    private final VoiceEdgeConfig config;

    private static CommandHandler commandHandler = new CommandHandler();

    private static class EventRelayCallback implements CommandCallback
    {
        public void call(Object instance, Message msg) throws Exception
        {
            VoiceEdgeManager.getInstance().relayEvent(msg);
        }
    }

    static
    {
        EventRelayCallback callback = new EventRelayCallback();

        commandHandler.registerCallback(Protocol.VoiceServerEvents.ClientRegisteredToChannel, callback);
        commandHandler.registerCallback(Protocol.VoiceServerEvents.ClientUnregistered, callback);
        commandHandler.registerCallback(Protocol.VoiceServerEvents.ChannelDestroyed, callback);
        commandHandler.registerCallback(Protocol.VoiceServerEvents.StateClient, callback);
    }

    public VoiceConnection(VoiceEdgeConfig config)
    {
        super(true, config.getRegistrationTimeout(), config.getKeepaliveInterval());
        this.config = config;
    }

    public void finalizeRegistration()
    {
        KeyPair keyPair = VoiceEdgeManager.getInstance().getKeyPair();

        if (keyPair == null)
        {
            sendErrorReply(registerReplyId, Protocol.Reasons.TryAgain);
            return;
        }

        VoiceEdgeManager.getInstance().registerClient(this);
        setRegistered(true);

        PublicKey pubKey = keyPair.getPublic();

        sendSuccessReply(registerReplyId, pubKey.getAlgorithm(), pubKey.getFormat(), Base64.encodeBytes(pubKey.getEncoded()));
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        long startNanos = Clock.SYSTEM_CLOCK.nowNanos();
        super.messageReceived(ctx, e);
        long elapsedNanos = Clock.SYSTEM_CLOCK.nowNanos() - startNanos;
        VoiceEdgeStats.getStatsManager().getRequestStats().requestComplete(elapsedNanos / 1000);
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        VoiceEdgeStats.getStatsManager().getRequestStats().incErrors();
        super.exceptionCaught(ctx, e);
    }

    public void challengeExpired()
    {
        this.sendErrorReply(registerReplyId, Protocol.Errors.ChallengeExpired).addListener(new ChannelFutureListener()
        {
            public void operationComplete(ChannelFuture future) throws Exception
            {
                closeChannel();
            }
        });
    }

    public String getId()
    {
        return serverId;
    }

    public String getLocation()
    {
        return location;
    }

    @Override
    protected void onRegistrationTimeout()
    {
        sendUnregisterAndClose(Protocol.Errors.Timeout, "");
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        /*
        REGISTER [UUID] [localIP:port] [Voice port] [voiceserver version] [max clients] [location] */
        //TODO: Use location for real

        if (registerReplyId != -1)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.Unexpected);
            return false;
        }

        if (msg.getArgumentCount() < 6)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return false;
        }

        String argUuid = msg.getArgument(0);
        String argLocalAddress = msg.getArgument(1);
        String argVoipPort = msg.getArgument(2);
        String argVersion = msg.getArgument(3);
        String argMaxClients = msg.getArgument(4);
        this.location = msg.getArgument(5);

        //Validate UUID
        try
        {
            UUID.fromString(argUuid);
        } catch (IllegalArgumentException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        String[] versionSplit = tokenize(argVersion, '.');

        if (versionSplit.length != 2)
        {
            this.sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        try
        {
            int versionMajor = Integer.parseInt(versionSplit[0]);

            if (versionMajor != Protocol.Versions.voiceServerMajor)
            {
                this.sendErrorReply(msg.getId(), Protocol.Errors.WrongVersion);
                return false;
            }

        } catch (NumberFormatException e)
        {
            this.sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        //TODO: IPV6 issue here!
        String[] addrSplit = Utils.tokenize(argLocalAddress, ':');

        if (addrSplit.length != 2)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        voipAddress = addrSplit[0];

        this.serverId = argUuid;


        try
        {
            this.maxClients = Integer.valueOf(argMaxClients);

            if (maxClients < 1 || maxClients > 65535)
            {
                if (VoiceEdgeServer.voiceEdgeLog.isTraceEnabled())
                    VoiceEdgeServer.voiceEdgeLog.trace(String.format("Rejecting connection from %s. Invalid argument for max clients (%d) specified.", getRemoteAddress(), maxClients));

                sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
                return false;
            }

            this.voipPort = Integer.valueOf(argVoipPort);
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return false;
        }

        SocketAddress voipSocketAddress;
        try
        {
            voipSocketAddress = new InetSocketAddress(addrSplit[0], voipPort);
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        } catch (IllegalArgumentException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        registerReplyId = msg.getId();
        VoiceEdgeManager.getInstance().createChallenge(this, voipSocketAddress);


        return true;
    }

    @Override
    protected boolean onMessage(Message msg) throws Exception
    {
        if (!commandHandler.call(this, msg))
        {
            if (VoiceEdgeServer.voiceEdgeLog.isTraceEnabled())
            {
                VoiceEdgeServer.voiceEdgeLog.trace(String.format("Unexpected message '%s' received from voice server %s", msg.getName(), this));
            }
        }

        //VoiceEdgeManager.getInstance().relayEvent(serverId, msg);

        return true;
    }


    @Override
    protected void onResult(Message msg)
    {
    }

    @Override
    protected void onChannelDisconnected()
    {
        if (isRegistered())
        {
            VoiceEdgeManager.getInstance().unregisterClient(this, "CONNECTION_LOST", "");
        }
    }

    @Override
    protected void onChannelConnected()
    {
    }

    public String getVoipAddress()
    {
        return voipAddress;
    }

    public int getVoipPort()
    {
        return voipPort;
    }

    public int getMaxClients()
    {
        return maxClients;
    }

    @Override
    public String toString()
    {
        return serverId;
    }
}

