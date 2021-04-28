package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.SonarClientFactory;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.util.Utils;
import com.esn.sonar.master.Operator;
import com.esn.sonar.master.api.event.EventConsumer;
import com.esn.sonar.master.api.operator.OperatorService;
import com.jayway.awaitility.core.ConditionFactory;
import org.apache.log4j.Logger;
import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;

import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeoutException;

import static com.jayway.awaitility.Awaitility.await;
import static java.util.concurrent.TimeUnit.SECONDS;

abstract public class BaseIntegrationTest implements EventConsumer
{
    private static final Logger log = Logger.getLogger(BaseIntegrationTest.class);
    protected static final String OPERATOR_ID = "operatorId";
    protected Operator operator;

    protected AllInOne server;
    protected AllInOneConfig config;
    protected OperatorService operatorService;

    protected SonarClientFactory clientFactory;
    protected ConcurrentLinkedQueue<Message> receivedEvents = null;
    protected final ConditionFactory await = await().atMost(5, SECONDS);

    protected BaseIntegrationTest()
    {
        System.setProperty("sonar.unittest", "true");
    }

    /**
     * Allows subclasses to manipulate the configuration before the {@link AllInOne} is started
     *
     * @param config the actual config to use when starting the {@link AllInOne}
     */
    protected void setUpAllInOneConfig(AllInOneConfig config)
    {

    }

    @Before
    public void startServer() throws Exception
    {
        log.debug("BaseIntegrationTest.startServer");
        log.info("Starting server..");

        receivedEvents = new ConcurrentLinkedQueue<Message>();

        Utils.create();

        config = new AllInOneConfig();

        config.userEdgeConfig.setBindAddress(getAvailableAddress());
        config.voiceEdgeConfig.setBindAddress(getAvailableAddress());
        config.voiceEdgeConfig.setChallengeBindAddress(getAvailableAddress());
        config.masterConfig.setEventServiceBindAddress(getAvailableAddress());
        config.masterConfig.setHttpServiceBindAddress(getAvailableAddress());
        config.masterConfig.setOperatorServiceBindAddress(getAvailableAddress());
        config.masterConfig.setUserBindAddress(getAvailableAddress());
        config.masterConfig.setVoiceBindAddress(getAvailableAddress());

        config.userEdgeConfig.setMasterAddress(config.masterConfig.getUserBindAddress());
        config.voiceEdgeConfig.setMasterAddress(config.masterConfig.getVoiceBindAddress());
        config.userEdgeConfig.setLocalAddress("127.0.0.1");

        config.userEdgeConfig.setRegistrationTimeout(2);
        config.voiceEdgeConfig.setRegistrationTimeout(2);
        config.voiceEdgeConfig.setChallengeTimeout(2);

        // Allow subclasses to manipulate the configuration
        setUpAllInOneConfig(config);

        clientFactory = new SonarClientFactory();

        server = new AllInOne(config, clientFactory.serverSocketFactory, clientFactory.socketChannelFactory, clientFactory.datagramChannelFactory);
        server.start();

        operatorService = createOperatorService();

        log.debug("BaseIntegrationTest.startServer done");
    }

    @After
    public void stopServer() throws InterruptedException
    {
        log.debug("BaseIntegrationTest.stopServer");
        log.info("Stopping server..");
        server.stop();

        operatorService = null;
        server = null;
        clientFactory.release();

        Utils.destroy();
        log.debug("BaseIntegrationTest.stopServer done");
    }


    protected OperatorService createOperatorService() throws InterruptedException
    {
        return server.getMasterServer().getOperatorService();
    }

    protected static final List<Integer> usedPorts = new ArrayList<Integer>();

    public static InetSocketAddress getAvailableAddress()
    {
        return getAvailablePortInRange(31337, 1000);
    }

    private static InetSocketAddress getAvailablePortInRange(int start, int length)
    {
        for (int i = start; i <= start + length; i++)
        {
            if (!usedPorts.contains(i) && checkPortAvailable(i))
            {
                usedPorts.add(i);
                return new InetSocketAddress("127.0.0.1", i);
            }
        }

        throw new RuntimeException("No available port found in range");
    }

    /**
     * Checks to see if a specific port is available.
     * For simplicity this function verifies both UDP and TCP
     *
     * @param port the port to check for availability
     * @return true if port is available
     */
    private static boolean checkPortAvailable(int port)
    {
        try
        {
            ServerSocket ss = new ServerSocket(port);
            ss.close();
        } catch (Throwable e)
        {
            return false;
        }

        try
        {
            DatagramSocket ds = new DatagramSocket(port);
            ds.close();
        } catch (Throwable e)
        {
            return false;
        }

        return true;
    }

    protected void expectEvents(Message... expectedEvents) throws Exception
    {
        try
        {
            await.until(receivedEvents(), Matchers.hasItems(expectedEvents));
        } catch (TimeoutException te)
        {
            ConcurrentLinkedQueue<Message> q = new ConcurrentLinkedQueue<Message>(receivedEvents);

            StringBuilder sb = new StringBuilder("Expected events doesn't match actual events\n");
            sb.append("Expected events:\n");
            for (Message e : expectedEvents)
                sb.append(e + "\n");

            sb.append("\n ---------------- \nBut got events:\n");
            for (Message e : q)
                sb.append(e + "\n");

            throw new AssertionError(sb.toString());
        }

        receivedEvents.removeAll(Arrays.asList(expectedEvents));
    }

    private Callable<Iterable<Message>> receivedEvents()
    {
        return new Callable<Iterable<Message>>()
        {
            public Iterable<Message> call() throws Exception
            {
                return receivedEvents;
            }
        };
    }

    public String getRoundRobinGroup()
    {
        return "group1";
    }

    public void handleEvent(Message event)
    {
        receivedEvents.add(event);
    }

    protected Callable<Iterable<String>> usersInChannel(final String channelId)
    {
        return new Callable<Iterable<String>>()
        {
            public Iterable<String> call()
            {
                try
                {
                    return operatorService.getUsersInChannel(OPERATOR_ID, channelId);
                } catch (OperatorService.ChannelNotFoundException e)
                {
                    return new ArrayList<String>(0);

                } catch (OperatorService.InvalidArgumentException e)
                {
                    return new ArrayList<String>(0);
                }
            }
        };
    }

    protected Callable<Boolean> isChannelUnlinked(final String channelId)
    {
        return new Callable<Boolean>()
        {
            public Boolean call() throws Exception
            {
                return (operator.getChannelManager().getVoiceChannel(channelId) == null);
            }
        };
    }

    protected Callable<Iterable<OperatorService.UserOnlineStatus>> usersOnlineStatus(final Collection<String> userIds)
    {
        return new Callable<Iterable<OperatorService.UserOnlineStatus>>()
        {
            public Iterable<OperatorService.UserOnlineStatus> call() throws Exception
            {
                return operatorService.getUsersOnlineStatus(OPERATOR_ID, userIds);
            }
        };
    }
}
