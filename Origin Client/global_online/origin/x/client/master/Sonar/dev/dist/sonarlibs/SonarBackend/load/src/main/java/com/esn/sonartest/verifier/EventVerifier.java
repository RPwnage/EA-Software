package com.esn.sonartest.verifier;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.api.event.EventConsumer;
import com.esn.sonar.master.api.event.EventPublisher;
import com.esn.sonartest.MasterTest.Channel;
import com.esn.sonartest.MasterTest.User;

/**
 * User: ronnie
 * Date: 2011-06-17
 * Time: 11:27
 */
public class EventVerifier extends Verifier<String>
{
    private static final EventVerifier instance = new EventVerifier();
    public static final String EMPTY_STRING = "";
    public static final String KEY_DELIMITER = "|";
    public static final String KEY_PREFIX_USER_REGISTERED_TO_CHANNEL = "JOIN";
    public static final String KEY_PREFIX_USER_DISCONNECTED_FROM_CHANNEL = "PART";
    public static final String KEY_PREFIX_CHANNEL_DESTROYED = "DESTROY";
    private static final String KEY_PREFIX_USER_CONNECT = "CONNECT";
    private static final String KEY_PREFIX_USER_DISCONNECT = "DISCONNECT";

    public static class NullEventPublisher implements EventPublisher
    {
        public void addConsumer(EventConsumer consumer)
        {
        }

        public void removeConsumer(EventConsumer consumer)
        {
        }

        public void publishEvent(Message event)
        {
        }

        public boolean hasConsumer(EventConsumer consumer)
        {
            return false;
        }
    }

    private static final ThreadLocal<StringBuilder> keyBuilder = new ThreadLocal<StringBuilder>()
    {
        @Override
        protected StringBuilder initialValue()
        {
            return new StringBuilder(256);
        }
    };

    public static EventVerifier getInstance()
    {
        return instance;
    }

    /**
     * Invoked by EventClient(s) as events are received
     *
     * @param msg the event
     */

    public void onEvent(Message msg)
    {
        Expectation expectation = null;

        if (msg.getName().equals(Protocol.Events.UserConnectedToChannel))
        {
            // operatorId, userId, userDesc, channelId, channelDesc, Integer.toString(channelUserCount)

            String key = createKey(KEY_PREFIX_USER_REGISTERED_TO_CHANNEL, msg.getArgument(1), msg.getArgument(3));
            expectation = aquireExpectation(key);
        } else if (msg.getName().equals(Protocol.Events.UserDisconnectedFromChannel))
        {
            // operatorId, userId, userDesc, channelId, channelDesc, Integer.toString(channelUserCount)

            String key = createKey(KEY_PREFIX_USER_DISCONNECTED_FROM_CHANNEL, msg.getArgument(1), msg.getArgument(3));
            expectation = aquireExpectation(key);

            if (expectation == null)
            {
                return;
            }
        } else if (msg.getName().equals(Protocol.Events.ChannelDestroyed))
        {
            // operatorId, userId, userDesc, channelId, channelDesc, Integer.toString(channelUserCount)
            String key = createKey(KEY_PREFIX_CHANNEL_DESTROYED, msg.getArgument(1));
            expectation = aquireExpectation(key);
        } else if (msg.getName().equals(Protocol.Events.UserConnected))
        {
            // operatorId, userId, userDesc
            String key = createKey(KEY_PREFIX_USER_CONNECT, msg.getArgument(1));
            expectation = aquireExpectation(key);
        } else if (msg.getName().equals(Protocol.Events.UserDisconnected))
        {
            // operatorId, userId, userDesc
            String key = createKey(KEY_PREFIX_USER_DISCONNECT, msg.getArgument(1));
            expectation = aquireExpectation(key);
        }

        // Check timestamp
        long elapsedMillis = expectation.checkTimeout();

        // Check expectation
        expectation.verify(msg);
    }

    private boolean findExpectation(String key)
    {
        return expectations.containsKey(key);
    }

    public void addUserShouldBeInChannelExpectation(final User user, final Channel channel, Expectation expectation)
    {
        String key = createKey(KEY_PREFIX_USER_REGISTERED_TO_CHANNEL, user.getUserId(), channel.getChannelId());
        addExpectation(key, expectation);
    }

    public void addUserShouldLeaveChannelExpectation(final User user, final Channel channel, Expectation expectation)
    {
        String key = createKey(KEY_PREFIX_USER_DISCONNECTED_FROM_CHANNEL, user.getUserId(), channel.getChannelId());
        addExpectation(key, expectation);
    }

    public void addChannelShouldBeDestroyed(Channel channel, Expectation expectation)
    {
        String key = createKey(KEY_PREFIX_CHANNEL_DESTROYED, channel.getChannelId());
        addExpectation(key, expectation);
    }

    public void addUserShouldConnect(User user, Expectation expectation)
    {
        String key = createKey(KEY_PREFIX_USER_CONNECT, user.getUserId());
        addExpectation(key, expectation);
    }

    public void addUserShouldDisconnect(User user, Expectation expectation)
    {
        String key = createKey(KEY_PREFIX_USER_DISCONNECT, user.getUserId());
        addExpectation(key, expectation);
    }



    /*
        Private helpers
     */

    /**
     * Adds an expectation.
     *
     * @param key         the hash key
     * @param expectation the expectation to store
     * @throws RuntimeException An exception is thrown if the key is already associated with an expectation.
     * @see #createKey(String...)
     */
    private void addExpectation(String key, Expectation expectation) throws RuntimeException
    {
        if (expectations.put(key, expectation) != null)
        {
            throw new RuntimeException("There already exists an expectation for given key (" + key + ")");
        }
    }

    /**
     * Pulls an expectation out of the internal store.
     *
     * @param key the hash key
     * @return the associated expectation or null
     * @see #createKey(String...)
     */
    private Expectation aquireExpectation(String key) throws RuntimeException
    {
        return expectations.remove(key);
    }

    /**
     * Creates a concatinated and TAB separated String from the given arguments
     *
     * @param args the arguments to create key for
     * @return a new key
     */
    private String createKey(String... args)
    {
        StringBuilder sb = keyBuilder.get();
        sb.setLength(0);
        String prepend = EMPTY_STRING;
        for (String arg : args)
        {
            sb.append(prepend);
            prepend = KEY_DELIMITER;
            sb.append(arg);
        }
        return sb.toString();
    }
}
