package com.esn.sonar.master.voice;

//TODO: Don't send the additional texts for CLIENT_UNREGISTER

import com.esn.geoip.GeoGridV1;
import com.esn.geoip.GeoMapper;
import com.esn.geoip.Position;
import com.esn.geoip.Positionable;
import com.esn.geoip.util.Validator;
import com.esn.sonar.core.ConnectionManager;
import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.Operator;
import com.esn.sonar.master.OperatorManager;
import com.esn.sonar.master.user.User;

import java.security.NoSuchAlgorithmException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

public class VoiceManager extends ConnectionManager
{
    private static final Validator<Positionable> VOICE_SERVER_FREE_SLOT_VALIDATOR = new Validator<Positionable>()
    {
        public boolean verify(Positionable item)
        {
            return ((VoiceServer) item).getUserCount() < ((VoiceServer) item).getMaximumUsers();
        }
    };
    private final OperatorManager operatorManager;
    private static final int MAXIMUM_CANDIDATES = 10;
    private static Random random = new Random();

    private final GeoMapper<Positionable> geoGridHigh = new GeoGridV1(256, 256, 256 / 16);
    private final GeoMapper<Positionable> geoGridLow = new GeoGridV1(8, 8, 8);

    public Collection<VoiceServer> getVoiceServers()
    {
        return new ArrayList<VoiceServer>(serverMap.values());
    }

    private static class UserCountSorter implements Comparator<VoiceServer>
    {
        public int compare(VoiceServer o1, VoiceServer o2)
        {
            return o1.getUserCount() - o2.getUserCount();
        }
    }

    private final UserCountSorter userCountSorter = new UserCountSorter();
    private final ConcurrentMap<String, VoiceServer> serverMap = new ConcurrentHashMap<String, VoiceServer>();

    public VoiceManager(OperatorManager operatorManager) throws NoSuchAlgorithmException
    {
        super();
        this.operatorManager = operatorManager;
    }

    public VoiceServer getServerById(String serverId)
    {
        return serverMap.get(serverId);
    }


    public InboundConnection getConnectionById(String id)
    {
        return null;
    }

    @Override
    public void registerClient(InboundConnection client)
    {
    }

    @Override
    public void unregisterClient(InboundConnection client, String reasonType, String reasonDesc)
    {
    }

    public VoiceServer findServerForChannelByLocation(String address, String location)
    {
        float longitude;
        float latitude;

        if (location.startsWith("IP:"))
        {
            Position position = MasterServer.getGeoipProvider().getPosition(location.substring(3));

            longitude = position.getLongitude();
            latitude = position.getLatitude();
        } else if (location.startsWith("GP:"))
        {
            String[] args = Utils.tokenize(location.substring(3), ' ');

            longitude = Float.parseFloat(args[0]);
            latitude = Float.parseFloat(args[1]);
        } else
        {
            Position position = MasterServer.getGeoipProvider().getPosition(address);

            longitude = position.getLongitude();
            latitude = position.getLatitude();
        }


        List<Positionable> candidateList = geoGridHigh.findNearest(longitude, latitude, 5, VOICE_SERVER_FREE_SLOT_VALIDATOR);

        if (candidateList.size() == 0)
        {
            candidateList = geoGridLow.findNearest(longitude, latitude, 5, VOICE_SERVER_FREE_SLOT_VALIDATOR);

            if (candidateList.size() == 0)
            {
                MasterServer.channelLog.warn(String.format(
                        "Channel allocation for location %s/%s at (%f, %f) failed. No servers found",
                        address, location, longitude, latitude));

                return null;
            }
        }

        if (MasterServer.channelLog.isTraceEnabled()) MasterServer.channelLog.trace(String.format(
                "Allocating channel for location %s/%s at (%f, %f). Found %d candidates",
                address, location, longitude, latitude, candidateList.size()));

        VoiceServer voiceServer = (VoiceServer) candidateList.get(random.nextInt(candidateList.size()));
        voiceServer.increaseUserCount();

        return voiceServer;
    }

    public void onChannelDestroyed(String serverId, String operatorId, String channelId)
    {
        Operator operator = operatorManager.getOperator(operatorId);
        VoiceServer callingVoiceServer = serverMap.get(serverId);

        if (callingVoiceServer == null)
        {
            MasterServer.voiceEdgeLog.warn(String.format("Server %s:%s not found (onChannelDestroyed)", operatorId, serverId));
            return;
        }

        VoiceChannelManager.ChannelState channelState = operator.getChannelManager().validateChannel(callingVoiceServer.getId(), channelId);

        if (MasterServer.voiceEdgeLog.isTraceEnabled())
        {
            MasterServer.voiceEdgeLog.trace(String.format("Channel %s was destroyed on server %s => %s", channelId, serverId, channelState));
        }

        switch (channelState)
        {
            case NOT_FOUND:
                /* Channel doesn't exist anymore, that's fine we ignore this */
                break;

            case OTHER_SERVER:
                /* Channel is on another server, that's fine we ignore this */
                break;

            case ACCEPTED:
            case DESTROYED:
                /* Channel is destroyed or belongs to this server and this looks like the final confirmation of that it
                has been destroyed, either by us or by all users leaving it. Let's unlink the channel */
                operator.getChannelManager().unlinkChannel(channelId);
                operator.getCache().destroyChannel(channelId);
                break;
        }
    }

    public void onUserRegisteredToChannel(String serverId, String operatorId, String userId, String userDesc, String channelId, String channelDesc, int numClients)
    {
        Operator operator = operatorManager.getOperator(operatorId);

        VoiceServer callingVoiceServer = serverMap.get(serverId);

        if (callingVoiceServer == null)
        {
            MasterServer.voiceEdgeLog.warn(String.format("Server %s:%s not found (onUserRegisteredToChannel)", operatorId, serverId));
            return;
        }

        callingVoiceServer.setUserCount(numClients);

        VoiceChannelManager.ChannelState channelState = operator.getChannelManager().validateChannel(callingVoiceServer.getId(), channelId);

        if (MasterServer.voiceEdgeLog.isTraceEnabled())
        {
            MasterServer.voiceEdgeLog.trace(String.format("User %s is connecting to channel %s on server %s => %s", userId, channelId, serverId, channelState));
        }


        switch (channelState)
        {
            case NOT_FOUND:
            {
                VoiceServer voiceServer = operator.getChannelManager().createChannel(channelId, channelDesc, this, callingVoiceServer);

                if (voiceServer != callingVoiceServer)
                {
                    /*
                    Channel was created at another location while we attempted this, roam instead
                     */
                    callingVoiceServer.sendClientUnregister(operatorId, userId, Protocol.Reasons.MasterSync, "Calling server is not owner");
                    roamClientToChannel(operatorId, userId, userDesc, channelId, channelDesc, voiceServer);
                } else
                {
                    operator.getCache().setUserInChannel(userId, userDesc, channelId, channelDesc);
                }
                return;
            }
                /*
                // Channel doesn't exist anymore, let's destroy the channel
                callingVoiceServer.sendClientUnregister(operatorId, userId, Protocol.Reasons.MasterSync, "URTC: NOT_FOUND " + channelId);
                return;
                */


            case OTHER_SERVER:
            {
                /* Channel is on another server, let's do the lazy roaming */
                callingVoiceServer.sendClientUnregister(operatorId, userId, Protocol.Reasons.Roaming, "URTC: OTHER_SERVER");
                roamClientToChannel(operatorId, userId, userDesc, channelId, channelDesc);
                return;
            }
            case DESTROYED:
                /* Channel has been destroyed  and should not exist, we destroy it*/
                callingVoiceServer.sendClientUnregister(operatorId, userId, Protocol.Reasons.MasterSync, "URTC: DESTROYED");
                return;

            case ACCEPTED:
                /* Everything is correct, the channel should be on this voice server and the channel isn't marked as destroyed */
                break;
        }

        operator.getCache().setUserInChannel(userId, userDesc, channelId, channelDesc);

    }

    public void onUserUnregistered(String serverId, String operatorId, String userId, String channelId, int numClients)
    {
        Operator operator = operatorManager.getOperator(operatorId);

        //TODO: Expect/prove and test this to be null and then figure out what to do
        VoiceServer voiceServer = serverMap.get(serverId);

        if (voiceServer == null)
        {
            MasterServer.voiceEdgeLog.warn(String.format("Server %s:%s not found (onUserUnregistered)", operatorId, serverId));
            return;
        }

        voiceServer.setUserCount(numClients);

        VoiceChannelManager.ChannelState channelState = operator.getChannelManager().validateChannel(voiceServer.getId(), channelId);

        if (MasterServer.voiceEdgeLog.isTraceEnabled())
        {
            MasterServer.voiceEdgeLog.trace(String.format("User %s is unregistered from channel %s on server %s => %s", userId, channelId, serverId, channelState));
        }


        switch (channelState)
        {
            case NOT_FOUND:
                break;

            case OTHER_SERVER:
                /*
                Channel is on another channel, let's destroy the channel.
                NOTE: In this case we could perhaps roam remaining users but that scenario is very unlikely
                 */
                voiceServer.sendChannelDestroy(operatorId, channelId, Protocol.Reasons.MasterSync, "UUR: OTHER_SERVER");

            case DESTROYED:
                /*
                Channel has been marked as destroyed but it's on the right server so we relay it to the cache
                 */
                break;
            case ACCEPTED:
                /*
                Channel is existing on calling voice server and everything is OK
                */
                break;
        }

        operator.getCache().removeUserFromChannel(userId, channelId);

    }

    public void onStateClient(String serverId, String operatorId, String userId, String userDesc, String channelId, String channelDesc)
    {
        VoiceServer callingVoiceServer = serverMap.get(serverId);

        if (callingVoiceServer == null)
        {
            MasterServer.voiceEdgeLog.warn(String.format("Server %s:%s not found (onStateClient)", operatorId, serverId));
            return;
        }


        Operator sphere = operatorManager.getOperator(operatorId);

        callingVoiceServer.increaseUserCount();

        VoiceChannelManager.ChannelState channelState = sphere.getChannelManager().validateChannel(callingVoiceServer.getId(), channelId);

        switch (channelState)
        {
            case NOT_FOUND:
                /* Channel isn't found, since we are syncing state here we recreate the channel */
                VoiceServer voiceServer = sphere.getChannelManager().createChannel(channelId, channelDesc, this, callingVoiceServer);

                if (voiceServer != callingVoiceServer)
                {
                    /*
                    Channel was created at another location while we attempted this, roam instead
                     */
                    callingVoiceServer.sendClientUnregister(operatorId, userId, Protocol.Reasons.MasterSync, "Calling server is not owner");
                    roamClientToChannel(operatorId, userId, userDesc, channelId, channelDesc, voiceServer);
                } else
                {
                    sphere.getCache().setUserInChannel(userId, userDesc, channelId, channelDesc);
                }
                break;

            case DESTROYED:
                /*
                Not found channels
                 */
                callingVoiceServer.sendClientUnregister(sphere.getId(), userId, Protocol.Reasons.MasterSync, "Channel has been destroyed");
                break;

            case ACCEPTED:
                // Do nothing
                sphere.getCache().setUserInChannel(userId, userDesc, channelId, channelDesc);
                break;

            case OTHER_SERVER:
                /*
                Channel is existing but on other server, handle roaming here
                 */
                callingVoiceServer.sendClientUnregister(sphere.getId(), userId, Protocol.Reasons.Roaming, "Channel exists on other server");
                roamClientToChannel(sphere.getId(), userId, userDesc, channelId, channelDesc);
                break;
        }
    }

    private void roamClientToChannel(String operatorId, String userId, String userDesc, String channelId, String channelDesc, VoiceServer voiceServer)
    {
        Operator sphere = operatorManager.getOperator(operatorId);
        User user = sphere.getUserMap().get(userId);

        if (user == null)
        {
            //TODO: Log this as a roaming failed scenario
            return;
        }

        // InetSocketAddress remoteAddr = user.getConnection().getRemoteAddress();

        //TODO: We need to test a roaming scenario!
        user.sendUpdateToken(new Token(sphere.getId(), user.getUserId(), user.getUserDesc(), channelId, channelDesc, "", 0, "", 0, voiceServer.getVoipAddress(), voiceServer.getVoipPort()), Protocol.Reasons.Roam);
    }

    private void roamClientToChannel(String operatorId, String userId, String userDesc, String channelId, String channelDesc)
    {
        Operator sphere = operatorManager.getOperator(operatorId);

        VoiceChannelManager.VoiceChannel channel = sphere.getChannelManager().getVoiceChannel(channelId);

        if (channel == null)
        {
            //TODO: Log this as a roaming failed scenario
            return;
        }

        VoiceServer voiceServer = serverMap.get(channel.getServerId());

        if (voiceServer == null)
        {
            //TODO: Log this as roaming failed scenario
            return;
        }

        roamClientToChannel(operatorId, userId, userDesc, channel.getChannelId(), channel.getChannelDescription(), voiceServer);
    }


    public VoiceServer getVoiceServerById(String serverId)
    {
        return serverMap.get(serverId);
    }

    public VoiceServer mapClientToServer(String serverId, String voipAddress, int port, int maxClients, VoiceEdgeConnection edgeConnection)
    {
        Position pos = MasterServer.getGeoipProvider().getPosition(voipAddress);

        VoiceServer edgeVoiceServer = new VoiceServer(edgeConnection, pos, serverId, voipAddress, port, maxClients);
        VoiceServer oldVoiceServer = serverMap.put(serverId, edgeVoiceServer);

        /*
        If the user is a returning user and he's returning from the same edge connection, let's not send a
        LoggedInElsewhere request back to his edge, his edge won't be able to tell the two connections apart and
        will diconnect the new one
        */

        if (oldVoiceServer != null && oldVoiceServer.getConnection() != edgeVoiceServer.getConnection())
        {
            oldVoiceServer.sendEdgeUnregisterClient(Protocol.Reasons.LoggedInElsewhere, "");
        }

        geoGridHigh.mapObject(edgeVoiceServer);
        geoGridLow.mapObject(edgeVoiceServer);

        return edgeVoiceServer;
    }

    public VoiceServer unmapClientToServer(String serverId, String reasonType, String reasonDesc)
    {
        VoiceServer voiceServer = serverMap.remove(serverId);

        if (voiceServer == null)
        {
            MasterServer.masterLog.warn(String.format("Voice server %s wasn't found upon unregistration", serverId));
            return null;
        }

        geoGridHigh.unmapObject(voiceServer);
        geoGridLow.unmapObject(voiceServer);

        return voiceServer;
    }

    public int getServerCount()
    {
        return serverMap.size();
    }

}

