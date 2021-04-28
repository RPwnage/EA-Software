package com.esn.sonar.useredge;

import com.esn.sonar.core.*;
import com.esn.sonar.core.util.Utils;
import com.twitter.common.util.Clock;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.security.KeyPair;

public class UserConnection extends InboundConnection
{
    private String userId;
    private String userDescription;
    private String operatorId;
    private Token currToken;
    private final TokenExpireTask tokenExpireTask = new TokenExpireTask();

    private class TokenExpireTask implements TimerTask
    {
        private Timeout timeout;

        public void scheduleExpireTimer()
        {
            UserEdgeConfig config = UserEdgeManager.getInstance().getConfig();

            long expire =
                    UserConnection.this.currToken.getCreationTime() + config.getControlTokenExpire() - 2;

            expire -= (Utils.getTimestamp() / 1000);

            if (expire < 0)
            {
                expire = 1;
            }

            timeout = Maintenance.getInstance().scheduleTask((int) expire, this);
        }

        public void run(Timeout timeout) throws Exception
        {
            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.TokenUpdateTasks).incrementAndGet();

            currToken.setCreationTime(Utils.getTimestamp() / 1000);
            UserConnection.this.updateToken(currToken, Protocol.Reasons.Update);
        }

        public void cancel()
        {
            if (timeout != null)
            {
                timeout.cancel();
            }
        }

        public void tokenUpdated()
        {
            cancel();

            scheduleExpireTimer();

        }
    }

    public String getId()
    {
        return userId;
    }

    public UserConnection(UserEdgeConfig config)
    {
        super(true, config.getRegistrationTimeout(), config.getKeepaliveInterval());
    }


    public String getUserDescription()
    {
        return userDescription;
    }

    synchronized public void updateToken(Token token, String type)
    {
        currToken = token;
        tokenExpireTask.tokenUpdated();

        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.TokenUpdates).incrementAndGet();
        sendCommand(Protocol.Commands.UpdateToken, token.sign(UserEdgeManager.getInstance().getKeyPair().getPrivate()), type);
    }

    public String getOperatorId()
    {
        return operatorId;
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        long startNanos = Clock.SYSTEM_CLOCK.nowNanos();
        super.messageReceived(ctx, e);
        long elapsedNanos = Clock.SYSTEM_CLOCK.nowNanos() - startNanos;
        UserEdgeStats.getStatsManager().getRequestStats().requestComplete(elapsedNanos / 1000);
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        UserEdgeStats.getStatsManager().getRequestStats().incErrors();
        super.exceptionCaught(ctx, e);
    }

    @Override
    protected void onChannelDisconnected()
    {
        if (isRegistered())
        {
            UserEdgeManager.getInstance().unregisterClient(this, "CONNECTION_LOST", "");
            tokenExpireTask.cancel();
        }
    }

    @Override
    protected void onRegistrationTimeout()
    {
        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.RegistrationTimeouts).incrementAndGet();
        closeChannel();
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        /* Arguments
        [token] [version]
         */

        KeyPair keyPair = UserEdgeManager.getInstance().getKeyPair();

        if (keyPair == null)
        {
            /* No key-pair means that we're not talking to the master properly yet, let's boot the user out  */

            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolTryAgain).incrementAndGet();

            sendErrorReply(msg.getId(), Protocol.Reasons.TryAgain);
            return false;
        }

        if (msg.getArgumentCount() < 2)
        {
            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolNotEnoughArguments).incrementAndGet();

            this.sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return false;
        }

        String tokenString = msg.getArgument(0);
        String version = msg.getArgument(1);

        //T Validate token here!
        String[] versionSplit = Utils.tokenize(version, '.');

        if (versionSplit.length != 2)
        {
            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolInvalidArgument).incrementAndGet();

            this.sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }

        try
        {
            int versionMajor = Integer.parseInt(versionSplit[0]);

            if (versionMajor != Protocol.Versions.voiceClientMajor)
            {
                UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolWrongVersion).incrementAndGet();

                this.sendErrorReply(msg.getId(), Protocol.Errors.WrongVersion);
                return false;
            }

        } catch (NumberFormatException e)
        {
            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolInvalidArgument).incrementAndGet();

            this.sendErrorReply(msg.getId(), Protocol.Errors.InvalidArgument);
            return false;
        }


        try
        {
            currToken = new Token(tokenString, keyPair.getPublic(), UserEdgeManager.getInstance().getConfig().getControlTokenExpire());

            this.userId = currToken.getUserId();
            this.userDescription = currToken.getUserDescription();
            this.operatorId = currToken.getOperatorId();

            setRegistered(true);
            UserEdgeManager.getInstance().registerClient(this);

            tokenExpireTask.scheduleExpireTimer();

            sendSuccessReply(msg.getId());
        } catch (Token.InvalidToken invalidToken)
        {
            if (UserEdgeServer.userEdgeLog.isTraceEnabled())
                UserEdgeServer.userEdgeLog.trace(String.format("Rejecting connection from %s (%s)", getRemoteAddress(), invalidToken.getMessage()));

            UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.ProtocolInvalidToken).incrementAndGet();


            sendErrorReply(msg.getId(), Protocol.Errors.InvalidToken);
            return false;
        }

        return true;
    }

    @Override
    protected boolean onMessage(Message msg)
    {
        return false;
    }

    @Override
    protected void onResult(Message msg)
    {
    }


    @Override
    protected void onChannelConnected()
    {
        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.InboundConnections).incrementAndGet();
    }

    public void sendUpdateToken(String token)
    {
        sendCommand(Protocol.Commands.UpdateToken, token);
    }

    @Override
    public String toString()
    {
        return operatorId + "|" + userId;
    }
}

