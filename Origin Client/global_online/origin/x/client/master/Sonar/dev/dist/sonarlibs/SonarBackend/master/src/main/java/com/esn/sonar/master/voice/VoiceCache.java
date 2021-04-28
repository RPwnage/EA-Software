package com.esn.sonar.master.voice;

import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.api.event.EventPublisher;
import com.esn.sonar.master.api.operator.OperatorService;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class VoiceCache
{

    private EventPublisher eventPublisher;

    private static class VoiceUser implements Comparable<VoiceUser>
    {
        public int compareTo(VoiceUser o)
        {
            return this.userId.compareTo(o.userId);
        }

        private final VoiceChannel channel;
        private final String userId;
        private final String userDesc;

        public VoiceUser(String userId, String userDesc, VoiceChannel voiceChannel)
        {
            this.userDesc = userDesc;
            this.userId = userId;
            this.channel = voiceChannel;
        }

        public VoiceChannel getChannel()
        {
            return channel;
        }

        public String getUserId()
        {
            return userId;
        }

        public String getUserDesc()
        {
            return userDesc;
        }

        @Override
        public String toString()
        {
            return userId;
        }
    }


    private static class VoiceChannel
    {
        private final String channelId;
        private final String channelDesc;
        private final Set<VoiceUser> userSet = new HashSet<VoiceUser>();

        public VoiceChannel(String channelId, String channelDesc)
        {
            this.channelDesc = channelDesc;
            this.channelId = channelId;
        }

        public String getChannelDesc()
        {
            return channelDesc;
        }

        public String getChannelId()
        {
            return channelId;
        }

        synchronized public Collection<VoiceUser> getUsers()
        {
            return new ArrayList<VoiceUser>(userSet);
        }

        synchronized public int getUserCount()
        {
            return userSet.size();
        }

        synchronized public void addUser(VoiceUser voiceUser)
        {
            if (!userSet.add(voiceUser))
            {
                throw new RuntimeException("User already in channel");
            }
        }

        synchronized public void removeUser(VoiceUser voiceUser)
        {
            if (!userSet.remove(voiceUser))
            {
                throw new RuntimeException("User not found channel");
            }
        }

        @Override
        public String toString()
        {
            return channelId;
        }
    }

    private final ConcurrentHashMap<String, VoiceChannel> channelMap = new ConcurrentHashMap<String, VoiceChannel>();
    private final ConcurrentHashMap<String, VoiceUser> userMap = new ConcurrentHashMap<String, VoiceUser>();
    private final String operatorId;

    public void clear()
    {
        channelMap.clear();
        userMap.clear();
    }

    public VoiceCache(EventPublisher eventPublisher, String operatorId)
    {
        this.eventPublisher = eventPublisher;
        this.operatorId = operatorId;
    }

    public void removeUserFromChannel(String userId, String channelId)
    {
        VoiceUser voiceUser;

        voiceUser = userMap.remove(userId);

        if (voiceUser == null)
        {
            MasterServer.userLog.info(
                    String.format("User %s not found in cache", userId));
            return;
        }

        VoiceChannel voiceChannel = voiceUser.getChannel();
        voiceChannel.removeUser(voiceUser);


        eventPublisher.publishEvent(
                new OperatorService.UserDisconnectedFromChannelEvent(
                        operatorId,
                        voiceUser.getUserId(),
                        voiceUser.getUserDesc(),
                        voiceUser.getChannel().getChannelId(),
                        voiceUser.getChannel().getChannelDesc(),
                        voiceUser.getChannel().getUserCount()
                ));

        if (MasterServer.userLog.isTraceEnabled()) MasterServer.userLog.trace(String.format(
                "Removing user %s from channel %s", voiceUser, voiceChannel));
    }

    public void setUserInChannel(String userId, String userDesc, String channelId, String channelDesc)
    {
        VoiceUser oldVoiceUser;
        VoiceUser voiceUser;

        VoiceChannel newVoiceChannel = new VoiceChannel(channelId, channelDesc);
        VoiceChannel voiceChannel = channelMap.putIfAbsent(channelId, newVoiceChannel);

        if (voiceChannel == null)
        {
            voiceChannel = newVoiceChannel;
            if (MasterServer.channelLog.isTraceEnabled()) MasterServer.channelLog.trace(String.format(
                    "Adding channel %s", voiceChannel));

        }

        voiceUser = new VoiceUser(userId, userDesc, voiceChannel);
        oldVoiceUser = userMap.put(userId, voiceUser); // 1)


        if (oldVoiceUser != null)
        {
            oldVoiceUser.getChannel().removeUser(oldVoiceUser);
        }

        voiceChannel.addUser(voiceUser); // 2)

        //NOTE: Concurrency issue


        if (oldVoiceUser != null)
        {
            eventPublisher.publishEvent(
                    new OperatorService.UserDisconnectedFromChannelEvent(
                            operatorId,
                            oldVoiceUser.getUserId(),
                            oldVoiceUser.getUserDesc(),
                            oldVoiceUser.getChannel().getChannelId(),
                            oldVoiceUser.getChannel().getChannelDesc(),
                            oldVoiceUser.getChannel().getUserCount()
                    ));
        }

        eventPublisher.publishEvent(
                new OperatorService.UserConnectedToChannelEvent(
                        operatorId,
                        voiceUser.getUserId(),
                        voiceUser.getUserDesc(),
                        voiceUser.getChannel().getChannelId(),
                        voiceUser.getChannel().getChannelDesc(),
                        voiceUser.getChannel().getUserCount()
                ));

        if (MasterServer.userLog.isTraceEnabled())
            MasterServer.userLog.trace(String.format("Adding user %s to channel %s", voiceUser, voiceChannel));

    }


    public void destroyChannel(String channelId)
    {
        VoiceChannel voiceChannel = channelMap.remove(channelId);

        if (voiceChannel == null)
        {
            MasterServer.channelLog.trace(String.format(
                    "Unable to remove channel %s. Channel not found", channelId));

            return;
        } else
        {
            if (MasterServer.channelLog.isTraceEnabled()) MasterServer.channelLog.trace(String.format(
                    "Removing channel %s", voiceChannel));
        }

        Collection<VoiceUser> users = voiceChannel.getUsers();

        for (VoiceUser user : users)
        {
            userMap.remove(user.getUserId());
        }


        eventPublisher.publishEvent(
                new OperatorService.ChannelDestroyedEvent(
                        operatorId,
                        channelId));
    }

    public Collection<String> getChannelIdsForUsers(Collection<String> userIds)
    {
        ArrayList<String> retList = new ArrayList<String>(userIds.size());

        for (String userId : userIds)
        {
            VoiceUser voiceUser = userMap.get(userId);

            String channelId = "";

            if (voiceUser != null)
            {
                channelId = voiceUser.getChannel().getChannelId();
            }

            retList.add(channelId);
        }

        return retList;
    }

    public Collection<String> getUsersInChannel(String channelId)
    {
        VoiceChannel channel = channelMap.get(channelId);

        if (channel == null)
        {
            return null;
        }

        Collection<VoiceUser> users = channel.getUsers();
        ArrayList<String> retList = new ArrayList<String>(users.size());

        for (VoiceUser user : users)
        {
            retList.add(user.getUserId());
        }

        return retList;
    }

    public String getChannelIdForUser(String userId)
    {
        VoiceUser user = userMap.get(userId);

        if (user == null)
        {
            return null;
        }

        return user.getChannel().getChannelId();
    }

    public int getChannelCount()
    {
        return channelMap.size();
    }

    public int getUserCount()
    {
        return userMap.size();
    }
}
