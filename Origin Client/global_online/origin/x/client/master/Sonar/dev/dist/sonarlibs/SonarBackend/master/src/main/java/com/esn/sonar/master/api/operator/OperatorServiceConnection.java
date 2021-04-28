package com.esn.sonar.master.api.operator;

import com.esn.sonar.core.CommandCallback;
import com.esn.sonar.core.CommandHandler;
import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.MasterStats;
import com.esn.sonar.master.MasterMetrics;
import com.twitter.common.util.Clock;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;

import java.util.ArrayList;
import java.util.Collection;

/*
-----------------------------------------------------------------------------------------------------------------------
Sonar Operator service protocol ("OSP")

Basic request formatting:
The protocol has the concept of requests and responses via sending commands and receiving replies.
Several commands may be executed asynchronously and are always paired with a reply through the same id as the command
specified.

Command to server =>
<id><tab><command><tab><args...><lf>

Reply from server <=
<id><tab>REPLY><tab><args...><lf>

<id> = 32-bit unsigned integer
<lf> = \n (ASCII 10)
<tab> = \t (ASCII 9)

Example:
1. Open a TCP connection to the OSP port of the master server.
2. Send:
31337<tab>GET_CHANNEL_TOKEN<tab>oper<tab>uid<tab>ud<tab>cid<tab>cd<tab><tab>127.0.0.1<lf>
3. Expect reply:
31337<tab>REPLY<tab>OK<tab>SONAR2||127.0.0.1:22990|oper|uid|ud|cid|cd||1367841721|PdW5hXdrp+s8fEvTCT2yPhslAlCQjtA5x7KHzrFIfYqsHOWdPsx2vgIt6TJ3zrG43LMzs7hr7NrJDd1dl2/miQ==<lf>

-----------------------------------------------------------------------------------------------------------------------

JOIN_USER [operatorId] [userId] [channelId] [channelDesc] [location]
- Joins a user to a channel and explicitly also creates the channel. Should the user not be connected to the control
  server this method will put a pending future join request and join the user if he connects within a given time-out

[userId]        : Id of user to join
[channelId]     : Id of channel to join
[channelDesc]   : Description of channel to join
[location]      : Preferred location of channel, leave empty for default

Returns:
OK

Errors:
USER_NOT_FOUND              : The specified user is not connected or could not be found
CHANNEL_ALLOCATION_FAILED   : Allocation of the channel resource failed
INVALID_ARGUMENT            : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------

PART_USER [operatorId] [userId] [channelId] [reasonType] [reasonDesc]
- Parts (leaves) a user from the specified channel

[userId] : Id of user to part
[channelId] : Id of channel to part user from
[reasonType] : See Basic types
[reasonDesc] : See Basic types

Returns:
OK

Errors:
CHANNEL_NOT_FOUND   : The specified channel could not be found
OUT_OF_SYNC         : The channel resides on a voice server which is currently not connected to the cluster
USER_NOT_FOUND      : The user could not be found or is not connected
NOT_IN_THAT_CHANNEL : The user is in a channel but not in the channel specified
INVALID_ARGUMENT    : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------
DISCONNECT_USER [operatorId] [userId] [reasonType] [reasonDesc]
- Disconnects a user from the backend and voice server

[userId]            : Id of user to disconnect
[reasonType]        : See Basic types
[reasonDesc]        : See Basic types

Returns:
OK

Errors:
INVALID_ARGUMENT    : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------
DESTROY_CHANNEL [operatorId] [channelId] [reasonType] [reasonDesc]
- Destroys a channel and parts all users in that channel

[channelId]         : Id of channel to destroy
[reasonType]        : See Basic types
[reasonDesc]        : See Basic types

Errors:
CHANNEL_NOT_FOUND   : The specified channel could not be found
INVALID_ARGUMENT    : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------
GET_ONLINE_STATUS [operatorId] [userIds...]
- Retrieves the online status for specified userIds. The order of the input argument list is reflected in the result list
[userIds...]        : Id list of users of which to retrieve online status

Returns:
OK [userOnlineStatus...]

[userOnlineStatus]:
[userId]:[isOnline "1" or "0"]:[channelId]

Errors:
INVALID_ARGUMENT    : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------
GET_CHANNEL_USERS [operatorId] [channelId]
- Retrieves the userIds of the users in the specified channel

Returns
OK [userIds...]

Errors:
CHANNEL_NOT_FOUND   : The specified channel could not be found

-----------------------------------------------------------------------------------------------------------------------
GET_CONTROL_TOKEN [operatorId] [userId] [userDesc] [channelId] [channelDesc] [location]
- Retrieves a user control token for passing to setupInstance of the client library. If a channel argument is not empty
the returned token will also be signed to let the user join the channel as soon as connected to the backend

[userId]            : Id of user
[userDesc]          : Description of user (usually username)
[channelId]         : Id of channel to join (leave empty to only connect to backend)
[channelDesc]       : Description of channel to join
[location]          : Preferred location of channel to join, leave empty for default.

Returns:
OK [token]

Errors:
INVALID_ARGUMENT    : One or more arguments is too long or has invalid characters

-----------------------------------------------------------------------------------------------------------------------
GET_CHANNEL_TOKEN [operatorId] [userId] [userDesc] [channelId] [channelDesc] [location] [clientAddress]
- Retrieves a channel token for passing to a Sonar client nstance of the client library.

[userId]            : Id of user
[userDesc]          : Description of user (usually username)
[channelId]         : Id of channel to join (leave empty to only connect to backend)
[channelDesc]       : Description of channel to join
[location]          : Preferred location of channel to join, leave empty for default.
[clientAddress]     : IPv4 dot notation (x.x.x.x) of client IP address, used for GeoIP resolving of voice server

Returns:
OK [token]

Errors:
INVALID_ARGUMENT            : One or more arguments is too long or has invalid characters
CHANNEL_ALLOCATION_FAILED   : No voice servers are available
-----------------------------------------------------------------------------------------------------------------------

Basic types:
[userId]
A unique string identifier at most 255 bytes long. ASCII 0xb3 (PIPE) is not allowed.

[userDesc]
A plain text description of the user (usually username) at most 255 bytes long. ASCII 0xb3 (PIPE) is not allowed.

[channelId]
A unique string identifier at most 255 bytes long. ASCII 0xb3 (PIPE) is not allowed.

[channelDesc]
A plaint text description of the channel (usually it's name) at most 255 bytes long. ASCII 0xb3 (PIPE) is not allowed.

[location]
A geographic location identifier describing where a channel resource is to be allocated at most 64 bytes long. ASCII 0xb3 (PIPE) is not allowed.
Leave empty

[reasonType]
A type definition for a reason. Must be one of the following:
DISCONNECT_REQUESTED - The client in question requested through an action to be disconnected
ADMIN_KICK - The client is being kicked by an admin
ADMIN_BAN - The client is being banned by an admin
ACCESS_DENIED - The client is not allowed
CHANNEL_DESTROYED - The channel is being destroyed
CHANNEL_EXPIRED - The channel is being destroyed because it expired
LEAVING - The client is leaving (usually when parting)
DESTROYED_BY_OWNER - The channel was destroyed by the owner
UNKNOWN - Unknown reason, see reasonDesc

[reasonDesc]
A plain text description for a reason. Intended for human interaction if localization isn't required. At most 255 bytes long.

Examples:
Kicked by administrator
Kicked by channel owner
Leaving the lobby

-----------------------------------------------------------------------------------------------------------------------

Events:

EVENT_USER_CONNECTED [operatorId] [userId] [userDesc]
- A user connected to the backend control service

EVENT_USER_DISCONNECTED [operatorId] [userId] [userDesc] [reasonType] [reasonDesc]
- A user disconnected from the backend control service

EVENT_USER_CONNECTED_TO_CHANNEL [operatorId] [userId] [userDesc] [channelId] [channelDesc] [userCount]
- A user connected to a channel

EVENT_USER_DISCONNECTED_FROM_CHANNEL [operatorId] [userId] [userDesc] [channelId] [channelDesc] [userCount]
- A user disconnected from a channel

 */

public class OperatorServiceConnection extends InboundConnection
{
    private static final CommandHandler commandHandler = new CommandHandler();

    static
    {
        setupCommandHandler();
    }

    private final OperatorService operatorService;

    public OperatorServiceConnection(OperatorService operatorService)
    {
        super(false, 0, 0);
        this.operatorService = operatorService;
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        long startNanos = Clock.SYSTEM_CLOCK.nowNanos();
        super.messageReceived(ctx, e);
        long elapsedNanos = Clock.SYSTEM_CLOCK.nowNanos() - startNanos;
        MasterStats.getStatsManager().getRequestStats().requestComplete(elapsedNanos / 1000);
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        MasterStats.getStatsManager().metric(MasterMetrics.ApiOperatorFailures).incrementAndGet();
        MasterStats.getStatsManager().getRequestStats().incErrors();
        super.exceptionCaught(ctx, e);
    }

    private void sendErrorReplyWithMonitor(long msgId, String reply, MasterMetrics monitorValue)
    {
        MasterStats.getStatsManager().metric(monitorValue).incrementAndGet();
        sendErrorReply(msgId, reply);
    }


    private void handleGetUsersInChannel(Message msg)
    {
        if (msg.getArgumentCount() < 2)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String channelId = msg.getArgument(1);

        try
        {
            Collection userList = operatorService.getUsersInChannel(operatorId, channelId);
            sendSuccessReply(msg.getId(), userList);

        } catch (OperatorService.ChannelNotFoundException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.ChannelNotFound, MasterMetrics.ProtocolChannelNotFound);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }
    }

    private void handleGetUsersOnlineStatus(Message msg)
    {
        if (msg.getArgumentCount() < 2)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }


        String operatorId = msg.getArgument(0);

        Collection<String> userIdList = new ArrayList<String>(msg.getArgumentCount() - 1);

        for (int index = 1; index < msg.getArgumentCount(); index++)
        {
            userIdList.add(msg.getArgument((index)));
        }

        try
        {
            Collection statusList = operatorService.getUsersOnlineStatus(operatorId, userIdList);
            sendSuccessReply(msg.getId(), statusList);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }

    }

    private void handleDestroyChannel(Message msg)
    {
        if (msg.getArgumentCount() < 4)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String channelId = msg.getArgument(1);
        String reasonType = msg.getArgument(2);
        String reasonDesc = msg.getArgument(3);

        try
        {
            operatorService.destroyChannel(operatorId, channelId, reasonType, reasonDesc);
            sendSuccessReply(msg.getId());
        } catch (OperatorService.ChannelNotFoundException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.ChannelNotFound, MasterMetrics.ProtocolChannelNotFound);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }

    }

    private void handleDisconnectUser(Message msg)
    {
        if (msg.getArgumentCount() < 4)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String reasonType = msg.getArgument(2);
        String reasonDesc = msg.getArgument(3);

        try
        {
            operatorService.disconnectUser(operatorId, userId, reasonType, reasonDesc);
            sendSuccessReply(msg.getId());
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }
    }

    private void handlePartUserFromChannel(Message msg)
    {
        if (msg.getArgumentCount() < 5)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String channelId = msg.getArgument(2);
        String reasonType = msg.getArgument(3);
        String reasonDesc = msg.getArgument(4);

        try
        {
            operatorService.partUserFromChannel(operatorId, userId, channelId, reasonType, reasonDesc);
            sendSuccessReply(msg.getId());
        } catch (OperatorService.ChannelNotFoundException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.ChannelNotFound, MasterMetrics.ProtocolChannelNotFound);
        } catch (OperatorService.OutOfSyncException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.OutOfSync, MasterMetrics.ProtocolOutOfSync);
        } catch (OperatorService.UserNotFoundException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.UserNotFound, MasterMetrics.ProtocolUserNotFound);
        } catch (OperatorService.NotInThatChannelException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotInThatChannel, MasterMetrics.ProtocolNotInThatChannel);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }
    }

    private void handleJoinUserToChannel(Message msg)
    {
        if (msg.getArgumentCount() < 5)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String channelId = msg.getArgument(2);
        String channelDesc = msg.getArgument(3);
        String location = msg.getArgument(4);

        try
        {
            operatorService.joinUserToChannel(operatorId, location, userId, channelId, channelDesc);
            sendSuccessReply(msg.getId());
        } catch (OperatorService.ChannelAllocationFailed channelAllocationFailed)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.ChannelAllocationFailed, MasterMetrics.ProtocolChannelAllocationFailed);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        }
    }

    private void handleGetUserControlToken(Message msg)
    {
        if (msg.getArgumentCount() < 6)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String userDesc = msg.getArgument(2);
        String channelId = msg.getArgument(3);
        String channelDesc = msg.getArgument(4);
        String location = msg.getArgument(5);

        String controlToken;
        try
        {
            controlToken = operatorService.getUserControlToken(operatorId, userId, userDesc, channelId, channelDesc, location);
            sendSuccessReply(msg.getId(), controlToken);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        } catch (OperatorService.UnavailableException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.Unavailable, MasterMetrics.ProtocolUnavailable);
        }
    }

    private void handleGetChannelToken(Message msg)
    {
        if (msg.getArgumentCount() < 7)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.NotEnoughArguments, MasterMetrics.ProtocolNotEnoughArguments);
            return;
        }

        String operatorId = msg.getArgument(0);
        String userId = msg.getArgument(1);
        String userDesc = msg.getArgument(2);
        String channelId = msg.getArgument(3);
        String channelDesc = msg.getArgument(4);
        String location = msg.getArgument(5);
        String clientAddress = msg.getArgument(6);

        String channelToken;
        try
        {
            channelToken = operatorService.getChannelToken(operatorId, userId, userDesc, channelId, channelDesc, location, clientAddress);
            sendSuccessReply(msg.getId(), channelToken);
        } catch (OperatorService.InvalidArgumentException e)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.InvalidArgument, MasterMetrics.ProtocolInvalidArgument);
        } catch (OperatorService.ChannelAllocationFailed channelAllocationFailed)
        {
            sendErrorReplyWithMonitor(msg.getId(), Protocol.Errors.ChannelAllocationFailed, MasterMetrics.ProtocolChannelAllocationFailed);
        }
    }


    @Override
    public String getId()
    {
        return this.toString();
    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    @Override
    protected void onRegistrationTimeout()
    {
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        return false;
    }

    @Override
    protected boolean onMessage(Message msg) throws Exception
    {
        return commandHandler.call(this, msg);
    }

    @Override
    protected void onResult(Message msg)
    {
    }

    @Override
    protected void onChannelConnected()
    {
    }

    /**
     * Initialization of command handler, performed only once when this class is first loaded.
     */
    private static void setupCommandHandler()
    {
        commandHandler.registerCallback(Protocol.Commands.JoinUserToChannel, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleJoinUserToChannel(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.PartUserFromChannel, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handlePartUserFromChannel(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.DisconnectUser, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleDisconnectUser(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.DestroyChannel, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleDestroyChannel(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.GetUsersOnlineStatus, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleGetUsersOnlineStatus(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.GetUsersInChannel, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleGetUsersInChannel(msg);
            }
        });
        commandHandler.registerCallback(Protocol.Commands.GetUserControlToken, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((OperatorServiceConnection) instance).handleGetUserControlToken(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.GetChannelToken, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws Exception
            {
                ((OperatorServiceConnection) instance).handleGetChannelToken(msg);
            }
        });
    }
}

