package com.esn.sonar.useredge;

import com.esn.sonar.core.HttpStatisticsPipeline;
import com.esn.sonar.core.StatsManager;
import com.esn.sonar.core.util.Ref;
import com.esn.sonar.core.util.Utils;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.socket.ClientSocketChannelFactory;
import org.jboss.netty.channel.socket.ServerSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;

import java.net.BindException;
import java.util.concurrent.Executor;

import static com.esn.sonar.core.util.Utils.bindSocket;
import static java.util.concurrent.Executors.newCachedThreadPool;

public class UserEdgeServer
{
    private final UserEdgeConfig config;
    private Channel socket;
    private Channel httpSocket;
    private final ServerSocketChannelFactory serverSocketFactory;
    private final ClientSocketChannelFactory clientFactory;
    private boolean waitForMasterConnection;
    static final public Logger userEdgeLog = Logger.getLogger("UserEdgeSrv");
    private static final Ref<UserEdgeServer> instance = new Ref<UserEdgeServer>();
    private UserEdgeManager userEdgeManager;


    public UserEdgeServer(UserEdgeConfig config, ServerSocketChannelFactory serverSocketFactory, ClientSocketChannelFactory clientFactory, boolean waitForMasterConnection)
    {
        if (!instance.isNull() && !Utils.isUnitTestMode())
        {
            throw new IllegalStateException("There can be only one UserEdgeServer");
        }
        instance.set(this);
        this.config = config;
        this.serverSocketFactory = serverSocketFactory;
        this.clientFactory = clientFactory;
        this.waitForMasterConnection = waitForMasterConnection;

        userEdgeLog.setLevel(Level.toLevel(config.getLogLevel()));
    }

    public static UserEdgeServer getInstance()
    {
        if (instance.isNull())
        {
            throw new IllegalStateException("No UserEdgeServer instance registered");
        }
        return instance.get();
    }

    public UserEdgeManager getUserEdgeManager()
    {
        return userEdgeManager;
    }

    public void start(StatsManager[] statsManagers) throws Exception
    {
        userEdgeLog.info(String.format("User Edge service on %s (TCP). Looking for master at %s", config.getBindAddress(), config.getMasterAddress()));
        socket = bindSocket(new PipelineFactory(config), config.getBindAddress(), serverSocketFactory);

        userEdgeLog.info(String.format("User Edge HTTP monitor service on %s", config.getHttpAddress()));
        try
        {
            httpSocket = bindSocket(new HttpStatisticsPipeline(statsManagers), config.getHttpAddress(), serverSocketFactory);
        } catch (BindException e)
        {
            userEdgeLog.warn(String.format("Failed to open HTTP monitor service on %s, service will be unavailable", config.getHttpAddress()));
        }


        userEdgeManager = new UserEdgeManager(config, clientFactory);


        if (waitForMasterConnection)
        {
            userEdgeManager.waitForMasterRegistration();
        }
    }

    public void stop()
    {
        UserEdgeManager.getInstance().shutdown();
        socket.close().awaitUninterruptibly();

        if (httpSocket != null)
            httpSocket.close().awaitUninterruptibly();
    }

    private static StatsManager[] createDefaultStatsManagers()
    {
        return new StatsManager[]{UserEdgeStats.getStatsManager()};
    }

    public static void main(String[] args) throws Exception
    {
        userEdgeLog.info(String.format("ESN Sonar User Edge server"));

        Utils.create();
        Executor workerExecutor = newCachedThreadPool();
        Executor bossExecutor = newCachedThreadPool();

        NioServerSocketChannelFactory serverSocketFactory = new NioServerSocketChannelFactory(bossExecutor, workerExecutor);
        NioClientSocketChannelFactory clientSocketFactory = new NioClientSocketChannelFactory(bossExecutor, workerExecutor);


        new UserEdgeServer(new UserEdgeConfig(System.getProperty("sonar.config")), serverSocketFactory, clientSocketFactory, true).start(createDefaultStatsManagers());
    }
}
