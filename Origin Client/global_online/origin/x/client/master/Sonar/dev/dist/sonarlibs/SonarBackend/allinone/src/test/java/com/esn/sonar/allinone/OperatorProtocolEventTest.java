package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeEventClient;
import com.esn.sonar.allinone.client.SonarClientFactory;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.api.operator.OperatorService;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import static junit.framework.Assert.assertEquals;

//TODO: Test needed for edge connections dropping cleanup and edge connection state sync!

public class OperatorProtocolEventTest extends BaseIntegrationTest
{
    private FakeEventClient eventClient;
    private static final String OPERATOR_ID = "operatorId";
    private SonarClientFactory clientFactory;

    @Before
    public void connectClient() throws InterruptedException
    {
        clientFactory = new SonarClientFactory();
        eventClient = clientFactory.newEventClient(config, OPERATOR_ID);
        eventClient.connect().waitForRegister();
    }

    @After
    public void disconnectClient() throws InterruptedException
    {
        eventClient.disconnect().waitForDisconnect();
        eventClient.close();
        clientFactory.release();
    }

    @Test
    public void userConnected()
    {
        OperatorService.UserConnectedEvent event = new OperatorService.UserConnectedEvent(OPERATOR_ID, "userId", "userDesc");
        server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID).getEventPublisher().publishEvent(event);

        Message message = eventClient.waitForEventMessage();

        assertEquals(Protocol.Events.UserConnected, message.getName());
        assertEquals(3, message.getArgumentCount());

        assertEquals(OPERATOR_ID, message.getArgument(0));
        assertEquals("userId", message.getArgument(1));
        assertEquals("userDesc", message.getArgument(2));
    }

    @Test
    public void userDisconnected()
    {
        OperatorService.UserDisconnectedEvent event = new OperatorService.UserDisconnectedEvent(OPERATOR_ID, "userId", "userDesc");
        server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID).getEventPublisher().publishEvent(event);

        Message message = eventClient.waitForEventMessage();

        assertEquals(Protocol.Events.UserDisconnected, message.getName());
        assertEquals(3, message.getArgumentCount());

        assertEquals(OPERATOR_ID, message.getArgument(0));
        assertEquals("userId", message.getArgument(1));
        assertEquals("userDesc", message.getArgument(2));
    }

    @Test
    public void userConnectedToChannel()
    {
        OperatorService.UserConnectedToChannelEvent event = new OperatorService.UserConnectedToChannelEvent(OPERATOR_ID, "userId", "userDesc", "channelId", "channelDesc", 34);
        server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID).getEventPublisher().publishEvent(event);

        Message message = eventClient.waitForEventMessage();

        assertEquals(Protocol.Events.UserConnectedToChannel, message.getName());
        assertEquals(6, message.getArgumentCount());

        assertEquals(OPERATOR_ID, message.getArgument(0));
        assertEquals("userId", message.getArgument(1));
        assertEquals("userDesc", message.getArgument(2));
        assertEquals("channelId", message.getArgument(3));
        assertEquals("channelDesc", message.getArgument(4));
        assertEquals("34", message.getArgument(5));
    }

    @Test
    public void userDisconnectedFromChannel()
    {
        OperatorService.UserDisconnectedFromChannelEvent event = new OperatorService.UserDisconnectedFromChannelEvent(OPERATOR_ID, "userId", "userDesc", "channelId", "channelDesc", 22);
        server.getMasterServer().getOperatorManager().getOperator(OPERATOR_ID).getEventPublisher().publishEvent(event);

        Message message = eventClient.waitForEventMessage();

        assertEquals(Protocol.Events.UserDisconnectedFromChannel, message.getName());
        assertEquals(6, message.getArgumentCount());

        assertEquals(OPERATOR_ID, message.getArgument(0));
        assertEquals("userId", message.getArgument(1));
        assertEquals("userDesc", message.getArgument(2));
        assertEquals("channelId", message.getArgument(3));
        assertEquals("channelDesc", message.getArgument(4));
        assertEquals("22", message.getArgument(5));
    }
}
