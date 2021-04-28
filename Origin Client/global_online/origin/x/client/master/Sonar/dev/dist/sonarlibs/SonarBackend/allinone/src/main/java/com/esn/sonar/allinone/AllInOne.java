package com.esn.sonar.allinone;

import com.esn.sonar.core.StatsManager;
import com.esn.sonar.core.util.Utils;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.MasterStats;
import com.esn.sonar.useredge.UserEdgeServer;
import com.esn.sonar.useredge.UserEdgeStats;
import com.esn.sonar.voiceedge.VoiceEdgeServer;
import com.esn.sonar.voiceedge.VoiceEdgeStats;
import org.jboss.netty.channel.socket.DatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioDatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;

import java.util.concurrent.Executor;

import static java.util.concurrent.Executors.newCachedThreadPool;

public class AllInOne
{
    private final NioServerSocketChannelFactory serverSocketFactory;
    private MasterServer masterServer;
    private UserEdgeServer userEdgeServer;
    private VoiceEdgeServer voiceEdgeServer;
    private final NioClientSocketChannelFactory clientSocketFactory;
    private final AllInOneConfig config;
    private final DatagramChannelFactory datagramChannelFactory;

    public AllInOne(AllInOneConfig config, NioServerSocketChannelFactory serverSocketFactory, NioClientSocketChannelFactory clientSocketFactory, DatagramChannelFactory datagramChannelFactory)
    {
        this.config = config;
        this.serverSocketFactory = serverSocketFactory;
        this.clientSocketFactory = clientSocketFactory;
        this.datagramChannelFactory = datagramChannelFactory;
    }

    public void start() throws Exception
    {
        masterServer = new MasterServer(config.masterConfig, serverSocketFactory);
        userEdgeServer = new UserEdgeServer(config.userEdgeConfig, serverSocketFactory, clientSocketFactory, true);
        voiceEdgeServer = new VoiceEdgeServer(config.voiceEdgeConfig, serverSocketFactory, clientSocketFactory, datagramChannelFactory);

        StatsManager[] statsManagers = createStatsManagers();

        masterServer.start(statsManagers);
        voiceEdgeServer.start(statsManagers);
        userEdgeServer.start(statsManagers);
    }

    private StatsManager[] createStatsManagers()
    {
        return new StatsManager[]{
                MasterStats.getStatsManager(),
                UserEdgeStats.getStatsManager(),
                VoiceEdgeStats.getStatsManager()
        };
    }

    public void stop() throws InterruptedException
    {
        userEdgeServer.stop();
        voiceEdgeServer.stop();
        masterServer.stop();
    }


    public static void main(String[] args) throws Exception
    {
        Executor workerExecutor = newCachedThreadPool();
        Executor bossExecutor = newCachedThreadPool();
        NioServerSocketChannelFactory serverSocketFactory = new NioServerSocketChannelFactory(bossExecutor, workerExecutor);
        NioClientSocketChannelFactory clientSocketFactory = new NioClientSocketChannelFactory(bossExecutor, workerExecutor);
        NioDatagramChannelFactory datagramChannelFactory = new NioDatagramChannelFactory(workerExecutor);
        Utils.create();

        new AllInOne(new AllInOneConfig(), serverSocketFactory, clientSocketFactory, datagramChannelFactory).start();
    }

    public MasterServer getMasterServer()
    {
        return masterServer;
    }
}
