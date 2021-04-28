package com.esn.sonar.master.voice;

import com.esn.sonar.master.MasterServer;

import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

public class VoiceChannelManager
{
    static public class VoiceChannel
    {
        private final String channelId;
        private final String serverId;
        private final AtomicBoolean destroyed;
        private final String channelDescription;
        private int orphanCount = 0;

        private VoiceChannel(String channelId, String channelDesc, String serverId)
        {
            this.channelId = channelId;
            this.serverId = serverId;
            this.destroyed = new AtomicBoolean(false);
            this.channelDescription = channelDesc;
        }

        public String getChannelId()
        {
            return channelId;
        }

        public String getServerId()
        {
            return serverId;
        }

        public boolean isDestroyed()
        {
            return destroyed.get();
        }

        public void setAsDestroyed(boolean state)
        {
            destroyed.set(state);
        }

        public String getChannelDescription()
        {
            return channelDescription;
        }

        @Override
        public String toString()
        {
            return channelId + "|" + channelDescription;
        }

        public int increaseOrphanCount()
        {
            orphanCount++;
            return orphanCount;
        }
    }

    final ConcurrentHashMap<String, VoiceChannel> masterChannelMap = new ConcurrentHashMap<String, VoiceChannel>();

    public void clear()
    {
        masterChannelMap.clear();
    }


    /**
     * Creates a channel on the specified VoiceServer unless the channel previously exists on another voice server
     *
     * @param channelId      Id of channel to create
     * @param channelDesc    Description of channel to create
     * @param manager        A voice server manager to lookup voice servers from
     * @param newVoiceServer The voice server for the new channel (if channel doesn't exist)
     * @return
     */
    public VoiceServer createChannel(String channelId, String channelDesc, VoiceManager manager, VoiceServer newVoiceServer)
    {
        VoiceChannel newVoiceChannel = new VoiceChannel(channelId, channelDesc, newVoiceServer.getId());
        VoiceChannel currVoiceChannel = masterChannelMap.putIfAbsent(channelId, newVoiceChannel);

        if (currVoiceChannel != null)
        {
            VoiceServer currVoiceServer = manager.getVoiceServerById(currVoiceChannel.getServerId());

            if (currVoiceServer == null)
            {
                if (MasterServer.channelLog.isInfoEnabled())
                    MasterServer.channelLog.info(String.format("Creating channel %s on server %s. Channel found on dead server.", newVoiceChannel, newVoiceServer));

                masterChannelMap.put(channelId, newVoiceChannel);
                return newVoiceServer;
            }

            if (MasterServer.channelLog.isInfoEnabled()) MasterServer.channelLog.info(String.format(
                    "Creating channel %s on server %s. Channel already found.", currVoiceChannel, currVoiceServer));

            return currVoiceServer;
        }

        if (MasterServer.channelLog.isInfoEnabled())
            MasterServer.channelLog.info(String.format("Creating channel %s on server %s. Channel didn't exist.", newVoiceChannel, newVoiceServer));

        return newVoiceServer;
    }

    public enum ChannelState
    {
        NOT_FOUND,
        OTHER_SERVER,
        DESTROYED,
        ACCEPTED,
    }

    public ChannelState validateChannel(String serverId, String channelId)
    {
        VoiceChannel voiceChannel = masterChannelMap.get(channelId);

        if (voiceChannel == null)
        {
            return ChannelState.NOT_FOUND;
        }

        if (!voiceChannel.getServerId().equals(serverId))
        {
            return ChannelState.OTHER_SERVER;
        }

        if (voiceChannel.isDestroyed())
        {
            return ChannelState.DESTROYED;
        }

        return ChannelState.ACCEPTED;
    }

    public VoiceChannel setChannelAsDestroyed(String channelId)
    {
        VoiceChannel voiceChannel = masterChannelMap.get(channelId);

        if (voiceChannel == null)
        {
            return null;
        }

        if (MasterServer.channelLog.isInfoEnabled()) MasterServer.channelLog.info(String.format(
                "Marking channel %s as destroyed.", voiceChannel));

        voiceChannel.setAsDestroyed(true);
        return voiceChannel;
    }

    public VoiceChannel getVoiceChannel(String channelId)
    {
        return masterChannelMap.get(channelId);
    }

    public void unlinkChannel(String channelId)
    {

        VoiceChannel removedChannel = masterChannelMap.remove(channelId);

        if (removedChannel == null)
        {
            MasterServer.channelLog.error(String.format(
                    "Channel %s not found in master channel map.", channelId));

        } else
        {
            if (MasterServer.channelLog.isInfoEnabled()) MasterServer.channelLog.info(String.format(
                    "Unlinking channel %s.", removedChannel));
        }
    }

    public boolean isChannelUnlinkedOrDestroyed(String channelId)
    {
        VoiceChannel channel = masterChannelMap.get(channelId);

        if (channel == null)
        {
            return true;
        }

        if (channel.isDestroyed())
        {
            return true;
        }

        return false;

    }

    public int getChannelCount()
    {
        return masterChannelMap.size();
    }

    public Collection<VoiceChannel> getVoiceChannels()
    {
        return masterChannelMap.values();
    }

}
