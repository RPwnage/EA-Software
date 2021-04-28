package com.esn.sonar.master;

import com.esn.geoip.GeoLiteCity;
import com.esn.geoip.GeoipProvider;
import com.esn.geoip.Position;
import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.StatisticsHandler;
import com.esn.sonar.core.StatsManager;
import com.esn.sonar.core.util.Ref;
import com.esn.sonar.core.util.Utils;
import com.esn.sonar.master.api.event.EventServicePipelineFactory;
import com.esn.sonar.master.api.operator.OperatorService;
import com.esn.sonar.master.api.operator.OperatorServiceImpl;
import com.esn.sonar.master.api.operator.OperatorServicePipelineFactory;
import com.esn.sonar.master.api.rest.HttpPipelineFactory;
import com.esn.sonar.master.api.rest.RestApi;
import com.esn.sonar.master.user.UserManager;
import com.esn.sonar.master.voice.VoiceEdgeConnection;
import com.esn.sonar.master.voice.VoiceManager;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.io.File;
import java.io.IOException;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import java.util.Collection;
import java.util.concurrent.Executor;

import static com.esn.sonar.core.util.Utils.bindSocket;
import static java.util.concurrent.Executors.newCachedThreadPool;

public class MasterServer
{
    static final public Logger channelLog = Logger.getLogger("Channel");
    static final public Logger userLog = Logger.getLogger("User");
    static final public Logger userEdgeLog = Logger.getLogger("UserEdge");
    static final public Logger voiceEdgeLog = Logger.getLogger("VoiceEdge");
    static final public Logger masterLog = Logger.getLogger("Master");

    private OperatorServiceImpl operatorService;
    private Channel voiceServiceSocket;
    private Channel operatorServiceSocket;
    private Channel httpServiceSocket;
    private Channel userServiceSocket;
    private Channel eventServiceSocket;

    private final MasterConfig config;
    private final NioServerSocketChannelFactory serverSocketFactory;
    private OperatorManager operatorManager;
    private KeyPair keyPair;
    private static GeoipProvider geoip;
    private static final Ref<MasterServer> instance = new Ref<MasterServer>();
    private GeoipDataUpdateTask geoipUpdateTask = new GeoipDataUpdateTask();
    private VoiceManager voiceManager;
    private UserManager userManager;
    private OrphanTask orphanTask;
    private DebugModule debugModule;

    public MasterServer(MasterConfig config, NioServerSocketChannelFactory serverSocketFactory) throws NoSuchAlgorithmException, InvalidKeySpecException
    {
        if (!instance.isNull() && !Utils.isUnitTestMode())
        {
            throw new IllegalStateException("There can be only one MasterServer");
        }
        instance.set(this);
        this.config = config;
        this.serverSocketFactory = serverSocketFactory;
        this.keyPair = Utils.loadOrGenerateKeyPair(System.getProperty("user.home"), "sonarmaster.pub", "sonarmaster.prv");

        channelLog.setLevel(Level.toLevel(config.getLogLevel()));
        userLog.setLevel(Level.toLevel(config.getLogLevel()));
        userEdgeLog.setLevel(Level.toLevel(config.getLogLevel()));
        voiceEdgeLog.setLevel(Level.toLevel(config.getLogLevel()));
        masterLog.setLevel(Level.toLevel(config.getLogLevel()));
    }


    public VoiceManager getVoiceManager()
    {
        return voiceManager;
    }

    public UserManager getUserManager()
    {
        return userManager;
    }

    private GeoipProvider geoipProviderFactory()
    {
        if (config.getGeoipProvider().toLowerCase().equals("geolitecity"))
        {
            return new GeoLiteCity();
        } else
        {
            return new GeoipProvider()
            {
                public File[] getFiles(String basePath)
                {
                    return new File[0];
                }

                public void loadFromJar(Class cls, String basePath) throws IOException
                {
                }

                public void loadFromDisk(String basePath) throws IOException
                {
                }

                public Position getPosition(String address)
                {
                    return new Position()
                    {
                        public float getLatitude()
                        {
                            return 0.0f;
                        }

                        public float getLongitude()
                        {
                            return 0.0f;
                        }

                        public String getAddress()
                        {
                            return "Dummy-Position";
                        }
                    };
                }

                public String getLicense()
                {
                    return "Dummy GeoIP provider in use.";
                }
            };
        }

    }

    public void start(StatsManager[] statsManagers) throws Exception
    {
        if (geoip == null)
        {
            geoip = geoipProviderFactory();

            System.err.printf("%s\n", geoip.getLicense());

            boolean success = false;

            try
            {
                success = geoipUpdateTask.poll();
            } catch (IOException e)
            {
            }

            if (!success)
            {
                geoip.loadFromJar(this.getClass(), "/geoip");
                masterLog.info("Loaded GeoIP data from JAR");
            }
        }


        Maintenance.getInstance().scheduleTask(GeoipDataUpdateTask.intervalSec, geoipUpdateTask);

        operatorManager = new OperatorManager();
        voiceManager = new VoiceManager(operatorManager);
        userManager = new UserManager(operatorManager, config);
        operatorService = new OperatorServiceImpl(operatorManager, voiceManager, userManager, config);

        this.orphanTask = new OrphanTask(this, config.getOrphanTaskInterval(), config.getMaxOrphanCount());

        userManager.setOperatorService(operatorService);

        if (config.getVoiceBindAddress().getPort() != 0)
        {
            masterLog.info(String.format("Enabling voice service on %s (TCP)", config.getVoiceBindAddress()));
            voiceServiceSocket = bindSocket(new com.esn.sonar.master.voice.PipelineFactory(voiceManager), config.getVoiceBindAddress(), serverSocketFactory);
        }

        if (config.getOperatorServiceBindAddress().getPort() != 0)
        {
            masterLog.info(String.format("Enabling operator service on %s (TCP)", config.getOperatorServiceBindAddress()));
            operatorServiceSocket = bindSocket(new OperatorServicePipelineFactory(operatorService), config.getOperatorServiceBindAddress(), serverSocketFactory);
        }

        RestApi.setOperatorService(operatorService);

        if (config.getHttpServiceBindAddress().getPort() != 0)
        {
            masterLog.info(String.format("Enabling http service on %s (TCP)", config.getHttpServiceBindAddress()));
            httpServiceSocket = bindSocket(new HttpPipelineFactory(new StatisticsHandler(statsManagers)), config.getHttpServiceBindAddress(), serverSocketFactory);
        }

        if (config.getUserBindAddress().getPort() != 0)
        {
            masterLog.info(String.format("Enabling user service on %s (TCP)", config.getUserBindAddress()));
            userServiceSocket = bindSocket(new com.esn.sonar.master.user.PipelineFactory(userManager), config.getUserBindAddress(), serverSocketFactory);
        }

        if (config.getEventServiceBindAddress().getPort() != 0)
        {
            masterLog.info(String.format("Enabling event service on %s (TCP)", config.getEventServiceBindAddress()));
            eventServiceSocket = bindSocket(new EventServicePipelineFactory(), config.getEventServiceBindAddress(), serverSocketFactory);
        }


        if (!config.getDebugOperatorId().equals(""))
        {
            this.debugModule = new DebugModule(config);
            debugModule.start();
        }


    }

    public void stop() throws InterruptedException
    {
        if (voiceServiceSocket != null)
            voiceServiceSocket.close().awaitUninterruptibly();

        if (operatorServiceSocket != null)
            operatorServiceSocket.close().awaitUninterruptibly();

        if (httpServiceSocket != null)
            httpServiceSocket.close().awaitUninterruptibly();

        if (userServiceSocket != null)
            userServiceSocket.close().awaitUninterruptibly();

        if (eventServiceSocket != null)
            eventServiceSocket.close().awaitUninterruptibly();


        if (debugModule != null)
        {
            debugModule.stop();
        }

        orphanTask.cancel();
    }

    private static StatsManager[] createDefaultStatsManagers()
    {
        return new StatsManager[]{MasterStats.getStatsManager()};
    }

    public static void main(String[] args) throws Exception
    {
        Utils.create();

        masterLog.info("ESN Sonar Master server");
        Executor workerExecutor = newCachedThreadPool();
        Executor bossExecutor = newCachedThreadPool();
        NioServerSocketChannelFactory serverSocketFactory = new NioServerSocketChannelFactory(bossExecutor, workerExecutor);

        new MasterServer(new MasterConfig(System.getProperty("sonar.config")), serverSocketFactory).start(createDefaultStatsManagers());

    }

    public OperatorService getOperatorService()
    {
        return operatorService;
    }

    public OperatorManager getOperatorManager()
    {
        return operatorManager;
    }

    public static MasterServer getInstance()
    {
        if (instance.isNull())
        {
            throw new IllegalStateException("No MasterServer instance registered");
        }
        return instance.get();
    }

    public KeyPair getKeyPair()
    {
        return keyPair;
    }

    public static GeoipProvider getGeoipProvider()
    {
        return geoip;
    }

    public void flushChannels()
    {
        Collection<Operator> operators = operatorManager.getOperators();
        for (Operator operator : operators)
        {
            operator.getCache().clear();
            operator.getChannelManager().clear();
        }
    }

    private class GeoipDataUpdateTask implements TimerTask
    {
        private long lastModified = -1;

        static public final int intervalSec = 3600;

        public boolean poll() throws IOException
        {
            MasterConfig config = MasterServer.this.config;
            GeoipProvider provider = geoip;

            File[] newFiles = provider.getFiles(config.getGeoipPath());

            long newestModified = -1;

            for (int index = 0; index < newFiles.length; index++)
            {
                masterLog.info(String.format("Scanning %s...", newFiles[index].getCanonicalPath()));

                if (!newFiles[index].exists())
                {
                    return false;
                }

                long modifTime = newFiles[index].lastModified();

                if (modifTime > newestModified)
                {
                    newestModified = modifTime;
                }
            }

            if (newestModified <= lastModified)
            {
                return false;
            }

            this.lastModified = newestModified;

            GeoipProvider newProvider = geoipProviderFactory();
            newProvider.loadFromDisk(config.getGeoipPath());

            System.err.printf("Loaded new GeoIP provider from disk!\n");

            MasterServer.geoip = newProvider;
            return true;
        }

        public void run(Timeout timeout)
        {
            try
            {
                poll();
            } catch (Throwable e)
            {
            } finally
            {
                Maintenance.getInstance().scheduleTask(intervalSec, this);
            }
        }
    }
}

