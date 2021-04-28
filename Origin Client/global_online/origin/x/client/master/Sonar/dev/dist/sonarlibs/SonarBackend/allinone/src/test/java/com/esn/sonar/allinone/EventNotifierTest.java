package com.esn.sonar.allinone;

import com.esn.sonar.master.api.event.EventConsumer;
import com.esn.sonar.master.api.event.RoundRobinEventPublisher;
import com.esn.sonar.core.Message;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.LinkedList;

import static junit.framework.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class EventNotifierTest
{
    private class TestEventConsumer implements EventConsumer
    {
        private final String roundRobinGroup;
        final LinkedList<Message> events = new LinkedList<Message>();

        public TestEventConsumer(String roundRobinGroup)
        {
            this.roundRobinGroup = roundRobinGroup;
        }

        public String getRoundRobinGroup()
        {
            return roundRobinGroup;
        }

        public void handleEvent(Message event)
        {
            events.addLast(event);
        }

        public LinkedList<Message> getEvents()
        {
            return events;
        }
    }

    private class TestEvent extends Message
    {
        private TestEvent(String argument)
        {
            super(0, argument);
        }
    }


    private RoundRobinEventPublisher eventNotifier;

    @Before
    public void start()
    {
        eventNotifier = new RoundRobinEventPublisher();
    }

    @After
    public void stop()
    {
        eventNotifier = null;
    }

    @Test
    public void eventArgumentOrder()
    {
        TestEventArgument testEvent = new TestEventArgument("John", "Doe", "31337", "Pizza");

        assertEquals("TestEventArgument", testEvent.getName());

        assertEquals(4, testEvent.getArgumentCount());

        assertEquals(testEvent.getArgument(0), "John");
        assertEquals(testEvent.getArgument(1), "Doe");
        assertEquals(testEvent.getArgument(2), "31337");
        assertEquals(testEvent.getArgument(3), "Pizza");
    }

    @Test
    public void addRemoveConsumer()
    {
        TestEventConsumer consumer = new TestEventConsumer("");

        eventNotifier.addConsumer(consumer);
        assertTrue(eventNotifier.hasConsumer(consumer));

        eventNotifier.removeConsumer(consumer);
        assertFalse(eventNotifier.hasConsumer(consumer));
    }

    @Test
    public void roundRobinOneGroup()
    {
        TestEventConsumer group1[] = {new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1")};

        for (TestEventConsumer consumer : group1)
        {
            eventNotifier.addConsumer(consumer);
        }

        eventNotifier.publishEvent(new TestEvent("TEST"));

        int msgCount = 0;

        for (TestEventConsumer consumer : group1)
        {
            msgCount += consumer.getEvents().size();
        }

        assertEquals(1, msgCount);
    }

    @Test
    public void roundRobinTwoGroup()
    {
        TestEventConsumer group1[] = {new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1")};
        TestEventConsumer group2[] = {new TestEventConsumer("GROUP2"), new TestEventConsumer("GROUP2"), new TestEventConsumer("GROUP2")};

        for (TestEventConsumer consumer : group1)
        {
            eventNotifier.addConsumer(consumer);
        }

        for (TestEventConsumer consumer : group2)
        {
            eventNotifier.addConsumer(consumer);
        }

        eventNotifier.publishEvent(new TestEvent("TEST"));

        int msgCount = 0;

        for (TestEventConsumer consumer : group1)
        {
            msgCount += consumer.getEvents().size();
        }
        assertEquals(1, msgCount);

        msgCount = 0;

        for (TestEventConsumer consumer : group2)
        {
            msgCount += consumer.getEvents().size();
        }

        assertEquals(1, msgCount);
    }


    @Test
    public void roundRobinTwoGroupFairness()
    {
        TestEventConsumer group1[] = {new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1"), new TestEventConsumer("GROUP1")};
        TestEventConsumer group2[] = {new TestEventConsumer("GROUP2"), new TestEventConsumer("GROUP2"), new TestEventConsumer("GROUP2")};

        for (TestEventConsumer consumer : group1)
        {
            eventNotifier.addConsumer(consumer);
        }

        for (TestEventConsumer consumer : group2)
        {
            eventNotifier.addConsumer(consumer);
        }

        eventNotifier.publishEvent(new TestEvent("TEST1"));
        eventNotifier.publishEvent(new TestEvent("TEST2"));
        eventNotifier.publishEvent(new TestEvent("TEST3"));

        int msgCount = 0;

        for (TestEventConsumer consumer : group1)
        {
            assertEquals(1, consumer.getEvents().size());
            msgCount += consumer.getEvents().size();
        }
        assertEquals(3, msgCount);

        msgCount = 0;

        for (TestEventConsumer consumer : group2)
        {
            assertEquals(1, consumer.getEvents().size());
            msgCount += consumer.getEvents().size();
        }

        assertEquals(3, msgCount);
    }

    private class TestEventArgument extends Message
    {
        public TestEventArgument(String... args)
        {
            super(0, "TestEventArgument", args);
        }
    }
}
