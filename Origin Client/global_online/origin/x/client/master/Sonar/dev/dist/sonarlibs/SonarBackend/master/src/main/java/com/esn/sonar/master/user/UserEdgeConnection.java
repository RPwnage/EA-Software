package com.esn.sonar.master.user;

import com.esn.sonar.core.CommandCallback;
import com.esn.sonar.core.CommandHandler;
import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Base64;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.MasterStats;
import com.esn.sonar.master.MasterMetrics;
import com.twitter.common.util.Clock;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;

import java.io.IOException;
import java.security.KeyPair;
import java.util.HashMap;

public class UserEdgeConnection extends InboundConnection
{
    private static final CommandHandler commandHandler = new CommandHandler();

    static
    {
        setupCommandHandler();
    }

    private final HashMap<String, User> localUserMap = new HashMap<String, User>();
    private String publicAddress;
    private int publicPort;
    private final UserManager userManager;

    public UserEdgeConnection(UserManager userManager)
    {
        super(true, 30, 60);

        this.userManager = userManager;
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        long startNanos = Clock.SYSTEM_CLOCK.nowNanos();
        super.messageReceived(ctx, e);
        long elapsedNanos = Clock.SYSTEM_CLOCK.nowNanos() - startNanos;
        MasterStats.getStatsManager().getRequestStats().requestComplete(elapsedNanos / 1000);
    }

    private void handleEdgeUserRegistered(Message msg)
    {
        /*
        [operatorId] [userId] [userDesc] [remoteAddress]
         */

        if (msg.getArgumentCount() < 4)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String userDesc = msg.getArgument(2);
        String remoteAddress = msg.getArgument(3);

        User user = userManager.mapClientToServer(operatorId, userId, userDesc, remoteAddress, this);

        localUserMap.put(user.getUserId(), user);

    }

    private void handleEdgeUserUnregistered(Message msg)
    {
        if (msg.getArgumentCount() < 5)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String userDesc = msg.getArgument(2);
        String reasonType = msg.getArgument(3);
        String reasonDesc = msg.getArgument(4);

        User user = localUserMap.remove(userId);

        if (user == null)
        {
            //TODO: Log this, spurious event
            return;
        }

        userManager.unmapClientToServer(operatorId, userId, userDesc, reasonType, reasonDesc);
    }


    @Override
    protected void onRegistrationTimeout()
    {
        closeChannel();
    }

    @Override
    protected boolean onRegisterCommand(Message msg) throws IOException
    {
        /*
        [publicIP] [publicPort]
         */

        if (msg.getArgumentCount() < 2)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return false;
        }

        //TODO: Maybe validate these arguments?
        this.publicAddress = msg.getArgument(0);
        try
        {
            this.publicPort = Integer.parseInt(msg.getArgument(1));
        } catch (NumberFormatException e)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        userManager.registerClient(this);

        updateKeys();

        sendSuccessReply(msg.getId());
        return true;
    }

    private void updateKeys() throws IOException
    {
        KeyPair keyPair = MasterServer.getInstance().getKeyPair();
        byte[] prvEnc = keyPair.getPrivate().getEncoded();
        byte[] pubEnc = keyPair.getPublic().getEncoded();

        sendCommand(Protocol.Commands.EdgeUpdateKeys, Base64.encodeBytes(prvEnc), Base64.encodeBytes(pubEnc));
    }

    @Override
    protected boolean onMessage(Message msg) throws Exception
    {
        return commandHandler.call(this, msg);
    }

    @Override
    protected void onResult(Message msg)
    {
    }

    @Override
    protected void onChannelDisconnected()
    {
        userManager.unregisterClient(this, Protocol.Reasons.ConnectionLost, "");

        for (User user : localUserMap.values())
        {
            userManager.unmapClientToServer(user.getOperatorId(), user.getUserId(), user.getUserDesc(), Protocol.Reasons.ConnectionLost, "Edge server lost");
        }
    }

    @Override
    protected void onChannelConnected()
    {
    }

    @Override
    public String getId()
    {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public void sendEdgeUpdateToken(String operatorId, String userId, Token token, String type)
    {
        sendCommand(Protocol.Commands.EdgeUpdateToken, operatorId, userId, token.toString(), type);
    }

    public void sendEdgeUnregisterClient(String operatorId, String userId, String reasonType, String reasonDesc)
    {
        sendCommand(Protocol.Commands.EdgeUnregisterClient, operatorId, userId, reasonType, reasonDesc);
    }

    public String getPublicAddress()
    {
        return publicAddress;
    }

    public int getPublicPort()
    {
        return publicPort;
    }

    @Override
    public String toString()
    {
        return publicAddress + ":" + publicPort;
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        MasterStats.getStatsManager().metric(MasterMetrics.UserEdgeConnectionError).incrementAndGet();
        MasterStats.getStatsManager().getRequestStats().incErrors();
        super.exceptionCaught(ctx, e);
    }

    /**
     * Initialization of command handler, performed only once when this class is first loaded.
     */
    private static void setupCommandHandler()
    {
        commandHandler.registerCallback(Protocol.Commands.EdgeUserRegistered, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((UserEdgeConnection) instance).handleEdgeUserRegistered(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.EdgeUserUnregistered, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((UserEdgeConnection) instance).handleEdgeUserUnregistered(msg);
            }
        });
    }
}
