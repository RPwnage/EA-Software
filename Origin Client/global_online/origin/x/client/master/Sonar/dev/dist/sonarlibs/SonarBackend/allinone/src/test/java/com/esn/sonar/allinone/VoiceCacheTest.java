package com.esn.sonar.allinone;

import com.esn.sonar.master.voice.VoiceCache;
import com.esn.sonar.master.api.event.RoundRobinEventPublisher;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import static junit.framework.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

public class VoiceCacheTest
{
    private VoiceCache cache;

    @Before
    public void start()
    {
        this.cache = new VoiceCache(new RoundRobinEventPublisher(), "operatorId");
    }

    @After
    public void stop()
    {
        this.cache = null;
    }

    @Test
    public void removeUserFromChannel()
    {
        cache.setUserInChannel("userId", "userDesc", "channelId", "channelDesc");
        cache.removeUserFromChannel("userId", "channelId");

        Collection<String> userIds = cache.getUsersInChannel("channelId");
        assertEquals(0, userIds.size());
    }

    @Test
    public void setUserInChannel()
    {
        cache.setUserInChannel("userId", "userDesc", "channelId", "channelDesc");

        Collection<String> userIds = cache.getUsersInChannel("channelId");
        assertEquals(1, userIds.size());
        assertEquals("userId", userIds.iterator().next());
    }

    @Test
    public void replaceUserInChannel()
    {
        cache.setUserInChannel("userId", "userDesc", "channelId", "channelDesc");
        Collection<String> userIds = cache.getUsersInChannel("channelId");
        assertEquals(1, userIds.size());
        assertEquals("userId", userIds.iterator().next());

        cache.setUserInChannel("userId", "userDesc", "channelId2", "channelDesc");
        userIds = cache.getUsersInChannel("channelId");
        assertEquals(0, userIds.size());

        userIds = cache.getUsersInChannel("channelId2");
        assertEquals(1, userIds.size());

        assertEquals("channelId2", cache.getChannelIdForUser("userId"));

    }


    @Test
    public void destroyChannel()
    {
        cache.setUserInChannel("userId", "userDesc", "channelId", "channelDesc");
        cache.destroyChannel("channelId");

        Collection<String> userIds = cache.getUsersInChannel("channelId");
        assertNull(userIds);
    }


    @Test
    public void getChannelIdsForUsers()
    {
        cache.setUserInChannel("user1", "userDesc", "channel1", "channelDesc");
        cache.setUserInChannel("user2", "userDesc", "channel1", "channelDesc");
        cache.setUserInChannel("user3", "userDesc", "channel2", "channelDesc");
        cache.setUserInChannel("user4", "userDesc", "channel2", "channelDesc");

        ArrayList<String> userIds = new ArrayList<String>();
        userIds.add("user1");
        userIds.add("user2");
        userIds.add("user3");
        userIds.add("user4");
        userIds.add("user5");

        Collection<String> channelIds = cache.getChannelIdsForUsers(userIds);

        Iterator<String> iterator = channelIds.iterator();

        assertEquals("channel1", iterator.next());
        assertEquals("channel1", iterator.next());
        assertEquals("channel2", iterator.next());
        assertEquals("channel2", iterator.next());
        assertEquals("", iterator.next());
    }

    @Test
    public void getUsersInChannel()
    {
        cache.setUserInChannel("user1", "", "channel1", "");
        cache.setUserInChannel("user2", "", "channel1", "");
        cache.setUserInChannel("user3", "", "channel1", "");
        cache.setUserInChannel("user4", "", "channel2", "");

        Collection<String> userIds = cache.getUsersInChannel("channel1");

        assertEquals(3, userIds.size());

        assertTrue(userIds.contains("user1"));
        assertTrue(userIds.contains("user2"));
        assertTrue(userIds.contains("user3"));
    }

    @Test
    public void getChannelIForUser()
    {
        cache.setUserInChannel("user1", "", "channel1", "");
        cache.setUserInChannel("user2", "", "channel2", "");

        assertEquals("channel1", cache.getChannelIdForUser("user1"));
        assertEquals("channel2", cache.getChannelIdForUser("user2"));
    }
}
