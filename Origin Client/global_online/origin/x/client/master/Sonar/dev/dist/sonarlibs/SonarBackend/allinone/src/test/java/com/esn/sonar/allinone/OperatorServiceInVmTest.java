package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeUserClient;
import com.esn.sonar.allinone.client.FakeVoiceServerClient;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.master.api.operator.OperatorService;
import org.apache.log4j.Logger;
import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import java.util.Arrays;

import static com.esn.sonar.master.api.operator.OperatorService.*;
import static org.junit.Assert.*;

public class OperatorServiceInVmTest extends BaseIntegrationTest
{
    private static final Logger log = Logger.getLogger(OperatorServiceInVmTest.class);

    private FakeUserClient user1;
    private FakeUserClient user2;
    private FakeVoiceServerClient voiceServerClient;

    private static final String USER_ID_INVALID = "d|mitr|";

    private static final String USER_1_ID = "hector";
    private static final String USER_1_DESC = "I'm the inspector";
    private static final String USER_2_ID = "dmitry";
    private static final String USER_2_DESC = "Hector's best friend";
    private static final String CHANNEL_ID = "chat-channel";
    private static final String CHANNEL_DESC = "Hector the Inspector's chat channel";
    private static final String LOCATION = "world";
    private static final String REASON = "You were kicked";
    private static final String REASON_TYPE = Protocol.Reasons.AdminKick;


    @Before
    public void setup() throws InterruptedException
    {
        log.debug("OperatorServiceInVmTest.setup");
        operator = server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID);
        operator.getEventPublisher().addConsumer(this);

        log.info("Connecting clients");
        user1 = clientFactory.newUserClient(new Token(OPERATOR_ID, USER_1_ID, USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        user2 = clientFactory.newUserClient(new Token(OPERATOR_ID, USER_2_ID, USER_2_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegister();
        log.debug("OperatorServiceInVmTest.setup done");
    }

    @After
    public void tearDown() throws InterruptedException
    {
        log.debug("OperatorServiceInVmTest.tearDown");
        assertEquals("All messages handled " + receivedEvents.toString(), 0, receivedEvents.size());

        log.info("Disconnecting clients");
        if (user1 != null) user1.close();
        if (user2 != null) user2.close();
        if (voiceServerClient != null) voiceServerClient.close();
        log.debug("OperatorServiceInVmTest.tearDown done");
    }

    @Test
    public void tokenGeneration() throws Token.InvalidToken, InvalidArgumentException, UnavailableException
    {
        log.debug("OperatorServiceInVmTest.tokenGeneration");
        Token t = new Token(operatorService.getUserControlToken("operator", USER_1_ID, USER_1_DESC, "", "", ""), null, -1);
        assertEquals(USER_1_ID, t.getUserId());
        assertEquals(USER_1_DESC, t.getUserDescription());
        assertEquals("", t.getChannelId());
        assertEquals("", t.getChannelDescription());
        assertEquals(config.userEdgeConfig.getLocalAddress(), t.getControlAddress());
        assertEquals(config.userEdgeConfig.getBindAddress().getPort(), t.getControlPort());
        log.debug("OperatorServiceInVmTest.tokenGeneration done");
    }

    @Test
    public void joinUserToChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.joinUserToChannel");
        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_1_ID));
        assertTrue(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_1_ID));

        expectEvents(new UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));
        log.debug("OperatorServiceInVmTest.joinUserToChannel done");
    }

    @Test
    public void partUserFromChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.partUserFromChannel");
        user1.connect().waitForRegister();
        this.expectEvents(new UserConnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();
        this.expectEvents(new UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        user2.connect().waitForRegister();
        this.expectEvents(new UserConnectedEvent(user1.getOperatorId(), user2.getUserId(), user2.getUserDescription()));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_2_ID, CHANNEL_ID, CHANNEL_DESC);
        user2.waitForJoinChannel();

        this.expectEvents(new UserConnectedToChannelEvent(user2.getOperatorId(), user2.getUserId(), user2.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 2));

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItems(USER_1_ID, USER_2_ID));

        operatorService.partUserFromChannel(OPERATOR_ID, USER_1_ID, CHANNEL_ID, REASON_TYPE, REASON);
        user1.waitForVoiceServerDisconnect();
        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_2_ID));

        assertFalse(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_1_ID));
        expectEvents(new UserDisconnectedFromChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));
        log.debug("OperatorServiceInVmTest.partUserFromChannel done");
    }

    @Test(expected = ChannelNotFoundException.class)
    public void destroyChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.destroyChannel");
        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_1_ID));
        expectEvents(new UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        operatorService.destroyChannel(OPERATOR_ID, CHANNEL_ID, REASON_TYPE, REASON);
        user1.waitForVoiceServerDisconnect();

        await.until(isChannelUnlinked(CHANNEL_ID));

        // Should raise an exception
        expectEvents(new UserDisconnectedFromChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 0));
        expectEvents(new ChannelDestroyedEvent(user1.getOperatorId(), CHANNEL_ID));

        assertFalse(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_1_ID));
        log.debug("OperatorServiceInVmTest.destroyChannel done");
    }

    @Test
    public void getUsersOnlineStatus() throws Exception
    {
        log.debug("OperatorServiceInVmTest.getUsersOnlineStatus");
        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));

        UserOnlineStatus uosOffline1 = new UserOnlineStatus(USER_1_ID, false, "");
        UserOnlineStatus uosOffline2 = new UserOnlineStatus(USER_2_ID, false, "");
        UserOnlineStatus uosOnlineWithoutChannel1 = new UserOnlineStatus(USER_1_ID, true, "");
        UserOnlineStatus uosOnline1 = new UserOnlineStatus(USER_1_ID, true, CHANNEL_ID);
        UserOnlineStatus uosOnline2 = new UserOnlineStatus(USER_2_ID, true, CHANNEL_ID);

        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItem(uosOnlineWithoutChannel1));


        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();

        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItem(uosOnline1));

        expectEvents(new UserConnectedToChannelEvent(OPERATOR_ID, USER_1_ID, USER_1_ID, CHANNEL_ID, CHANNEL_DESC, 1));

        user2.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_2_ID, USER_2_ID));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_2_ID, CHANNEL_ID, CHANNEL_DESC);
        user2.waitForJoinChannel();

        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID, USER_2_ID)), Matchers.hasItems(uosOnline1, uosOnline2));
        expectEvents(new UserConnectedToChannelEvent(OPERATOR_ID, USER_2_ID, USER_2_ID, CHANNEL_ID, CHANNEL_DESC, 2));


        user1.disconnect().waitForDisconnect();
        user2.disconnect().waitForDisconnect();

        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID, USER_2_ID)), Matchers.hasItems(uosOffline1, uosOffline2));
        await.until(isChannelUnlinked(CHANNEL_ID));

        expectEvents(
                new UserDisconnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID),
                new UserDisconnectedFromChannelEvent(OPERATOR_ID, USER_1_ID, USER_1_ID, CHANNEL_ID, CHANNEL_DESC, 1),
                new UserDisconnectedEvent(OPERATOR_ID, USER_2_ID, USER_2_ID),
                new UserDisconnectedFromChannelEvent(OPERATOR_ID, USER_2_ID, USER_2_ID, CHANNEL_ID, CHANNEL_DESC, 0),
                new ChannelDestroyedEvent(OPERATOR_ID, CHANNEL_ID));
        log.debug("OperatorServiceInVmTest.getUsersOnlineStatus done");
    }

    @Test
    public void getUsersInChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.getUsersInChannel");
        user1.connect().waitForRegister();
        user2.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID), new UserConnectedEvent(OPERATOR_ID, USER_2_ID, USER_2_ID));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();
        expectEvents(new UserConnectedToChannelEvent(OPERATOR_ID, USER_1_ID, USER_1_ID, CHANNEL_ID, CHANNEL_DESC, 1));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_2_ID, CHANNEL_ID, CHANNEL_DESC);
        user2.waitForJoinChannel();
        expectEvents(new UserConnectedToChannelEvent(OPERATOR_ID, USER_2_ID, USER_2_ID, CHANNEL_ID, CHANNEL_DESC, 2));

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_1_ID));
        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_2_ID));

        operatorService.partUserFromChannel(OPERATOR_ID, USER_1_ID, CHANNEL_ID, REASON_TYPE, REASON);
        user1.waitForVoiceServerDisconnect();
        await.until(usersInChannel(CHANNEL_ID), Matchers.not(Matchers.hasItem(USER_1_ID)));
        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_2_ID));

        expectEvents(new UserDisconnectedFromChannelEvent(OPERATOR_ID, USER_1_ID, USER_1_ID, CHANNEL_ID, CHANNEL_DESC, 1));
        log.debug("OperatorServiceInVmTest.getUsersInChannel done");
    }

    @Test
    public void getUserControlToken() throws Exception
    {
        log.debug("OperatorServiceInVmTest.getUserControlToken");
        String rawToken = operatorService.getUserControlToken(OPERATOR_ID, USER_1_ID, USER_1_DESC, "", "", "");
        Token t = new Token(rawToken, null, -1);
        assertEquals(USER_1_ID, t.getUserId());
        assertEquals(USER_1_DESC, t.getUserDescription());
        assertEquals("", t.getChannelId());
        assertEquals("", t.getChannelDescription());
        assertEquals(config.userEdgeConfig.getLocalAddress(), t.getControlAddress());
        assertEquals(config.userEdgeConfig.getBindAddress().getPort(), t.getControlPort());
        log.debug("OperatorServiceInVmTest.getUserControlToken done");
    }

    @Test
    public void disconnectUser() throws Exception
    {
        log.debug("OperatorServiceInVmTest.disconnectUser");
        UserOnlineStatus uosOnlineWithoutChannel = new UserOnlineStatus(USER_1_ID, true, "");
        UserOnlineStatus uosOffline = new UserOnlineStatus(USER_1_ID, false, "");

        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));

        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItems(uosOnlineWithoutChannel));

        operatorService.disconnectUser(OPERATOR_ID, USER_1_ID, REASON_TYPE, REASON);

        user1.waitForChannelClosed(5);
        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItems(uosOffline));

        expectEvents(new UserDisconnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));
        log.debug("OperatorServiceInVmTest.disconnectUser done");
    }

    @Test
    public void joinChannelFuture() throws Exception
    {
        log.debug("OperatorServiceInVmTest.joinChannelFuture");
        UserOnlineStatus uosOnline1 = new UserOnlineStatus(USER_1_ID, true, CHANNEL_ID);

        // Send a join channel call
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);

        // Connect client
        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));

        // Wait for connection to channel
        user1.waitForJoinChannel();
        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItem(uosOnline1));

        expectEvents(new UserConnectedToChannelEvent(OPERATOR_ID, USER_1_ID, USER_1_ID, CHANNEL_ID, CHANNEL_DESC, 1));
        log.debug("OperatorServiceInVmTest.joinChannelFuture done");
    }

    @Test
    public void voiceClientLimitTest() throws Exception
    {
        log.debug("OperatorServiceInVmTest.voiceClientLimitTest");
        voiceServerClient.disconnect().waitForDisconnect();

        Thread.sleep(1000);

        FakeVoiceServerClient vs1 = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 2);
        FakeVoiceServerClient vs2 = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 2);

        vs1.connect().waitForRegister();

        FakeUserClient us1 = clientFactory.newUserClient(new Token(OPERATOR_ID, "user1", USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        FakeUserClient us2 = clientFactory.newUserClient(new Token(OPERATOR_ID, "user2", USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        FakeUserClient us3 = clientFactory.newUserClient(new Token(OPERATOR_ID, "user3", USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        FakeUserClient us4 = clientFactory.newUserClient(new Token(OPERATOR_ID, "user4", USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);

        us1.connect().waitForRegister();
        us2.connect().waitForRegister();
        us3.connect().waitForRegister();
        us4.connect().waitForRegister();

        // Send a join channel call
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, "user1", "channel1", CHANNEL_DESC);
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, "user2", "channel1", CHANNEL_DESC);

        us1.waitForJoinChannel();
        us2.waitForJoinChannel();


        vs2.connect().waitForRegister();
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, "user3", "channel2", CHANNEL_DESC);
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, "user4", "channel2", CHANNEL_DESC);

        us3.waitForJoinChannel();
        us4.waitForJoinChannel();

        assertEquals(2, vs1.getClientCount());
        assertEquals(2, vs2.getClientCount());

        us1.close();
        us2.close();
        us3.close();
        us4.close();

        vs1.close();
        vs2.close();


        Thread.sleep(1000);

        receivedEvents.clear();
        log.debug("OperatorServiceInVmTest.voiceClientLimitTest done");
    }

    @Test
    public void joinChannelFutureExpired() throws Exception
    {
        log.debug("OperatorServiceInVmTest.joinChannelFutureExpired");
        UserOnlineStatus uosOnlineWithoutChannel1 = new UserOnlineStatus(USER_1_ID, true, "");

        config.masterConfig.setJoinChannelExpire(1);

        UserOnlineStatus uosOnline1 = new UserOnlineStatus(USER_1_ID, true, CHANNEL_ID);

        // Send a join channel call
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);

        Thread.sleep(2000);

        // Connect client
        user1.connect().waitForRegister();
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));

        // Wait for connection to channel
        await.until(usersOnlineStatus(Arrays.asList(USER_1_ID)), Matchers.hasItem(uosOnlineWithoutChannel1));

        //TODO: Validate that the expired joinFuture is gone here!
        log.debug("OperatorServiceInVmTest.joinChannelFutureExpired done");
    }

    @Test
    @Ignore("Fails in TC")
    public void controlTokenExpired() throws Exception
    {
        log.debug("OperatorServiceInVmTest.controlTokenExpired");
        config.userEdgeConfig.setControlTokenExpire(1);
        user1 = clientFactory.newUserClient(new Token("", "jonas", "jonas", "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        Thread.sleep(2000);

        user1.connect().waitForConnect().waitForRegisterError();
        log.debug("OperatorServiceInVmTest.controlTokenExpired done");
    }

    @Test
    public void updateControlToken() throws Exception
    {
        log.debug("OperatorServiceInVmTest.updateControlToken");
        config.userEdgeConfig.setControlTokenExpire(5);
        user1.connect().waitForRegister();
        user1.waitForUpdateToken(10);
        expectEvents(new UserConnectedEvent(OPERATOR_ID, USER_1_ID, USER_1_ID));
        log.debug("OperatorServiceInVmTest.updateControlToken done");
    }


    @Test(expected = InvalidArgumentException.class)
    public void invalidCharacterUserIdJoinUserToChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.invalidCharacterUserIdJoinUserToChannel");
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_ID_INVALID, CHANNEL_ID, CHANNEL_DESC);
        log.debug("OperatorServiceInVmTest.invalidCharacterUserIdJoinUserToChannel done");
    }

    @Test(expected = InvalidArgumentException.class)
    public void invalidLengthUserIdJoinUserToChannel() throws Exception
    {
        log.debug("OperatorServiceInVmTest.invalidLengthUserIdJoinUserToChannel");
        StringBuffer sb = new StringBuffer(Protocol.Limits.userIdLength + 1);
        for (int index = 0; index < Protocol.Limits.userIdLength + 1; index++)
        {
            sb.append('A');
        }

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, sb.toString(), CHANNEL_ID, CHANNEL_DESC);
        log.debug("OperatorServiceInVmTest.invalidLengthUserIdJoinUserToChannel done");
    }


    @Test(expected = InvalidArgumentException.class)
    public void invalidCharacterUserIdGetControlToken() throws Exception
    {
        log.debug("OperatorServiceInVmTest.invalidCharacterUserIdGetControlToken");
        operatorService.getUserControlToken(OPERATOR_ID, USER_ID_INVALID, USER_1_DESC, CHANNEL_ID, CHANNEL_DESC, LOCATION);
        log.debug("OperatorServiceInVmTest.invalidCharacterUserIdGetControlToken done");
    }


    @Test(expected = InvalidArgumentException.class)
    public void invalidLengthGetControlToken() throws Exception
    {
        log.debug("OperatorServiceInVmTest.invalidLengthGetControlToken");
        StringBuffer sb = new StringBuffer(Protocol.Limits.userIdLength + 1);
        for (int index = 0; index < Protocol.Limits.userIdLength + 1; index++)
        {
            sb.append('A');
        }

        operatorService.getUserControlToken(OPERATOR_ID, sb.toString(), USER_1_DESC, CHANNEL_ID, CHANNEL_DESC, LOCATION);
        log.debug("OperatorServiceInVmTest.invalidLengthGetControlToken done");
    }

    @Ignore("Fails in TC")
    @Test
    public void allocateChannelAndDisconnect() throws Exception
    {
        log.debug("OperatorServiceInVmTest.allocateChannelAndDisconnect");
        // Connect two voice servers
        FakeVoiceServerClient vs1 = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        vs1.connect().waitForRegister();
        // Create channel
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);

        // Wait for user to join channel
        user1.connect().waitForRegister();
        user1.waitForJoinChannel();

        expectEvents(new OperatorService.UserConnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));
        expectEvents(new UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        // Identify which server he joined
        Token token = user1.getCurrentToken();

        FakeVoiceServerClient dropServer;

        if (token.getVoipPort() == vs1.getVoipPort())
            dropServer = vs1;
        else if (token.getVoipPort() == voiceServerClient.getVoipPort())
            dropServer = voiceServerClient;
        else
            throw new RuntimeException("Server could not be matched!");

        // Disconnect voice server
        dropServer.close();

        // Wait for disconnect
        Thread.sleep(3000);

        // Join user to same channel
        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();
        expectEvents(new UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        // Make sure channel is on other server

        assertFalse("Not on disconnected voice server", dropServer.getVoipPort() == user1.getCurrentToken().getVoipPort());
        expectEvents(new UserDisconnectedFromChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        if (dropServer != vs1)
            vs1.close();

        //expectEvents(new UserDisconnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));
        log.debug("OperatorServiceInVmTest.allocateChannelAndDisconnect done");
    }

}
