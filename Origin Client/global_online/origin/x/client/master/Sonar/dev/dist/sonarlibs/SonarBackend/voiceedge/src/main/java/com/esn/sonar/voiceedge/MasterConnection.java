package com.esn.sonar.voiceedge;

import com.esn.sonar.core.*;
import com.esn.sonar.core.util.Base64;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.ChannelFuture;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import java.util.concurrent.CountDownLatch;

public class MasterConnection extends OutboundConnection
{
    private static final CommandHandler commandHandler = new CommandHandler();

    static
    {
        commandHandler.registerCallback(Protocol.Commands.EdgeUnregisterServer, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((MasterConnection) instance).handleEdgeUnregisterServer(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeUpdateKeys, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((MasterConnection) instance).handleEdgeUpdateKeys(msg);
            }
        });

        commandHandler.registerCallback(Protocol.VoiceServerCommands.ClientUnregister, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((MasterConnection) instance).handleClientUnregister(msg);
            }
        });

        commandHandler.registerCallback(Protocol.VoiceServerCommands.ChannelDestroy, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((MasterConnection) instance).handleChannelDestroy(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.Keepalive, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((MasterConnection) instance).handleKeepalive(msg);
            }
        });
    }

    private void handleKeepalive(Message msg)
    {
        write(new Message(msg.getId(), Protocol.Commands.Reply, "OK"));
    }


    protected final CountDownLatch initialConnectLatch = new CountDownLatch(1);

    private void handleChannelDestroy(Message msg)
    {
        VoiceEdgeManager.getInstance().relayCommand(msg);
    }

    private void handleClientUnregister(Message msg)
    {
        VoiceEdgeManager.getInstance().relayCommand(msg);
    }


    private void handleEdgeUpdateKeys(Message msg) throws IOException, InvalidKeySpecException
    {
        byte[] prvKey = Base64.decode(msg.getArgument(0));
        byte[] pubKey = Base64.decode(msg.getArgument(1));

        VoiceEdgeManager.getInstance().updateKeyPair(pubKey, prvKey);
    }

    private void handleEdgeUnregisterServer(Message msg)
    {
        String serverId = msg.getArgument(0);
        String reasonType = msg.getArgument(1);
        String reasonDesc = msg.getArgument(2);

        VoiceEdgeManager.getInstance().disconnectClient(serverId, reasonType, reasonDesc);

        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.ClientsDisconnected).incrementAndGet();

    }

    public MasterConnection(ClientBootstrap bootstrap, InetSocketAddress masterAddress) throws NoSuchAlgorithmException
    {
        super(1, 10, false, bootstrap, masterAddress);
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        if (commandHandler.call(this, msg))
        {
            return true;
        }

        VoiceEdgeServer.voiceEdgeLog.fatal(String.format("Unexpected message '%s' received from voice server %s", msg.getName(), this));

        return true;
    }

    @Override
    protected boolean onResult(Message msg)
    {
        return true;
    }

    @Override
    protected void onChannelDisconnected()
    {
        VoiceEdgeServer.voiceEdgeLog.info(String.format("Voice Edge connection to master lost"));
    }

    @Override
    protected void onChannelConnected()
    {
        initialConnectLatch.countDown();
        VoiceEdgeManager.getInstance().dropAllClients();

        VoiceEdgeServer.voiceEdgeLog.info(String.format("Voice Edge connection to master established"));
        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.MasterConnects).incrementAndGet();
    }

    public void sendEdgeServerRegistered(String serverId, String location, String publicAddress, int voipPort, int maxClients)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeServerRegistered, serverId, location, publicAddress, Integer.toString(voipPort), Integer.toString(maxClients)));
    }

    public void sendEdgeServerUnregistered(String serverId)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeServerUnregistered, serverId));
    }

    @Override
    protected void onRegisterError(Message msg)
    {
    }

    @Override
    protected void onSendRegister()
    {
    }

    @Override
    protected void onRegisterSuccess(Message msg)
    {
    }

    @Override
    public ChannelFuture close()
    {
        return super.close();    //To change body of overridden methods use File | Settings | File Templates.
    }
}
