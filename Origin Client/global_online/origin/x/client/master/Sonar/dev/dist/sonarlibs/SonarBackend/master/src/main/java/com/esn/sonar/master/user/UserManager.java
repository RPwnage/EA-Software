package com.esn.sonar.master.user;


import com.esn.sonar.core.ConnectionManager;
import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.MasterConfig;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.Operator;
import com.esn.sonar.master.OperatorManager;
import com.esn.sonar.master.api.operator.OperatorService;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class UserManager extends ConnectionManager
{
    private final OperatorManager operatorManager;
    private final ReentrantReadWriteLock rwl = new ReentrantReadWriteLock();
    private final AtomicInteger rrCounter = new AtomicInteger();
    private final ArrayList<UserEdgeConnection> clientList = new ArrayList<UserEdgeConnection>();

    private OperatorService operatorService;
    private final MasterConfig config;

    public UserManager(OperatorManager operatorManager, MasterConfig config) throws NoSuchAlgorithmException
    {
        super();
        this.operatorManager = operatorManager;
        this.config = config;
    }

    public void setOperatorService(OperatorService operatorService)
    {
        this.operatorService = operatorService;
    }

    private final ConcurrentHashMap<String, JoinChannelFuture> joinChannelMap = new ConcurrentHashMap<String, JoinChannelFuture>();

    public int getJoinChannelFutureCount()
    {
        return joinChannelMap.size();
    }

    //TODO: This doesn't work with operators! Move this management to the Operator class
    public void handleFutureJoin(User user) throws OperatorService.UserNotFoundException, OperatorService.ChannelAllocationFailed, OperatorService.InvalidArgumentException
    {
        JoinChannelFuture joinChannelFuture = joinChannelMap.remove(user.getUserId());

        if (joinChannelFuture == null)
        {
            return;
        }

        joinChannelFuture.unschedule();


        if (MasterServer.userLog.isTraceEnabled()) MasterServer.userLog.trace(String.format(
                "Handling future join %s", joinChannelFuture));

        operatorService.joinUserToChannel(joinChannelFuture.getOperatorId(), joinChannelFuture.getLocation(), user.getUserId(), joinChannelFuture.getChannelId(), joinChannelFuture.getChannelDescription());
    }

    public void setupJoinFuture(String sphereId, String userId, String channelId, String channelDesc, String location)
    {
        JoinChannelFuture joinChannelFuture = new JoinChannelFuture(sphereId, userId, channelId, channelDesc, location);
        joinChannelFuture.schedule();
        joinChannelMap.put(userId, joinChannelFuture);
    }

    private class JoinChannelFuture implements TimerTask
    {
        private Timeout timeout;
        private final String userId;
        private final String channelId;
        private final String channelDesc;
        private final String location;
        private final String operatorId;

        public JoinChannelFuture(String operatorId, String userId, String channelId, String channelDesc, String location)
        {
            this.userId = userId;
            this.channelId = channelId;
            this.channelDesc = channelDesc;
            this.location = location;
            this.operatorId = operatorId;
        }

        public String getChannelDescription()
        {
            return channelDesc;
        }

        public String getChannelId()
        {
            return channelId;
        }

        public String getLocation()
        {
            return location;
        }

        public String getUserId()
        {
            return userId;
        }

        public String getOperatorId()
        {
            return operatorId;
        }

        public void run(Timeout timeout) throws Exception
        {
            if (MasterServer.userLog.isTraceEnabled()) MasterServer.userLog.trace(String.format(
                    "Expiring future join %s", this));

            joinChannelMap.remove(userId);
        }

        public void schedule()
        {
            timeout = Maintenance.getInstance().scheduleTask(config.getJoinChannelExpire(), this);
        }

        public void unschedule()
        {
            if (timeout != null)
            {
                timeout.cancel();
            }
        }

        @Override
        public String toString()
        {
            return operatorId + "|" + userId + "|" + channelId;
        }
    }


    @Override
    public void registerClient(InboundConnection client)
    {
        rwl.writeLock().lock();

        try
        {
            client.setRegistered(true);

            MasterServer.userEdgeLog.info(String.format("User Edge server %s registered", client));

            clientList.add((UserEdgeConnection) client);
        } finally
        {
            rwl.writeLock().unlock();
        }
    }

    @Override
    public void unregisterClient(InboundConnection client, String reasonType, String reasonDesc)
    {
        rwl.writeLock().lock();

        try
        {
            if (clientList.remove(client))
            {
                MasterServer.userEdgeLog.info(String.format("User edge server %s unregistered", client));
            } else
            {
                MasterServer.userEdgeLog.error(String.format("Failed to remove edge server %s. Not found!", client));
            }


        } finally
        {
            rwl.writeLock().unlock();
        }
    }

    public synchronized UserEdgeConnection getRandomClient()
    {
        rwl.readLock().lock();

        if (clientList.size() == 0)
        {
            rwl.readLock().unlock();
            return null;
        }

        try
        {
            return clientList.get(rrCounter.getAndIncrement() % clientList.size());
        } finally
        {
            rwl.readLock().unlock();
        }

    }

    public User mapClientToServer(String operatorId, String userId, String userDescription, String remoteAddress, UserEdgeConnection edgeConnection)
    {
        Operator sphere = operatorManager.getOperator(operatorId);

        User user = new User(sphere.getId(), userId, userDescription, remoteAddress, edgeConnection);
        User oldUser = sphere.getUserMap().put(userId, user);

        /*
        If the user is a returning user and he's returning from the same edge connection, let's not send a
        LoggedInElsewhere request back to his edge, his edge won't be able to tell the two connections apart and
        will diconnect the new one 
        */

        if (oldUser != null && oldUser.getConnection() != user.getConnection())
        {
            oldUser.sendUnregisterClient(Protocol.Reasons.LoggedInElsewhere, "");

            if (MasterServer.userLog.isInfoEnabled()) MasterServer.userLog.info(String.format(
                    "User %s connected from edge %s. Replacing %s", user, edgeConnection, oldUser));
        } else
        {
            if (MasterServer.userLog.isInfoEnabled()) MasterServer.userLog.info(String.format(
                    "User %s connected from edge %s. ", user, edgeConnection));
        }

        try
        {
            handleFutureJoin(user);
        } catch (OperatorService.OperatorException e)
        {
            MasterServer.userLog.warn(String.format("Future join exception for user %s", user), e);
        }

        sphere.getEventPublisher().publishEvent(
                new OperatorService.UserConnectedEvent(
                        user.getOperatorId(), user.getUserId(), user.getUserDesc()));

        return user;
    }

    public User unmapClientToServer(String operatorId, String userId, String userDesc, String reasonType, String reasonDesc)
    {
        Operator sphere = operatorManager.getOperator(operatorId);

        User user = sphere.getUserMap().remove(userId);

        if (user == null)
        {
            MasterServer.userLog.warn(String.format(
                    "User %s|%s not found when unmapping. ", operatorId, userId));

        } else
        {
            if (MasterServer.userLog.isInfoEnabled()) MasterServer.userLog.info(String.format(
                    "User %s disconnected from edge %s. %s/%s", user, user.getConnection(), reasonType, reasonDesc));
        }

        sphere.getEventPublisher().publishEvent(
                new OperatorService.UserDisconnectedEvent(
                        operatorId,
                        userId,
                        userDesc));


        return user;
    }
}
