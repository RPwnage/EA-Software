package com.esn.sonartest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonartest.MasterTest.MasterTestConfig;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;

public class UserEdgeClient extends OutboundConnection
{
    private final ConcurrentHashMap<String, User> userMap = new ConcurrentHashMap<String, User>();
    private final CountDownLatch stateLatch = new CountDownLatch(1);
    private long msgId;
    private int portId;

    public UserEdgeClient(MasterTestConfig config, ClientBootstrap bootstrap, int portId)
    {
        super(-1, 60, true, bootstrap, config.getUserEdgeAddress());

        this.portId = portId;
    }

    @Override
    protected void onChannelConnected()
    {
    }

    @Override
    protected void onSendRegister()
    {
        /*
        [publicIP] [publicPort]
         */

        write(new Message(getMessageId(), Protocol.Commands.Register, "127.0.0.1", Integer.toString(portId)));
    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        if (msg.getName().equals(Protocol.Commands.EdgeUpdateToken))
        {
            String operatorId = msg.getArgument(0);
            String userId = msg.getArgument(1);
            Token newToken = new Token(msg.getArgument(2), null, -1);
            String type = msg.getArgument(3);

            User user = userMap.get(userId);

            if (!type.equals("JOIN"))
            {
                return true;
            }

            if (user == null)
            {
                throw new RuntimeException("UserId " + userId + " not found on server");
            }


            user.updateToken(newToken, type);
            return true;
        } else if (msg.getName().equals(Protocol.Commands.EdgeUpdateKeys))
        {
            return true;
        } else if (msg.getName().equals(Protocol.Commands.EdgeUnregisterClient))
        {
            String operatorId = msg.getArgument(0);
            String userId = msg.getArgument(1);
            String reasonType = msg.getArgument(2);
            String reasonDesc = msg.getArgument(3);

            User user = userMap.get(userId);

            disconnectUser(user);

            return true;
        }

        return true;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected boolean onResult(Message msg)
    {
        if (msgId == msg.getId())
        {
            stateLatch.countDown();
        }

        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        throw new RuntimeException();
    }

    @Override
    protected void onRegisterSuccess(Message msg)
    {
        for (User user : userMap.values())
        {
        /*
        [operatorId] [userId] [userDesc] [remoteAddress]
         */

            evtEdgeUserRegistered(user);
        }

        msgId = getMessageId();
        write(new Message(msgId, Protocol.Commands.Keepalive));
    }

    public void waitForStateSync() throws InterruptedException
    {
        stateLatch.await();
    }

    public void disconnectUser(User user)
    {
        /*
        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String userDesc = msg.getArgument(2);
        String reasonType = msg.getArgument(3);
        String reasonDesc = msg.getArgument(4);
        */

        write(new Message(getMessageId(), Protocol.Commands.EdgeUserUnregistered, MasterTestDirector.operatorId, user.getUserId(), user.getUserDesc(), "UNKNOWN", ""));

        userMap.remove(user.getUserId());
        user.setEdgeClient(null);
    }

    public void addUser(User user)
    {
        User currUser = userMap.put(user.getUserId(), user);

        if (currUser != null)
        {
            throw new RuntimeException();
        }

        user.setEdgeClient(this);
    }

    public void connectUser(User user)
    {
        addUser(user);
        evtEdgeUserRegistered(user);
    }

    private void evtEdgeUserRegistered(User user)
    {
        write(new Message(getMessageId(), Protocol.Commands.EdgeUserRegistered,
                MasterTestDirector.operatorId,
                user.getUserId(),
                user.getUserDesc(),
                user.getRemoteAddress()));
    }

    public int getPortId()
    {
        return portId;
    }

    public Collection<User> getUsers()
    {
        return userMap.values();
    }
}
