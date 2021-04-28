package com.esn.sonar.master.api.event;

import com.esn.sonar.core.Message;

import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;

public class RoundRobinEventPublisher implements EventPublisher
{
    private final Map<String, RoundRobinGroup> consumerMap = new HashMap<String, RoundRobinGroup>();


    private class RoundRobinGroup
    {
        private final AtomicInteger rrCounter = new AtomicInteger(0);
        private final List<EventConsumer> consumers = new ArrayList<EventConsumer>();

        public EventConsumer getConsumer()
        {
            return consumers.get(Math.abs(rrCounter.getAndIncrement()) % consumers.size());
        }

        public void addConsumer(EventConsumer consumer)
        {
            consumers.add(consumer);
        }

        public void removeConsumer(EventConsumer consumer)
        {
            consumers.remove(consumer);
        }

        public boolean hasConsumer(EventConsumer consumer)
        {
            return consumers.contains(consumer);
        }

        public boolean isEmpty()
        {
            return consumers.isEmpty();
        }
    }


    public RoundRobinEventPublisher()
    {
    }


    /*
    All methods are synchronized because there could be a race between adding and removing a RR group, or when iterating groups
     */

    public synchronized void addConsumer(EventConsumer consumer)
    {
        RoundRobinGroup group = consumerMap.get(consumer.getRoundRobinGroup());

        if (group == null)
        {
            group = new RoundRobinGroup();
            consumerMap.put(consumer.getRoundRobinGroup(), group);
        }

        group.addConsumer(consumer);
    }

    public synchronized void removeConsumer(EventConsumer consumer)
    {
        RoundRobinGroup group = consumerMap.get(consumer.getRoundRobinGroup());
        group.removeConsumer(consumer);

        if (group.isEmpty())
        {
            consumerMap.remove(consumer.getRoundRobinGroup());
        }
    }

    public void publishEvent(Message event)
    {
        Collection<EventConsumer> consumers = new LinkedList<EventConsumer>();
        synchronized (this)
        {
            for (RoundRobinGroup roundRobinGroup : consumerMap.values())
            {
                consumers.add(roundRobinGroup.getConsumer());
            }
        }

        // Leaving synchronized block before sending
        for (EventConsumer consumer : consumers)
        {
            consumer.handleEvent(event);
        }
    }

    public synchronized boolean hasConsumer(EventConsumer consumer)
    {
        RoundRobinGroup group = consumerMap.get(consumer.getRoundRobinGroup());

        if (group == null)
        {
            return false;
        }

        return group.hasConsumer(consumer);
    }


}
