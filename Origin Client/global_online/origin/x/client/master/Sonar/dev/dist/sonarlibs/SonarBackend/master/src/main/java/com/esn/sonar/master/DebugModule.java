package com.esn.sonar.master;


import com.esn.sonar.master.api.operator.OperatorService;

import java.util.*;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-09-20
 * Time: 17:32
 * To change this template use File | Settings | File Templates.
 */
public class DebugModule implements Runnable
{
    Thread thread;
    volatile boolean isRunning = false;
    private Operator operator;
    private double elapsed;
    private static Random random = new Random(31337);

    private class Channel
    {
        private Set<User> userSet = new HashSet<User>();
        private long channelId;

        public Channel(long channelId)
        {
            this.channelId = channelId;
        }

        public void removeUser(User user)
        {
            if (!userSet.remove(user))
            {
                throw new IllegalStateException("User was not in channel " + this);
            }
        }

        public String getChannelId()
        {
            return Long.toHexString(channelId);
        }

        public String getChannelDesc()
        {
            return "Description of " + getChannelId();
        }

        public int getUserCount()
        {
            return userSet.size();
        }

        public long getChannelIdNumber()
        {
            return channelId;
        }

        public void addUser(User user)
        {
            if (!userSet.add(user))
            {
                throw new IllegalStateException("User already in channel");
            }
        }
    }

    private class User
    {
        private double loginTime;
        private double logoutTime;
        private long userId;
        private Channel channel;

        public User(long userId)
        {
            this.userId = userId;
            this.loginTime = DebugModule.this.elapsed;
            this.logoutTime = DebugModule.this.elapsed + config.getDebugStayTime() + (long) random.nextInt(config.getDebugStayTime().intValue());
        }

        String getUserId()
        {
            return Long.toString(userId);
        }

        public double getLogoutTime()
        {
            return logoutTime;
        }

        public long getUserIdNumber()
        {
            return userId;
        }

        public Channel getChannel()
        {
            return channel;
        }

        public String getUserDesc()
        {
            return config.getDebugUserDescPrefix() + getUserId();
        }


        public void setChannel(Channel channel)
        {
            this.channel = channel;
        }
    }

    Deque<Long> availableUsers = new LinkedList<Long>();
    Deque<Long> availableChannels = new LinkedList<Long>();
    Deque<User> loginOrder = new LinkedList<User>();

    Set<User> onlineUsers = new HashSet<User>();
    Set<User> usersInChannelSet = new HashSet<User>();
    Deque<User> usersInChannelList = new LinkedList<User>();
    Set<User> usersNotInChannels = new HashSet<User>();
    Map<Long, Channel> onlineChannels = new HashMap<Long, Channel>();
    double eventCount = 0.0;

    private MasterConfig config;

    public DebugModule(MasterConfig config)
    {
        this.config = config;
        this.operator = MasterServer.getInstance().getOperatorManager().getOperator(config.getDebugOperatorId());

    }


    public void start()
    {
        thread = new Thread(this);
        thread.start();
        isRunning = true;

    }

    public void stop() throws InterruptedException
    {
        isRunning = false;
        thread.join();
    }

    public void run()
    {
        long userCount = 0;

        for (long userId = config.getDebugClientStart(); userId < config.getDebugClientEnd(); userId++)
        {
            availableUsers.push(userId);
            userCount++;
        }

        for (long channelId = 0; channelId < config.getDebugChannelCount(); channelId++)
        {
            availableChannels.addLast(channelId);
        }

        long tsStart = System.currentTimeMillis();

        double printOut = elapsed + 1.0;


        while (true)
        {
            this.elapsed = ((System.currentTimeMillis() - tsStart)) / 1000.0;
            if (eventCount >= this.elapsed * (double) config.getDebugEventsPerSecond())
            {
                try
                {
                    Thread.sleep(1);
                } catch (InterruptedException e)
                {
                }

                continue;
            }

            // Login users
            if (onlineUsers.size() < config.getDebugClientsOnline())
            {
                Long userId = availableUsers.removeFirst();
                User user = new User(userId);
                onlineUsers.add(user);
                loginOrder.addLast(user);
                usersNotInChannels.add(user);

                operator.getEventPublisher().publishEvent(new OperatorService.UserConnectedEvent(
                        config.getDebugOperatorId(),
                        user.getUserId(),
                        user.getUserDesc()));

                eventCount++;
                continue;
            }

            // Logout users
            if (onlineUsers.size() >= config.getDebugClientsOnline())
            {
                Iterator<User> iterator = loginOrder.iterator();

                int actions = 0;

                while (iterator.hasNext())
                {
                    User user = iterator.next();

                    if (elapsed < user.getLogoutTime())
                    {
                        break;
                    }

                    if (usersInChannelSet.contains(user))
                    {
                        continue;
                    }

                    iterator.remove();

                    logoutUser(user);
                    actions++;
                }

                if (actions > 0)
                {
                    continue;
                }
            }

            // Join
            if (usersInChannelSet.size() < onlineUsers.size())
            {
                Iterator<User> iterator = usersNotInChannels.iterator();
                User user = iterator.next();
                joinUser(user);
                continue;
            }

            // Part
            if (usersInChannelList.size() > 0)
            {
                User user = usersInChannelList.removeFirst();

                partUser(user);

            }


        }

    }

    private void joinUser(User user)
    {
        Channel channel;

        if (availableChannels.size() > 0)
        {
            long channelId = availableChannels.removeFirst();
            channel = new Channel(channelId);

            onlineChannels.put(channel.getChannelIdNumber(), channel);
        } else
        {
            int loops = 0;

            while (true)
            {
                channel = onlineChannels.get(random.nextLong() % config.getDebugChannelCount());

                if (channel != null)
                {
                    break;
                }

                loops++;

                if (loops > 1000)
                    throw new RuntimeException("Bad algorithm detected");


            }

        }

        channel.addUser(user);

        user.setChannel(channel);

        if (!usersNotInChannels.remove(user))
        {
            throw new IllegalStateException();
        }

        if (!usersInChannelSet.add(user))
        {
            throw new IllegalStateException();
        }

        usersInChannelList.addLast(user);

        operator.getCache().setUserInChannel(user.getUserId(), user.getUserDesc(), channel.getChannelId(), channel.getChannelDesc());

        eventCount++;


    }

    private void logoutUser(User user)
    {
        if (!onlineUsers.remove(user))
        {
            throw new IllegalStateException("User not found " + user);
        }

        availableUsers.addLast(user.getUserIdNumber());

        if (usersInChannelSet.contains(user))
        {
            throw new IllegalStateException("Must part channel first");
        }

        operator.getEventPublisher().publishEvent(new OperatorService.UserDisconnectedEvent(
                config.getDebugOperatorId(),
                user.getUserId(),
                user.getUserDesc()));

        eventCount++;

    }

    private void partUser(User user)
    {
        Channel channel = user.getChannel();

        channel.removeUser(user);

        operator.getCache().removeUserFromChannel(user.getUserId(), channel.getChannelId());

        eventCount++;

        if (channel.getUserCount() == 0)
        {
            availableChannels.addLast(channel.getChannelIdNumber());
            if (onlineChannels.remove(channel.getChannelIdNumber()) != channel)
            {
                throw new IllegalStateException("Channel not found as online " + channel);
            }

            operator.getCache().destroyChannel(channel.getChannelId());

            eventCount++;

        }

        if (!usersInChannelSet.remove(user))
        {
            throw new IllegalStateException("User not in channel as expected 2" + user);
        }

        if (!usersNotInChannels.add(user))
        {
            throw new IllegalStateException("User not in channel as expected 3" + user);
        }

    }

}
