package com.esn.sonar.useredge;

import com.esn.sonar.core.ConnectionManager;
import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Ref;
import com.esn.sonar.core.util.Utils;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.socket.ClientSocketChannelFactory;

import java.security.NoSuchAlgorithmException;
import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class UserEdgeManager extends ConnectionManager
{
    private static final Ref<UserEdgeManager> instance = new Ref<UserEdgeManager>();

    private final UserEdgeConfig config;
    private final MasterConnection master;
    private final ReentrantReadWriteLock rwl = new ReentrantReadWriteLock();

    public int getSphereCount()
    {
        rwl.readLock().lock();

        try
        {
            return sphereMap.size();

        } finally
        {
            rwl.readLock().unlock();
        }
    }

    private static class Sphere
    {
        private final ConcurrentHashMap<String, UserConnection> userMap = new ConcurrentHashMap<String, UserConnection>();

        public Sphere()
        {
        }

        public UserConnection addUser(UserConnection connection)
        {
            return userMap.put(connection.getId(), connection);

        }

        public void removeUser(UserConnection connection)
        {
            userMap.remove(connection.getId(), connection);
        }

        public UserConnection getUser(String userId)
        {
            return userMap.get(userId);
        }

        public int getUserCount()
        {
            return userMap.size();
        }

        public ConcurrentHashMap<String, UserConnection> getUsers()
        {
            return userMap;
        }
    }

    final HashMap<String, Sphere> sphereMap = new HashMap<String, Sphere>();

    synchronized public int getClientCount()
    {
        int count = 0;

        for (Sphere sphere : sphereMap.values())
        {
            count += sphere.getUserCount();
        }

        return count;
    }


    public UserEdgeManager(UserEdgeConfig config, ClientSocketChannelFactory factory) throws NoSuchAlgorithmException
    {
        super();
        if (!instance.isNull() && !Utils.isUnitTestMode())
        {
            throw new IllegalStateException("There can be only one UserEdgeManager");
        }
        instance.set(this);
        this.config = config;
        this.master = new MasterConnection(new ClientBootstrap(factory), config.getMasterAddress(), config);
        this.master.connect();
    }

    public void shutdown()
    {
        master.close();
    }

    public Sphere getSphere(String operatorId)
    {
        rwl.readLock().lock();

        Sphere sphere;
        try
        {
            sphere = sphereMap.get(operatorId);

            if (sphere == null)
            {
                // upgrade lock manually
                rwl.readLock().unlock();   // must unlock first to obtain writelock
                rwl.writeLock().lock();

                try
                {
                    sphere = sphereMap.get(operatorId);

                    if (sphere == null)
                    {
                        // Can't throw unless out of memory
                        sphere = new Sphere();

                        // Can't throw unless out of memory
                        sphereMap.put(operatorId, sphere);
                    }
                } finally
                {
                    // downgrade lock
                    rwl.readLock().lock();  // reacquire read without giving up write lock
                    rwl.writeLock().unlock(); // unlock write, still hold read
                }
            }
        } finally
        {
            rwl.readLock().unlock();
        }


        return sphere;
    }


    @Override
    public void registerClient(InboundConnection client)
    {
        UserConnection connection = (UserConnection) client;

        Sphere sphere = getSphere(connection.getOperatorId());

        UserConnection oldConnection = sphere.addUser(connection);

        if (oldConnection != null)
        {
            //TODO: Technically, if this happens, do we really need to send the EdgeUserRegistered event?
            oldConnection.sendUnregisterAndClose(Protocol.Reasons.LoggedInElsewhere, "");
        }

        master.sendEdgeUserRegistered(connection.getOperatorId(), connection.getId(), connection.getUserDescription(), connection.getRemoteAddress().getAddress().toString().substring(1));
        connection.setRegistered(true);

        if (UserEdgeServer.userEdgeLog.isTraceEnabled()) UserEdgeServer.userEdgeLog.trace(String.format(
                "User client %s registered from %s", client, client.getRemoteAddress().toString()));

        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.UserConnect).incrementAndGet();
    }

    @Override
    public void unregisterClient(InboundConnection client, String reasonType, String reasonDesc)
    {
        UserConnection connection = (UserConnection) client;

        Sphere sphere = getSphere(connection.getOperatorId());

        sphere.removeUser(connection);
        master.sendEdgeUserUnregistered(connection.getOperatorId(), connection.getId(), connection.getUserDescription(), reasonType, reasonDesc);

        connection.setRegistered(false);

        if (UserEdgeServer.userEdgeLog.isTraceEnabled()) UserEdgeServer.userEdgeLog.trace(String.format(
                "User client %s unregistered", client));

        UserEdgeStats.getStatsManager().metric(UserEdgeMetrics.UserDisconnect).incrementAndGet();
    }

    public void disconnectClient(String operatorId, String userId, String reasonType, String reasonDesc)
    {
        Sphere sphere = getSphere(operatorId);
        UserConnection connection = sphere.getUser(userId);

        if (connection == null)
            return;

        connection.sendUnregisterAndClose(reasonType, reasonDesc);
        unregisterClient(connection, reasonType, reasonDesc);
    }


    public static UserEdgeManager getInstance()
    {
        if (instance.isNull())
        {
            throw new IllegalStateException("No UserEdgeManager instance registered");
        }
        return instance.get();
    }

    public UserEdgeConfig getConfig()
    {
        return config;
    }

    public void sendUpdateToken(String operatorId, String userId, Token token, String type)
    {
        Sphere sphere = getSphere(operatorId);
        UserConnection connection = sphere.getUser(userId);

        if (connection == null)
        {
            return;
        }

        connection.updateToken(token, type);
    }

    public void waitForMasterRegistration() throws InterruptedException
    {
        while (master.isRegistered() == false)
        {
            Thread.sleep(100);
        }
    }

    public void waitForMasterClose() throws InterruptedException
    {
        while (master.isRegistered() == true)
        {
            Thread.sleep(100);
        }
    }


    //TODO: Write tests for this!
    synchronized public void dropAllClients()
    {
        rwl.readLock().lock();

        try
        {

            for (Sphere sphere : sphereMap.values())
            {
                Collection<UserConnection> users = sphere.getUsers().values();

                for (UserConnection user : users)
                {
                    user.sendUnregisterAndClose(Protocol.Reasons.TryAgain, "Connection to master lost");
                }

                sphere.getUsers().clear();
            }

        } finally
        {
            rwl.readLock().unlock();
        }
    }


}
