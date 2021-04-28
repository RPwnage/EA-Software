package com.esn.sonar.master.api.operator;

import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import com.esn.sonar.master.MasterConfig;
import com.esn.sonar.master.MasterServer;
import com.esn.sonar.master.MasterStats;
import com.esn.sonar.master.MasterMetrics;
import com.esn.sonar.master.Operator;
import com.esn.sonar.master.OperatorManager;
import com.esn.sonar.master.user.User;
import com.esn.sonar.master.user.UserEdgeConnection;
import com.esn.sonar.master.user.UserManager;
import com.esn.sonar.master.voice.VoiceChannelManager;
import com.esn.sonar.master.voice.VoiceManager;
import com.esn.sonar.master.voice.VoiceServer;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import static com.esn.sonar.core.util.Utils.validateArgument;

public class OperatorServiceImpl implements OperatorService
{
    private final VoiceManager voiceManager;
    private final OperatorManager operatorManager;
    private final UserManager userManager;

    public OperatorServiceImpl(OperatorManager operatorManager, VoiceManager voiceManager, UserManager userManager, MasterConfig config)
    {
        this.operatorManager = operatorManager;
        this.voiceManager = voiceManager;
        this.userManager = userManager;
    }

    public void destroyChannel(String operatorId, String channelId, String reasonType, String reasonDesc) throws ChannelNotFoundException, InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorDestroyChannel).incrementAndGet();

        if (reasonType.length() > Protocol.Limits.reasonTypeLength) throw new InvalidArgumentException("reasonType");
        if (reasonDesc.length() > Protocol.Limits.reasonDescLength) throw new InvalidArgumentException("reasonDesc");

        Operator operator = operatorManager.getOperator(operatorId);

        VoiceChannelManager.VoiceChannel channel = operator.getChannelManager().setChannelAsDestroyed(channelId);

        if (channel == null)
        {
            throw new ChannelNotFoundException(channelId);
        }

        channel.setAsDestroyed(true);

        VoiceServer voiceServer = voiceManager.getVoiceServerById(channel.getServerId());

        if (voiceServer == null)
        {
            return;
        }

        voiceServer.sendChannelDestroy(operatorId, channelId, reasonType, reasonDesc);

    }

    public Collection<UserOnlineStatus> getUsersOnlineStatus(String operatorId, Collection<String> userIds) throws InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorGetUsersOnlineStatus).incrementAndGet();

        Operator sphere = operatorManager.getOperator(operatorId);

        Collection<String> channelIds = sphere.getCache().getChannelIdsForUsers(userIds);
        ArrayList<UserOnlineStatus> statusList = new ArrayList<UserOnlineStatus>(channelIds.size());

        Iterator<String> cidIter = channelIds.iterator();

        for (String userId : userIds)
        {
            if (!Utils.validateArgument(userId, Protocol.Limits.userIdLength))
                throw new InvalidArgumentException("userId");

            boolean isOnline = true;
            String channelId = cidIter.next();

            if (sphere.getUserMap().get(userId) == null)
            {
                isOnline = false;
                channelId = "";
            }

            statusList.add(new UserOnlineStatus(userId, isOnline, channelId));
        }


        return statusList;
    }

    public Collection<String> getUsersInChannel(String operatorId, String channelId) throws ChannelNotFoundException, InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorGetUsersInChannel).incrementAndGet();

        Operator sphere = operatorManager.getOperator(operatorId);

        Collection userIds = sphere.getCache().getUsersInChannel(channelId);

        if (sphere.getChannelManager().isChannelUnlinkedOrDestroyed(channelId))
        {
            throw new ChannelNotFoundException(channelId);
        }

        if (userIds == null)
        {
            /* Cache is supposedly wrong, let's return an empty list */
            return new ArrayList<String>(0);
        }

        return userIds;
    }

    public String getUserControlToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location) throws InvalidArgumentException, UnavailableException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorGetUserControlToken).incrementAndGet();

        if (!Utils.validateArgument(location, Protocol.Limits.locationLength))
            throw new InvalidArgumentException("location");
        if (!Utils.validateArgument(userId, Protocol.Limits.userIdLength)) throw new InvalidArgumentException("userId");
        if (!Utils.validateArgument(userDesc, Protocol.Limits.userDescLength))
            throw new InvalidArgumentException("userDesc");
        if (!Utils.validateArgument(channelId, Protocol.Limits.channelIdLength))
            throw new InvalidArgumentException("channelId");
        if (!Utils.validateArgument(channelDesc, Protocol.Limits.channelDescLength))
            throw new InvalidArgumentException("channelDesc");
        if (!Utils.validateArgument(operatorId, Protocol.Limits.operatorIdLength))
            throw new InvalidArgumentException("operatorId");

        UserEdgeConnection randomClient = userManager.getRandomClient();

        if (randomClient == null)
        {
            throw new UnavailableException("No user edge servers are available");
        }


        Token token = new Token(operatorId, userId, userDesc, channelId, channelDesc, location, 0, randomClient.getPublicAddress(), randomClient.getPublicPort(), "", 0);
        return token.sign(MasterServer.getInstance().getKeyPair().getPrivate());
    }

    public String getChannelToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location, String clientAddress) throws InvalidArgumentException, ChannelAllocationFailed
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorGetChannelToken).incrementAndGet();

        if (!Utils.validateArgument(clientAddress, Protocol.Limits.addressLength))
            throw new InvalidArgumentException("clientAddress");
        if (!Utils.validateArgument(location, Protocol.Limits.locationLength))
            throw new InvalidArgumentException("location");
        if (!Utils.validateArgument(userId, Protocol.Limits.userIdLength))
            throw new InvalidArgumentException("userId");
        if (!Utils.validateArgument(userDesc, Protocol.Limits.userDescLength))
            throw new InvalidArgumentException("userDesc");
        if (!Utils.validateArgument(channelId, Protocol.Limits.channelIdLength))
            throw new InvalidArgumentException("channelId");
        if (!Utils.validateArgument(channelDesc, Protocol.Limits.channelDescLength))
            throw new InvalidArgumentException("channelDesc");
        if (!Utils.validateArgument(operatorId, Protocol.Limits.operatorIdLength))
            throw new InvalidArgumentException("operatorId");

        Token token = getTokenForChannelId(clientAddress, location, operatorId, userId, userDesc, channelId, channelDesc);
        return token.sign(MasterServer.getInstance().getKeyPair().getPrivate());
    }


    public void joinUserToChannel(String operatorId, String location, String userId, String channelId, String channelDesc) throws ChannelAllocationFailed, InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorJoinUserToChannel).incrementAndGet();

        Operator sphere = operatorManager.getOperator(operatorId);

        User user = sphere.getUserMap().get(userId);

        if (!validateArgument(location, Protocol.Limits.locationLength)) throw new InvalidArgumentException("location");
        if (!validateArgument(userId, Protocol.Limits.userIdLength)) throw new InvalidArgumentException("userId");
        if (!validateArgument(channelId, Protocol.Limits.channelIdLength))
            throw new InvalidArgumentException("channelId");
        if (!validateArgument(channelDesc, Protocol.Limits.channelDescLength))
            throw new InvalidArgumentException("channelDesc");
        if (!validateArgument(operatorId, Protocol.Limits.operatorIdLength))
            throw new InvalidArgumentException("operatorId");

        if (user != null)
        {
            Token token = getTokenForChannelId(user.getRemoteAddress(), location, sphere.getId(), user.getUserId(), user.getUserDesc(), channelId, channelDesc);

            user.sendUpdateToken(token, Protocol.Reasons.Join);
            return;
        }

        // Client isn't here yet, let's save a join channel future
        userManager.setupJoinFuture(sphere.getId(), userId, channelId, channelDesc, location);
    }

    public void disconnectUser(String operatorId, String userId, String reasonType, String reasonDesc) throws InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorDisconnectUser).incrementAndGet();

        if (operatorId.length() > Protocol.Limits.operatorIdLength) throw new InvalidArgumentException("operatorId");
        if (reasonType.length() > Protocol.Limits.reasonTypeLength) throw new InvalidArgumentException("reasonType");
        if (reasonDesc.length() > Protocol.Limits.reasonDescLength) throw new InvalidArgumentException("reasonDesc");

        Operator sphere = operatorManager.getOperator(operatorId);

        User user = sphere.getUserMap().get(userId);

        if (user != null)
        {
            user.sendUnregisterClient(reasonType, reasonDesc);
        }
    }

    public void partUserFromChannel(String operatorId, String userId, String channelId, String reasonType, String reasonDesc) throws UserNotFoundException, NotInThatChannelException, ChannelNotFoundException, OutOfSyncException, InvalidArgumentException
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorPartUserFromChannel).incrementAndGet();

        if (operatorId.length() > Protocol.Limits.operatorIdLength) throw new InvalidArgumentException("operatorId");
        if (reasonType.length() > Protocol.Limits.reasonTypeLength) throw new InvalidArgumentException("reasonType");
        if (reasonDesc.length() > Protocol.Limits.reasonDescLength) throw new InvalidArgumentException("reasonDesc");

        Operator operator = operatorManager.getOperator(operatorId);

        VoiceChannelManager.VoiceChannel channel = operator.getChannelManager().getVoiceChannel(channelId);

        if (channel == null)
        {
            throw new ChannelNotFoundException(channelId);
        }

        String currChannelId = operator.getCache().getChannelIdForUser(userId);

        if (currChannelId == null || !currChannelId.equals(channelId))
        {
            throw new NotInThatChannelException(channelId);
        }

        VoiceServer server = voiceManager.getVoiceServerById(channel.getServerId());

        if (server == null)
        {
            throw new OutOfSyncException("The voice server which the user is connected to could not be reached");
        }

        server.sendClientUnregister(operatorId, userId, reasonType, reasonDesc);
    }

    private Token getTokenForChannelId(String clientAddress, String location, String operatorId, String userId, String userDesc, String channelId, String channelDesc) throws ChannelAllocationFailed, InvalidArgumentException
    {
        Operator sphere = operatorManager.getOperator(operatorId);

        VoiceServer newVoiceServer = voiceManager.findServerForChannelByLocation(clientAddress, location);
        if (newVoiceServer == null)
            throw new ChannelAllocationFailed("No server available for channel allocation");

        newVoiceServer = sphere.getChannelManager().createChannel(channelId, channelDesc, voiceManager, newVoiceServer);

        return new Token(sphere.getId(), userId, userDesc, channelId, channelDesc, "", 0, "", 0, newVoiceServer.getVoipAddress(), newVoiceServer.getVoipPort());
    }
}
