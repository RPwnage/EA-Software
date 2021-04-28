package com.esn.sonartest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.verifier.Expectation;
import com.esn.sonartest.verifier.ResultVerifier;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.spec.InvalidKeySpecException;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

public class OperatorClient extends OutboundConnection
{
    private static final AtomicLong lastRequestId = new AtomicLong(31337);

    public OperatorClient(InetSocketAddress operatorAddress, ClientBootstrap bootstrap)
    {
        super(-1, 60, false, bootstrap, operatorAddress);
    }

    public void disconnectUser(String operatorId, String userId, String reasonType, String reasonDesc, Expectation expectation)
    {
        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.DisconnectUser, operatorId, userId, reasonType, reasonDesc);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }

    public void joinUserToChannel(String operatorId, String userId, String channelId, String channelDesc, String location, Expectation expectation)
    {
        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.JoinUserToChannel, operatorId, userId, channelId, channelDesc, location);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }

    public void partUserFromChannel(String operatorId, String userId, String channelId, String reasonType, String reasonDesc, Expectation expectation)
    {
        /*
        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String channelId = msg.getArgument(2);
        String reasonType = msg.getArgument(3);
        String reasonDesc = msg.getArgument(4);
        */

        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.PartUserFromChannel, operatorId, userId, channelId, reasonType, reasonDesc);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }

    public void getUsersInChannel(String operatorId, String channelId, Expectation expectation)
    {
        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.GetUsersInChannel, operatorId, channelId);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }

    public void destroyChannel(String operatorId, String channelId, String reasonType, String reasonDesc, Expectation expectation)
    {
        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.DestroyChannel, operatorId, channelId, reasonType, reasonDesc);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }

    public void getUserControlToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location, Expectation expectation)
    {
        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.GetUserControlToken, operatorId, userId, userDesc, channelId, channelDesc, location);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);
    }


    public void getUsersOnlineStatus(String operatorId, Collection<User> users, Expectation expectation)
    {
        List<String> argList = new LinkedList<String>();

        argList.add(operatorId);

        for (User user : users)
        {
            argList.add(user.getUserId());
        }

        String[] strs = argList.toArray(new String[argList.size()]);

        Message message = new Message(lastRequestId.incrementAndGet(), Protocol.Commands.GetUsersOnlineStatus, strs);
        ResultVerifier.getInstance().addExpectation(message.getId(), expectation);
        write(message);

    }


    @Override
    protected boolean onResult(Message msg)
    {
        ResultVerifier.getInstance().onResult(msg);
        return true;
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelConnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onSendRegister()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelDisconnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

}
