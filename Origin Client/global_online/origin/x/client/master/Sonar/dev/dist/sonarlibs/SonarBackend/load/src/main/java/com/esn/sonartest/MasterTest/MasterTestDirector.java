package com.esn.sonartest.MasterTest;

import com.esn.geoip.GeoLiteCity;
import com.esn.geoip.util.LocationGenerator;
import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.util.Utils;
import com.esn.sonartest.*;
import com.esn.sonartest.MasterTest.jobs.*;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.Director;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.director.Pool;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.Expectation;
import com.esn.sonartest.verifier.ResultVerifier;
import com.esn.sonartest.verifier.TimeoutVerifier;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.socket.DatagramChannelFactory;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class MasterTestDirector extends Director
{
    private final Pool<User> connectedUserPool = new Pool<User>();
    private final Pool<User> usersInChannels = new Pool<User>();
    private Pool<User> disconnectedUserPool = new Pool<User>();
    private Pool<OperatorClient> operatorClientPool = new Pool<OperatorClient>();
    private final Pool<Channel> destroyedChannelPool = new Pool<Channel>();
    private final Pool<Channel> createdChannelPool = new Pool<Channel>();
    private final Map<Integer, UserEdgeClient> userEdgeClients = new ConcurrentHashMap<Integer, UserEdgeClient>();
    private final List<VoiceEdgeClient> voiceEdgeClients = new CopyOnWriteArrayList<VoiceEdgeClient>();
    private final List<EventClient> eventClients = new CopyOnWriteArrayList<EventClient>();

    protected final Map<String, VoiceServer> voiceServerMap = new ConcurrentHashMap<String, VoiceServer>();
    private LocationGenerator locationGenerator;
    public static String operatorId;

    protected final MasterTestConfig config;

    public MasterTestDirector(MasterTestConfig config) throws IOException, InterruptedException
    {
        Maintenance.configure(10);

        this.config = config;

        operatorId = config.getOperatorId();

        GeoLiteCity provider = new GeoLiteCity();
        provider.loadFromJar(this.getClass(), "/geoip");
        locationGenerator = new LocationGenerator(provider);

        TimeoutVerifier timeoutVerifier = new TimeoutVerifier(config.getVerificationTimeout());
        timeoutVerifier.start();
    }

    protected void onStart() throws Exception
    {
        System.err.printf("Creating %d users (%d -> %d): ", config.getUserEnd() - config.getUserStart(), config.getUserStart(), config.getUserEnd());

        for (int id = config.getUserStart(); id < config.getUserEnd(); id++)
        {
            String userId = String.format("UID-%08x", id);
            String userDesc = String.format("Description of %s", userId);
            User user = new User(userId, userDesc, this);
            disconnectedUserPool.add(user);
        }

        System.err.printf("DONE!\n");

        final List<Channel> channels = createChannels(config.getChannelStart(), config.getChannelEnd());
        for (Channel channel : channels)
        {
            destroyedChannelPool.add(channel);
        }

        System.err.printf("Connecting event clients: ");

        for (int index = 0; index < config.getEventClientCount(); index++)
        {
            EventClient eventClient = createEventClient("DOUSCHEBAGS");
            eventClient.connect();
            eventClient.waitForRegistration();
            eventClients.add(eventClient);

            System.err.printf(".");
        }

        System.err.printf("DONE!\n");

        System.err.printf("Connecting operator clients: ");

        for (int index = 0; index < config.getOperatorCount(); index++)
        {
            OperatorClient operatorClient = createOperatorClient();
            operatorClient.connect().awaitUninterruptibly();
            operatorClientPool.add(operatorClient);
            System.err.printf(".");
        }

        System.err.printf("DONE!\n");

        System.err.printf("Connecting user edge clients: ");

        int userEdgeCount = config.getUserEdgeCount();

        for (int iEdge = 0; iEdge < userEdgeCount; iEdge++)
        {
            int portId = 10000 + iEdge;
            UserEdgeClient userEdgeClient = createUserEdgeClient(config, getBootstrap(), portId);

            userEdgeClient.connect().awaitUninterruptibly();
            userEdgeClient.waitForStateSync();
            userEdgeClients.put(portId, userEdgeClient);
        }

        System.err.printf("DONE!\n");

        System.err.printf("Connecting voice edge clients: ");

        /** The total amount of voice servers */
        List<Integer> voiceServerSequence = Utils.createSequence(0, config.getVoiceServerCount());

        for (int veIndex = 0; veIndex < config.getVoiceEdgeCount(); veIndex++)
        {
            VoiceEdgeClient voiceEdgeClient = createVoiceEdgeClient(config, getBootstrap(), datagramChannelFactory);
            /** The range of voice servers for current VoiceEdge */
            List<Integer> voiceServerSubSequence = Utils.extractRange(voiceServerSequence, config.getVoiceEdgeCount(), veIndex);
            List<VoiceServer> voiceServerList = new ArrayList<VoiceServer>(voiceServerSubSequence.size());

            for (Integer x : voiceServerSubSequence)
            {
                VoiceServer voiceServer = new VoiceServer(datagramChannelFactory, getRandomIP(), config.getVoiceServerMaxClients(), voiceEdgeClient);
                voiceServerList.add(voiceServer);

                String voiceAddressAndPort = voiceServer.getPublicAddress() + ":" + voiceServer.getVoipPort();
                voiceServerMap.put(voiceAddressAndPort, voiceServer);
            }

            voiceEdgeClient.setVoiceServers(voiceServerList);
            voiceEdgeClient.connect().awaitUninterruptibly();
            voiceEdgeClient.waitForStateSync();
            voiceEdgeClients.add(voiceEdgeClient);
        }

        System.err.printf("DONE!\n");

        // Destroy all channels
        System.err.printf("Destroying old channels: ");

        final AtomicInteger channelsToDestroy = new AtomicInteger(channels.size());

        OperatorClient operatorClient = getOperatorClientPool().acquire();

        for (Channel channel : channels)
        {
            operatorClient.destroyChannel(MasterTestDirector.operatorId, channel.getChannelId(), "UNKNOWN", "", new Expectation("DestroyChannels(initial)")
            {
                @Override
                public void verify(Message message)
                {
                    channelsToDestroy.decrementAndGet();
                }
            });
        }

        getOperatorClientPool().release(operatorClient);

        while (channelsToDestroy.get() != 0)
        {
            Thread.sleep(100);
        }

        System.err.printf("DONE!\n");
        System.err.printf("Superstar DJ... Here we go!\n");
    }

    protected void onStop() throws Exception
    {
        while (AbstractJob.getJobsRunning() > 0)
        {
            Thread.sleep(100);
        }

        Thread.sleep(5000);

        while (EventVerifier.getInstance().size() > 0 && ResultVerifier.getInstance().size() > 0)
        {
            Thread.sleep(100);
        }

        for (EventClient eventClient : eventClients)
        {
            eventClient.close().awaitUninterruptibly();
        }

        for (UserEdgeClient userEdgeClient : userEdgeClients.values())
        {
            userEdgeClient.close().awaitUninterruptibly();
        }

        for (VoiceEdgeClient voiceEdgeClient : voiceEdgeClients)
        {
            voiceEdgeClient.close().awaitUninterruptibly();
        }

        Collection<OperatorClient> operatorClients = getOperatorClientPool().acquireAll();
        for (OperatorClient operatorClient : operatorClients)
        {
            operatorClient.close().awaitUninterruptibly();
        }

        eventClients.clear();
        userEdgeClients.clear();
        voiceEdgeClients.clear();

        ResultVerifier.getInstance().reset();
        EventVerifier.getInstance().reset();

        destroyedChannelPool.acquireAll();
        connectedUserPool.acquireAll();
        disconnectedUserPool.acquireAll();
        createdChannelPool.acquireAll();
        usersInChannels.acquireAll();
    }


    public VoiceEdgeClient createVoiceEdgeClient(MasterTestConfig config, ClientBootstrap bootstrap, DatagramChannelFactory datagramChannelFactory)
    {
        return new VoiceEdgeClient(config, bootstrap, datagramChannelFactory);
    }

    public UserEdgeClient createUserEdgeClient(MasterTestConfig config, ClientBootstrap bootstrap, int portId)
    {
        return new UserEdgeClient(config, bootstrap, portId);
    }

    protected JobTrigger[] setupJobTriggers()
    {
        return new JobTrigger[]{
                new JobTrigger(workerThreadPool, "JoinUserToChannel", config.getJoinUserToChannel())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new JoinUserToChannelJob(this, MasterTestDirector.this);
                    }
                },


                new JobTrigger(workerThreadPool, "UserConnect", config.getUserEdgeConnects())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new UserConnect(this, MasterTestDirector.this);
                    }
                },


                new JobTrigger(workerThreadPool, "GetUsersInChannel", config.getUsersInChannel())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new GetUsersInChannel(this, MasterTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "GetUsersOnlineStatus", config.getGetUserOnlineStatus())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new GetUsersOnlineStatus(this, MasterTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "PartUserFromChannel", config.getPartUserFromChannel())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new PartUserFromChannelJob(this, MasterTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "UserDisconnect", config.getUserEdgeDisconnects())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new UserDisconnect(this, MasterTestDirector.this);
                    }
                },


                new JobTrigger(workerThreadPool, "EventConnect", config.getEventConnects())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new EventClientConnectionCycle(this, MasterTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "OperatorConnect", config.getOperatorConnects())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new OperatorClientConnectionCycle(this, MasterTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "DisconnectUser", config.getDisconnectUser())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new DisconnectUser(this, MasterTestDirector.this);
                    }
                },

        };
    }

    public OperatorClient createOperatorClient()
    {
        ClientBootstrap bootstrap = getBootstrap();

        OperatorClient clientHandler = new OperatorClient(config.getOperatorAddress(), bootstrap);
        bootstrap.setPipelineFactory(new VoiceServerPipelineFactory(clientHandler));
        return clientHandler;
    }

    public VoiceServer getVoiceServerByAddress(String voiceAddress)
    {
        return voiceServerMap.get(voiceAddress);
    }

    public Pool<OperatorClient> getOperatorClientPool()
    {
        return operatorClientPool;
    }

    public Pool<User> getConnectedUserPool()
    {
        return connectedUserPool;
    }

    public Pool<Channel> getDestroyedChannelPool()
    {
        return destroyedChannelPool;
    }

    public Pool<User> getUserInChannelPool()
    {
        return usersInChannels;
    }

    public Pool<Channel> getCreatedChannelPool()
    {
        return createdChannelPool;
    }


    public List<Channel> createChannels(int channelIdStart, int channelIdEnd)
    {
        List<Channel> channels = new ArrayList<Channel>(channelIdEnd - channelIdStart);
        for (int id = channelIdStart; id < channelIdEnd; id++)
        {
            String channelId = String.format("CID-%08x", id);
            String channelDesc = String.format("Description of %s", channelId);
            Channel channel = new Channel(channelId, channelDesc);
            channels.add(channel);
        }
        return channels;
    }

    static public void main(String args[]) throws Exception
    {
        MasterTestDirector masterTestDirector = new MasterTestDirector(new MasterTestConfig(System.getProperty("config")));

        masterTestDirector.start();
        masterTestDirector.start();
    }

    public Pool<User> getDisconnectedUserPool()
    {
        return disconnectedUserPool;
    }

    public UserEdgeClient getRandomUserEdgeClient()
    {
        Iterator<UserEdgeClient> iterator = userEdgeClients.values().iterator();
        UserEdgeClient next = iterator.next();
        return next;
    }


    public EventClient createEventClient(String rrGroup)
    {
        EventClient eventClient = new EventClient(config, getBootstrap(), rrGroup);
        return eventClient;
    }

    public UserEdgeClient getUserEdgeClientByPort(int port)
    {
        return userEdgeClients.get(port);
    }

    public String getRandomIP()
    {
        if (locationGenerator == null)
        {
            return "130.244.1.1";
        }

        return locationGenerator.getRandomIP();
    }

    public MasterTestConfig getConfig()
    {
        return config;
    }

    public void addUserEdgeClient(UserEdgeClient edgeClient)
    {
        userEdgeClients.put(edgeClient.getPortId(), edgeClient);
    }
}
