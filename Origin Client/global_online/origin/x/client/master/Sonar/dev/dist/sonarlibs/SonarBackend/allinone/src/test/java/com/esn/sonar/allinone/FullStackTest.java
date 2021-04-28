package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeEventClient;
import com.esn.sonar.allinone.client.FakeOperatorClient;
import com.esn.sonar.allinone.client.SonarClientFactory;
import com.esn.sonar.allinone.util.VoiceClientRunner;
import com.esn.sonar.allinone.util.VoiceServerRunner;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.api.operator.OperatorService;
import com.esn.sonar.master.voice.VoiceEdgeConnection;
import com.esn.sonar.master.voice.VoiceManager;
import com.esn.sonar.master.voice.VoiceServer;
import com.esn.sonar.voiceedge.VoiceEdgeServer;
import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.Callable;

import static org.junit.Assert.*;

public class FullStackTest extends BaseIntegrationTest
{
    private static final String USER_ID = "USER_ID";
    private static final String USER_DESC_1 = "User_Desc_1";
    private static final String CHANNEL_ID = "CHANNEL_ID";
    private static final String CHANNEL_DESC = "Channel_Desc_1";
    private static final String MY_REASON_TYPE = "MyOwnReasonType";
    private static final String MY_REASON_DESC = "MyOwnReasonDesc";

    private FakeEventClient eventClient;
    private FakeOperatorClient operatorClient;

    @Override
    protected OperatorService createOperatorService() throws InterruptedException
    {
        clientFactory = new SonarClientFactory();
        eventClient = clientFactory.newEventClient(config, OPERATOR_ID);
        eventClient.connect().waitForRegister();

        operatorClient = clientFactory.newFakeOperatorClient(config);
        operatorClient.connect().waitForConnect();
        return operatorClient;
    }

    @After
    public void cleanUp() throws InterruptedException
    {
        operatorClient.close();

        eventClient.disconnect().waitForDisconnect();
        eventClient.close();
        clientFactory.release();
    }

    private VoiceServerRunner createAndConnectVoiceRunner() throws Exception
    {
        VoiceServerRunner runner = new VoiceServerRunner(config.voiceEdgeConfig.getBindAddress().getPort());

        await.until(new Callable<Boolean>()
        {
            public Boolean call() throws Exception
            {
                return MasterServer.getInstance().getVoiceManager().getServerCount() > 0;
            }
        });

        return runner;
    }

    @Before
    public void before() throws Exception
    {
        voiceServerRunner = createAndConnectVoiceRunner();
    }


    @After
    public void after() throws InterruptedException, IOException
    {
        voiceServerRunner.shutdown();
        if (client1 != null) client1.shutdown();
        if (voiceServerRunner2 != null) voiceServerRunner2.shutdown();
    }

    private VoiceClientRunner clientFactory(String userId, String userDesc, String channelId, String channelDesc) throws IOException, InterruptedException, OperatorService.UnavailableException, OperatorService.InvalidArgumentException, Token.InvalidToken, OperatorService.ChannelAllocationFailed
    {
        String controlToken = operatorService.getChannelToken(OPERATOR_ID, userId, userDesc, channelId, channelDesc, "", "");
        Token token = new Token(controlToken, null, -1);
        return new VoiceClientRunner(token, MasterServer.getInstance().getKeyPair().getPrivate());
    }

    Collection<String> getUsersInChannel(String operatorId, String channelId)
    {
        try
        {
            return operatorService.getUsersInChannel(operatorId, channelId);
        } catch (OperatorService.ChannelNotFoundException e)
        {
            return new ArrayList<String>();
        } catch (OperatorService.InvalidArgumentException e)
        {
            return new ArrayList<String>();
        }
    }

    @Test
    public void connectDisconnectDestroy() throws Exception
    {
        client1 = clientFactory(USER_ID, USER_DESC_1, CHANNEL_ID, CHANNEL_DESC);

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_ID));
        assertTrue(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_ID));

        Message evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserConnectedToChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(1));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage(1);

        assertEquals(null, evt);
        client1.shutdown();
        client1 = null;

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() > 0)
            Thread.sleep(20);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserDisconnectedFromChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(0));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.ChannelDestroyed);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), CHANNEL_ID);
        assertEquals(evt.getArgumentCount(), 2);

        evt = eventClient.waitForEventMessage(1);
        assertEquals(null, evt);
    }

    @Test
    public void partUserFromChannelCommand() throws Exception
    {
        client1 = clientFactory(USER_ID, USER_DESC_1, CHANNEL_ID, CHANNEL_DESC);
        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_ID));
        assertTrue(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_ID));

        Message evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserConnectedToChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(1));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage(1);

        assertEquals(null, evt);
        operatorService.partUserFromChannel(OPERATOR_ID, USER_ID, CHANNEL_ID, MY_REASON_TYPE, MY_REASON_DESC);

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() > 0)
            Thread.sleep(20);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserDisconnectedFromChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(0));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.ChannelDestroyed);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), CHANNEL_ID);
        assertEquals(evt.getArgumentCount(), 2);

        evt = eventClient.waitForEventMessage(1);

        assertEquals(null, evt);

        assertEquals(client1.getReasonType(), MY_REASON_TYPE);
        assertEquals(client1.getReasonDesc(), MY_REASON_DESC);
    }

    @Test
    public void destroyChannelCommand() throws Exception
    {
        client1 = clientFactory(USER_ID, USER_DESC_1, CHANNEL_ID, CHANNEL_DESC);
        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_ID));
        assertTrue(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_ID));

        Message evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserConnectedToChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(1));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage(1);

        assertEquals(null, evt);
        operatorService.destroyChannel(OPERATOR_ID, CHANNEL_ID, MY_REASON_TYPE, MY_REASON_DESC);

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() > 0)
            Thread.sleep(20);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.UserDisconnectedFromChannel);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), USER_ID);
        assertEquals(evt.getArgument(2), USER_DESC_1);
        assertEquals(evt.getArgument(3), CHANNEL_ID);
        assertEquals(evt.getArgument(4), CHANNEL_DESC);
        assertEquals(evt.getArgument(5), Integer.toString(0));
        assertEquals(evt.getArgumentCount(), 6);

        evt = eventClient.waitForEventMessage();

        assertEquals(evt.getName(), Protocol.Events.ChannelDestroyed);
        assertEquals(evt.getArgument(0), OPERATOR_ID);
        assertEquals(evt.getArgument(1), CHANNEL_ID);
        assertEquals(evt.getArgumentCount(), 2);

        evt = eventClient.waitForEventMessage(1);

        assertEquals(null, evt);

        assertEquals(client1.getReasonType(), MY_REASON_TYPE);
        assertEquals(client1.getReasonDesc(), MY_REASON_DESC);
    }

    @Test
    public void disasterRecoveryAndState() throws Exception
    {
        List<VoiceClientRunner> clients = new LinkedList<VoiceClientRunner>();
        int MAX_CHANNELS = 4;
        int MAX_CLIENTS = 4;

        for (int channel = 0; channel < MAX_CHANNELS; channel++)
            for (int client = 0; client < MAX_CLIENTS; client++)
            {
                StringBuilder userId = new StringBuilder();
                userId.append("USER");
                userId.append(client);
                userId.append("_");
                userId.append(channel);

                StringBuilder userDesc = new StringBuilder();
                userDesc.append("User");
                userDesc.append(client);
                userDesc.append("_");
                userDesc.append(channel);

                StringBuilder channelId = new StringBuilder();
                channelId.append(channel);

                StringBuilder channelDesc = new StringBuilder();
                channelDesc.append("Channel");
                channelDesc.append(channel);

                clients.add(clientFactory(userId.toString(), userDesc.toString(), channelId.toString(), channelDesc.toString()));
            }

        for (int channel = 0; channel < MAX_CHANNELS; channel++)
            while (getUsersInChannel(OPERATOR_ID, Integer.toString(channel)).size() != MAX_CLIENTS)
                Thread.sleep(100);

        VoiceManager voiceManager = MasterServer.getInstance().getVoiceManager();
        Collection<VoiceServer> voiceServers = voiceManager.getVoiceServers();
        assertEquals(1, voiceServers.size());

        VoiceServer voiceServer = voiceManager.getVoiceServers().iterator().next();
        voiceServer.getConnection().sendEdgeUnregisterClient(voiceServer.getId(), Protocol.Reasons.TryAgain, "");

        while (MasterServer.getInstance().getVoiceManager().getServerCount() != 0) ;
        Thread.sleep(20);

        MasterServer.getInstance().flushChannels();

        for (int channel = 0; channel < MAX_CHANNELS; channel++)
            while (getUsersInChannel(OPERATOR_ID, Integer.toString(channel)).size() != 0)
                Thread.sleep(20);

        while (MasterServer.getInstance().getVoiceManager().getServerCount() != 1) ;
        Thread.sleep(20);

        for (int channel = 0; channel < MAX_CHANNELS; channel++)
            while (getUsersInChannel(OPERATOR_ID, Integer.toString(channel)).size() != MAX_CLIENTS)
                Thread.sleep(20);

        for (VoiceClientRunner client : clients)
        {
            client.shutdown();
        }
    }

    @Test
    public void disasterRecoverySplit() throws Exception
    {
        LinkedList<VoiceClientRunner> clients = new LinkedList<VoiceClientRunner>();
        int MAX_CLIENTS = 4;

        for (int client = 0; client < MAX_CLIENTS; client++)
        {
            StringBuilder userId = new StringBuilder();
            userId.append("USER");
            userId.append(client);

            StringBuilder userDesc = new StringBuilder();
            userDesc.append("User");
            userDesc.append(client);

            clients.addLast(clientFactory(userId.toString(), userDesc.toString(), CHANNEL_ID, CHANNEL_DESC));
        }

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() != MAX_CLIENTS)
            Thread.sleep(100);

        VoiceManager voiceManager = MasterServer.getInstance().getVoiceManager();
        Collection<VoiceServer> voiceServers = voiceManager.getVoiceServers();
        assertEquals(1, voiceServers.size());

        VoiceServer voiceServer = voiceManager.getVoiceServers().iterator().next();
        voiceServer.getConnection().sendEdgeUnregisterClient(voiceServer.getId(), Protocol.Reasons.TryAgain, "");

        while (MasterServer.getInstance().getVoiceManager().getServerCount() > 0) ;
        Thread.sleep(20);

        this.voiceServerRunner2 = createAndConnectVoiceRunner();

        while (MasterServer.getInstance().getVoiceManager().getServerCount() < 1) ;
        Thread.sleep(20);

        for (int client = 0; client < MAX_CLIENTS; client++)
        {
            StringBuilder userId = new StringBuilder();
            userId.append("USER");
            userId.append(client + 4);

            StringBuilder userDesc = new StringBuilder();
            userDesc.append("User");
            userDesc.append(client + 4);

            clients.addLast(clientFactory(userId.toString(), userDesc.toString(), CHANNEL_ID, CHANNEL_DESC));
        }

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() < (MAX_CLIENTS * 2))
            Thread.sleep(100);

        while (MasterServer.getInstance().getVoiceManager().getServerCount() < 2) ;
        Thread.sleep(20);

        Thread.sleep(1000);
        Iterator<VoiceClientRunner> iterator = clients.iterator();

        while (getUsersInChannel(OPERATOR_ID, CHANNEL_ID).size() > MAX_CLIENTS)
            Thread.sleep(100);

        int numDisconnected;

        do
        {
            numDisconnected = 0;

            for (VoiceClientRunner client : clients)
                if (client.getReasonType() != null)
                    numDisconnected++;

            Thread.sleep(100);
        } while (numDisconnected != MAX_CLIENTS);

        for (int index = 0; index < MAX_CLIENTS; index++)
        {
            VoiceClientRunner client = iterator.next();
            assertNotNull(client.getReasonType());
        }

        for (int index = 0; index < MAX_CLIENTS; index++)
        {
            VoiceClientRunner client = iterator.next();

            assertNull(client.getReasonType());
            assertNull(client.getReasonDesc());
        }

        for (VoiceClientRunner client : clients)
        {
            client.shutdown();
        }
    }


    private VoiceServerRunner voiceServerRunner;
    private VoiceServerRunner voiceServerRunner2;
    private VoiceClientRunner client1;

}
