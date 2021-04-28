package com.esn.sonar.master.voice;

import com.esn.sonar.core.*;
import com.esn.sonar.core.util.Base64;
import com.esn.sonar.master.MasterMetrics;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.MasterStats;
import com.twitter.common.util.Clock;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;

import java.io.IOException;
import java.security.KeyPair;
import java.util.HashMap;

public class VoiceEdgeConnection extends InboundConnection
{
    private static final CommandHandler commandHandler = new CommandHandler();

    static
    {
        setupCommandHandler();
    }

    private final HashMap<String, VoiceServer> localServerMap = new HashMap<String, VoiceServer>();
    private final VoiceManager voiceManager;

    public VoiceEdgeConnection(VoiceManager voiceManager)
    {
        super(false, -1, 10);

        this.voiceManager = voiceManager;
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        long startNanos = Clock.SYSTEM_CLOCK.nowNanos();
        super.messageReceived(ctx, e);
        long elapsedNanos = Clock.SYSTEM_CLOCK.nowNanos() - startNanos;
        MasterStats.getStatsManager().getRequestStats().requestComplete(elapsedNanos / 1000);
    }

    private void onEdgeServerRegistered(Message msg)
    {
        if (msg.getArgumentCount() < 5)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);
        // String location = msg.getArgument(1);
        String voipAddress = msg.getArgument(2);
        String voipPort = msg.getArgument(3);
        String maxClientsStr = msg.getArgument(4);

        int port;
        int maxClients;
        try
        {
            port = Integer.parseInt(voipPort);
            maxClients = Integer.parseInt(maxClientsStr);
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return;
        }

        VoiceServer edgeVoiceServer = voiceManager.mapClientToServer(serverId, voipAddress, port, maxClients, this);
        localServerMap.put(edgeVoiceServer.getId(), edgeVoiceServer);
    }

    private void onEdgeServerUnregistered(Message msg)
    {
        //channel.write(new Message(getMessageId(), Protocol.Commands.EdgeServerUnregistered, serverId));

        if (msg.getArgumentCount() < 1)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);


        VoiceServer edgeVoiceServer = voiceManager.unmapClientToServer(serverId, "", "");

        if (edgeVoiceServer == null)
        {
            return;
        }

        localServerMap.remove(edgeVoiceServer.getId());
    }


    private void onEventStateClient(Message msg)
    {
        if (msg.getArgumentCount() < 6)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);
        String operatorId = msg.getArgument(1);
        String userId = msg.getArgument(2);
        String userDesc = msg.getArgument(3);
        String channelId = msg.getArgument(4);
        String channelDesc = msg.getArgument(5);


        voiceManager.onStateClient(serverId, operatorId, userId, userDesc, channelId, channelDesc);
    }

    private void onEventClientUnregistered(Message msg)
    {
        //To change body of created methods use File | Settings | File Templates.
        if (msg.getArgumentCount() < 5)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);
        String operatorId = msg.getArgument(1);
        String userId = msg.getArgument(2);
        String channelId = msg.getArgument(3);

        try
        {
            int numClients = Integer.parseInt(msg.getArgument(4));
            voiceManager.onUserUnregistered(serverId, operatorId, userId, channelId, numClients);
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return;
        }
    }

    private void onEventClientRegisteredToChannel(Message msg)
    {
        //EVENT_CLIENT_REGISTERED_TOCHANNEL<2>jonas<3>jonas<4>jonas<5>jonas<6>192.168.1.174<7>58145<8>numClients

        if (msg.getArgumentCount() < 9)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);
        String operatorId = msg.getArgument(1);
        String userId = msg.getArgument(2);
        String userDesc = msg.getArgument(3);
        String channelId = msg.getArgument(4);
        String channelDesc = msg.getArgument(5);
//        String notUsed1 = msg.getArgument(6);
//        String notUsed2 = msg.getArgument(7);
        String strNumClients = msg.getArgument(8);

        int numClients;
        try
        {
            numClients = Integer.parseInt(strNumClients);
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return;
        }

        voiceManager.onUserRegisteredToChannel(serverId, operatorId, userId, userDesc, channelId, channelDesc, numClients);
    }

    private void onEventChannelDestroyed(Message msg)
    {
        if (msg.getArgumentCount() < 3)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String serverId = msg.getArgument(0);
        String operatorId = msg.getArgument(1);
        String channelId = msg.getArgument(2);

        voiceManager.onChannelDestroyed(serverId, operatorId, channelId);
    }

    public String getId()
    {
        return null;
    }

    @Override
    protected void onRegistrationTimeout()
    {
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        return false;
    }

    @Override
    protected boolean onMessage(Message msg) throws Exception
    {
        if (!commandHandler.call(this, msg))
        {
            MasterServer.voiceEdgeLog.warn(String.format("Unknown command '%s' from client %s", msg.getName(), this));

            return false;
        }

        return true;
    }


    @Override
    protected void onResult(Message msg)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelConnected() throws IOException
    {
        updateKeys();

        MasterServer.voiceEdgeLog.info(String.format("Voice Edge server %s registered", this));
    }

    private void updateKeys() throws IOException
    {
        KeyPair keyPair = MasterServer.getInstance().getKeyPair();
        byte[] prvEnc = keyPair.getPrivate().getEncoded();
        byte[] pubEnc = keyPair.getPublic().getEncoded();

        sendCommand(Protocol.Commands.EdgeUpdateKeys, Base64.encodeBytes(prvEnc), Base64.encodeBytes(pubEnc));
    }

    @Override
    protected void onChannelDisconnected()
    {
        MasterServer.voiceEdgeLog.info(String.format("Voice Edge server %s unregistered", this));

        for (VoiceServer voiceServer : localServerMap.values())
        {
            voiceManager.unmapClientToServer(voiceServer.getId(), Protocol.Reasons.ConnectionLost, "Edge server lost");
        }
    }

    public void sendClientUnregister(String serverId, String operatorId, String userId, String reasonType, String reasonDesc)
    {
        sendCommand(Protocol.VoiceServerCommands.ClientUnregister, serverId, operatorId, userId, reasonType, reasonDesc);
    }

    public void sendChannelDestroy(String serverId, String operatorId, String channelId, String reasonType, String reasonDesc)
    {
        sendCommand(Protocol.VoiceServerCommands.ChannelDestroy, serverId, operatorId, channelId, reasonType, reasonDesc);
    }

    public void sendEdgeUnregisterClient(String serverId, String reasonType, String reasonDesc)
    {
        sendCommand(Protocol.Commands.EdgeUnregisterServer, serverId, reasonType, reasonDesc);
    }

    @Override
    public String toString()
    {
        return this.getRemoteAddress().toString();
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        MasterStats.getStatsManager().metric(MasterMetrics.VoiceEdgeConnectionError).incrementAndGet();
        MasterStats.getStatsManager().getRequestStats().incErrors();
        super.exceptionCaught(ctx, e);
    }

    /**
     * Initialization of command handler, performed only once when this class is first loaded.
     */
    private static void setupCommandHandler()
    {
        commandHandler.registerCallback(Protocol.VoiceServerEvents.ClientRegisteredToChannel, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEventClientRegisteredToChannel(msg);
            }
        });
        commandHandler.registerCallback(Protocol.VoiceServerEvents.ClientUnregistered, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEventClientUnregistered(msg);
            }
        });
        commandHandler.registerCallback(Protocol.VoiceServerEvents.StateClient, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEventStateClient(msg);
            }
        });
        commandHandler.registerCallback(Protocol.Events.ChannelDestroyed, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEventChannelDestroyed(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeServerRegistered, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEdgeServerRegistered(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeServerUnregistered, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((VoiceEdgeConnection) instance).onEdgeServerUnregistered(msg);
            }
        });
    }

    @Override
    protected void closeChannel()
    {
        super.closeChannel();    //To change body of overridden methods use File | Settings | File Templates.
    }
}
