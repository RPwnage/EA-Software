package com.esn.sonar.useredge;

import com.esn.sonar.core.*;
import com.esn.sonar.core.util.Base64;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.spec.InvalidKeySpecException;
import java.util.concurrent.CountDownLatch;

public class MasterConnection extends OutboundConnection
{
    private static final CommandHandler commandHandler = new CommandHandler();

    static
    {
        setupCommandHandler();
    }

    protected final CountDownLatch registrationLatch = new CountDownLatch(1);
    private final UserEdgeConfig config;

    private void handleEdgeUnregisterClient(Message msg)
    {
        //sendCommand(Protocol.Commands.EdgeUnregisterClient, operatorId, userId, reasonType, reasonDesc);

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String reasonType = msg.getArgument(2);
        String reasonDesc = msg.getArgument(3);

        UserEdgeManager.getInstance().disconnectClient(operatorId, userId, reasonType, reasonDesc);
    }

    private void handleEdgeUpdateKeys(Message msg) throws IOException, InvalidKeySpecException
    {
        byte[] prvKey = Base64.decode(msg.getArgument(0));
        byte[] pubKey = Base64.decode(msg.getArgument(1));

        UserEdgeManager.getInstance().updateKeyPair(pubKey, prvKey);
    }

    private void handleEdgeUpdateToken(Message msg) throws Token.InvalidToken
    {
        //sendCommand(Protocol.Commands.EdgeUpdateToken, operatorId, userId, token.toString());

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String rawtoken = msg.getArgument(2);
        String type = msg.getArgument(3);

        Token token = new Token(rawtoken, null, -1);

        UserEdgeManager.getInstance().sendUpdateToken(operatorId, userId, token, type);

    }

    public MasterConnection(ClientBootstrap bootstrap, InetSocketAddress masterAddress, UserEdgeConfig config)
    {
        super(1, 10, true, bootstrap, masterAddress);
        this.config = config;
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        return commandHandler.call(this, msg);
    }

    @Override
    protected void onSendRegister()
    {
        write(new Message(getMessageId(),
                Protocol.Commands.Register,
                config.getLocalAddress(),
                Integer.toString(config.getBindAddress().getPort())));
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        UserEdgeManager.getInstance().dropAllClients();
        registrationLatch.countDown();

        UserEdgeServer.userEdgeLog.info(String.format("User Edge registration to master succeeded"));

    }

    @Override
    protected boolean onResult(Message msg)
    {
        return true;
    }

    @Override
    protected void onChannelConnected()
    {
        UserEdgeServer.userEdgeLog.info(String.format("User Edge connection to master established"));
        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.MasterConnects).incrementAndGet();
    }

    @Override
    protected void onChannelDisconnected()
    {
        UserEdgeServer.userEdgeLog.info(String.format("User Edge connection to master lost"));
    }

    public void sendEdgeUserRegistered(String operatorId, String userId, String userDescription, String remoteAddress)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeUserRegistered, operatorId, userId, userDescription, remoteAddress));
    }

    public void sendEdgeUserUnregistered(String operatorId, String userId, String userDescription, String reasonType, String reasonDesc)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeUserUnregistered, operatorId, userId, userDescription, reasonType, reasonDesc));
    }

    @Override
    protected void onRegisterError(Message msg)
    {
    }

    /**
     * Initialization of command handler, performed only once when this class is first loaded.
     */
    private static void setupCommandHandler()
    {
        commandHandler.registerCallback(Protocol.Commands.EdgeUnregisterClient, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((MasterConnection) instance).handleEdgeUnregisterClient(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeUpdateToken, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Token.InvalidToken
            {
                ((MasterConnection) instance).handleEdgeUpdateToken(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeUpdateKeys, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((MasterConnection) instance).handleEdgeUpdateKeys(msg);
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

}
