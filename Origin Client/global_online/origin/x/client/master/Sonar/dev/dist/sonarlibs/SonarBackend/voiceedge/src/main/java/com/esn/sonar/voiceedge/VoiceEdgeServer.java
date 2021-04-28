package com.esn.sonar.voiceedge;

import com.esn.sonar.core.HttpStatisticsPipeline;
import com.esn.sonar.core.StatsManager;
import com.esn.sonar.core.util.Ref;
import com.esn.sonar.core.util.Utils;
import org.apache.log4j.Logger;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.socket.DatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioDatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;

import java.net.BindException;
import java.util.concurrent.Executor;

import static com.esn.sonar.core.util.Utils.bindSocket;
import static java.util.concurrent.Executors.newCachedThreadPool;

public class VoiceEdgeServer
{
    private final VoiceEdgeConfig config;
    private Channel socket;
    private final NioServerSocketChannelFactory serverSocketFactory;
    private final NioClientSocketChannelFactory clientSocketFactory;
    private final DatagramChannelFactory datagramChannelFactory;
    static final public Logger voiceEdgeLog = Logger.getLogger("VoiceEdgeSrv");
    private Channel httpSocket;
    private static final Ref<VoiceEdgeServer> instance = new Ref<VoiceEdgeServer>();
    private VoiceEdgeManager voiceEdgeManager;

    public VoiceEdgeServer(VoiceEdgeConfig config, NioServerSocketChannelFactory serverSocketFactory, NioClientSocketChannelFactory clientSocketFactory, DatagramChannelFactory datagramChannelFactory)
    {
        if (!instance.isNull() && !Utils.isUnitTestMode())
        {
            throw new IllegalStateException("There can be only one VoiceEdgeServer");
        }
        instance.set(this);
        this.config = config;
        this.serverSocketFactory = serverSocketFactory;
        this.clientSocketFactory = clientSocketFactory;
        this.datagramChannelFactory = datagramChannelFactory;
    }

    public static VoiceEdgeServer getInstance()
    {
        if (instance.isNull())
        {
            throw new IllegalStateException("No VoiceEdgeServer instance registered");
        }
        return instance.get();
    }

    public VoiceEdgeManager getVoiceEdgeManager()
    {
        return voiceEdgeManager;
    }

    public void start(StatsManager[] statsManagers) throws Exception
    {
        voiceEdgeManager = new VoiceEdgeManager(config, clientSocketFactory, datagramChannelFactory);


        voiceEdgeLog.info(String.format("Voice Edge service on %s (TCP) and %s (UDP). Looking for master at %s", config.getBindAddress(), config.getChallengeBindAddress(), config.getMasterAddress()));
        socket = bindSocket(new PipelineFactory(config), config.getBindAddress(), serverSocketFactory);

        voiceEdgeLog.info(String.format("Voice Edge HTTP monitor service on %s", config.getHttpAddress()));
        try
        {
            httpSocket = bindSocket(new HttpStatisticsPipeline(statsManagers), config.getHttpAddress(), serverSocketFactory);
        } catch (BindException e)
        {
            voiceEdgeLog.warn(String.format("Failed to open HTTP monitor service on %s, service will be unavailable", config.getHttpAddress()));
        }


        voiceEdgeManager.waitForMasterConnection();
    }

    public void stop()
    {
        VoiceEdgeManager.getInstance().shutdown();
        socket.close().awaitUninterruptibly();

        if (httpSocket != null)
        {
            httpSocket.close().awaitUninterruptibly();
        }
    }

    private static StatsManager[] createDefaultStatsManagers()
    {
        return new StatsManager[]{VoiceEdgeStats.getStatsManager()};
    }

    public static void main(String[] args) throws Exception
    {
        voiceEdgeLog.info(String.format("ESN Sonar Voice Edge server"));

        Utils.create();
        Executor workerExecutor = newCachedThreadPool();
        Executor bossExecutor = newCachedThreadPool();

        NioServerSocketChannelFactory serverSocketFactory = new NioServerSocketChannelFactory(bossExecutor, workerExecutor);
        NioClientSocketChannelFactory clientSocketFactory = new NioClientSocketChannelFactory(bossExecutor, workerExecutor);
        NioDatagramChannelFactory datagramChannelFactory = new NioDatagramChannelFactory(workerExecutor);

        new VoiceEdgeServer(new VoiceEdgeConfig(System.getProperty("sonar.config")), serverSocketFactory, clientSocketFactory, datagramChannelFactory).start(createDefaultStatsManagers());
    }
}
