package com.esn.sonar.master;

/**
 * User: ronnie
 * Date: 2011-07-06
 * Time: 15:14
 */
public enum MasterMetrics
{
    ApiOperatorJoinUserToChannel,
    ApiOperatorDisconnectUser,
    ApiOperatorPartUserFromChannel,
    ApiOperatorDestroyChannel,
    ApiOperatorGetUsersOnlineStatus,
    ApiOperatorGetUsersInChannel,
    ApiOperatorGetUserControlToken,
    ApiOperatorFailures,

    ApiEventChannelDestroyedEvent,
    ApiEventUserConnectedEvent,
    ApiEventUserDisconnectedEvent,
    ApiEventUserDisconnectedFromChannelEvent,
    ApiEventUserConnectedToChannelEvent,

    ProtocolNotEnoughArguments,
    ProtocolInvalidArgument,
    ProtocolChannelNotFound,
    ProtocolOutOfSync,
    ProtocolUserNotFound,
    ProtocolNotInThatChannel,
    ProtocolChannelAllocationFailed,
    ProtocolUnavailable,

    UserEdgeConnectionError,
    ApiOperatorGetChannelToken,
    VoiceEdgeConnectionError
}
