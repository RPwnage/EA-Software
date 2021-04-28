package com.esn.sonartest.MasterTest;

import java.util.HashSet;
import java.util.Set;

/**
 * User: ronnie
 * Date: 2011-06-17
 * Time: 11:19
 */
public class Channel
{
    private final String channelId;
    private final String channelDesc;
    private final Set<User> userSet = new HashSet<User>();
    private volatile long index;

    public Channel(String channelId, String channelDesc)
    {
        this.channelId = channelId;
        this.channelDesc = channelDesc;
    }

    public String getChannelId()
    {
        return channelId;
    }

    public String getChannelDesc()
    {
        return channelDesc;
    }

    public synchronized int getClientCount()
    {
        return userSet.size();
    }

    public synchronized void addUser(User user)
    {
        if (!userSet.add(user))
        {
            throw new RuntimeException();
        }
    }

    public synchronized void removeUser(User user)
    {
        if (!userSet.remove(user))
        {
            throw new RuntimeException();
        }
    }

    public long getIndex()
    {
        return this.index;
    }

    public void setIndex(long index)
    {
        this.index = index;
    }

    public synchronized User[] getUsers()
    {
        return userSet.toArray(new User[userSet.size()]);
    }
}
