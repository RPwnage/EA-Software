package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeUserClient;
import com.esn.sonar.allinone.client.FakeVoiceServerClient;
import com.esn.sonar.core.Token;
import com.esn.sonar.master.api.operator.OperatorService;
import org.apache.log4j.Logger;
import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * User: ronnie
 * Date: 2011-07-05
 * Time: 14:16
 */
public class OrphanTest extends BaseIntegrationTest
{
    private static final Logger log = Logger.getLogger(OrphanTest.class);

    private FakeUserClient user1;
    private FakeUserClient user2;
    private FakeVoiceServerClient voiceServerClient;


    private static final String USER_1_ID = "hector";
    private static final String USER_2_ID = "dmitry";
    private static final String CHANNEL_ID = "chat-channel";
    private static final String CHANNEL_DESC = "Hector the Inspector's chat channel";
    private static final String LOCATION = "world";

    @Override
    protected void setUpAllInOneConfig(AllInOneConfig config)
    {
        config.masterConfig.setMaxOrphanCount(1);
        config.masterConfig.setOrphanTaskInterval(1);
    }

    @Before
    public void setup() throws InterruptedException
    {
        log.debug("OrphanTest.setup");
        operator = server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID);
        operator.getEventPublisher().addConsumer(this);

        log.info("Connecting clients");
        user1 = clientFactory.newUserClient(new Token(OPERATOR_ID, USER_1_ID, USER_1_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        user2 = clientFactory.newUserClient(new Token(OPERATOR_ID, USER_2_ID, USER_2_ID, "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegister();
        log.debug("OrphanTest.setup done");
    }

    @After
    public void tearDown() throws InterruptedException
    {
        log.debug("OrphanTest.tearDown");
        assertEquals("All messages handled " + receivedEvents.toString(), 0, receivedEvents.size());

        log.info("Disconnecting clients");
        if (user1 != null) user1.close();
        if (user2 != null) user2.close();
        if (voiceServerClient != null) voiceServerClient.close();
        log.debug("OrphanTest.tearDown done");
    }

    /*
    This server tests that the OrphanTask cleans up orphaned channel at the configured rate (see BaseIntegrationTest constructor)
     */
    @Test
    public void orphanChannelDestroy() throws Exception
    {
        log.debug("OrphanTest.orphanChannelDestroy");
        user1.connect().waitForRegister();
        expectEvents(new OperatorService.UserConnectedEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription()));

        operatorService.joinUserToChannel(OPERATOR_ID, LOCATION, USER_1_ID, CHANNEL_ID, CHANNEL_DESC);
        user1.waitForJoinChannel();

        await.until(usersInChannel(CHANNEL_ID), Matchers.hasItem(USER_1_ID));
        assertTrue(operatorService.getUsersInChannel(OPERATOR_ID, CHANNEL_ID).contains(USER_1_ID));

        expectEvents(new OperatorService.UserConnectedToChannelEvent(user1.getOperatorId(), user1.getUserId(), user1.getUserDescription(), CHANNEL_ID, CHANNEL_DESC, 1));

        voiceServerClient.disconnect().close();
        Thread.sleep(5000);

        expectEvents(new OperatorService.ChannelDestroyedEvent(user1.getOperatorId(), CHANNEL_ID));
        log.debug("OrphanTest.orphanChannelDestroy done");
    }

}
