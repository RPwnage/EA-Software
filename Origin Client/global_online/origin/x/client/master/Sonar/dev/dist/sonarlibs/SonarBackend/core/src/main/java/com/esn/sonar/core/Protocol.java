package com.esn.sonar.core;

/**
 * Created by IntelliJ IDEA.
 * VoiceUser: jonas
 * Date: 2010-okt-07
 * Time: 23:18:55
 * To change this template use File | Settings | File Templates.
 */
public class Protocol
{
    public static class Versions
    {
        public final static int voiceClientMajor = 10;
        public final static int voiceServerMajor = 10;
    }

    public static class Errors
    {
        /* Not enough arguments for command */
        public final static String NotEnoughArguments = "NOT_ENOUGH_ARGUMENTS";
        /* Command is not yet implemented */
        public final static String NotImplemented = "NOT_IMPLEMENTED";
        /* Command is unknown */
        public final static String UnknownCommand = "UNKNOWN_COMMAND";
        /* Command was unexpected at this point */
        public final static String Unexpected = "UNEXPECTED";
        /* One ore more arguments were invalid */
        public final static String InvalidArgument = "INVALID_ARGUMENT";
        /* Client was detected to be behind NAT */
        public final static String NATDetected = "NAT_DETECTED";
        /* Provided token was invalid */
        public final static String InvalidToken = "INVALID_TOKEN";
        /* Channel could not be found */
        public final static String ChannelNotFound = "CHANNEL_NOT_FOUND";
        /* User could not be found */
        public final static String UserNotFound = "USER_NOT_FOUND";
        /* User was not in specified channel */
        public final static String NotInThatChannel = "NOT_IN_THAT_CHANNEL";
        /* Allocation of channel has failed */
        public final static String ChannelAllocationFailed = "CHANNEL_ALLOCATION_FAILED";
        public final static String OutOfSync = "OUT_OF_SYNC";

        // The challenge request expired
        public final static String ChallengeExpired = "CHALLENGE_EXPIRED";

        // Something timedout, usually registration
        public final static String Timeout = "TIMEOUT";
        public final static String WrongVersion = "WRONG_VERSION";
        public final static String RegistrationTimedout = "REGISTRATION_TIMEDOUT";
        public final static String Unavailable = "UNAVAILABLE";
    }

    /*
    These commands are interpreted by voice servers and may not be changed without also changing in the voice server code.
     */
    public static class VoiceServerCommands
    {
        public final static String ClientUnregister = "CLIENT_UNREGISTER";
        public final static String ChannelDestroy = "CHANNEL_DESTROY";
    }

    public static class Commands
    {
        /* Generic commands */
        public final static String Keepalive = "KEEPALIVE";
        public final static String Register = "REGISTER";
        public final static String Unregister = "UNREGISTER";

        /* Backend to voice server */

        /* Backend to voice client */
        public final static String UpdateToken = "UPDATE_TOKEN";

        /* Operator connection commands */
        public final static String JoinUserToChannel = "JOIN_USER";
        public final static String PartUserFromChannel = "PART_USER";
        public final static String DisconnectUser = "DISCONNECT_USER";
        public final static String DestroyChannel = "DESTROY_CHANNEL";
        public final static String GetUsersOnlineStatus = "GET_ONLINE_STATUS";
        public final static String GetUsersInChannel = "GET_CHANNEL_USERS";
        public final static String GetChannelToken = "GET_CHANNEL_TOKEN";
        public final static String GetUserControlToken = "GET_CONTROL_TOKEN";
        public final static String Reply = "REPLY";

        // User edge
        public final static String EdgeUserRegistered = "EDGE_USER_REGISTERED";
        public final static String EdgeUserUnregistered = "EDGE_USER_UNREGISTERED";
        public final static String EdgeUnregisterClient = "EDGE_UNREGISTER_CLIENT";
        public final static String EdgeUpdateToken = "EDGE_UPDATE_TOKEN";

        // Voice edge
        public final static String EdgeUnregisterServer = "EDGE_UNREGISTER_SERVER";
        public final static String EdgeServerRegistered = "EDGE_SERVER_REGISTERED";
        public final static String EdgeServerUnregistered = "EDGE_SERVER_UNREGISTERED";
        public final static String EdgeUpdateKeys = "EDGE_UPDATE_KEYS";
    }

    public static class Reasons
    {
        /* Operator to client */
        public final static String AdminKick = "ADMIN_KICK";
        public final static String Unknown = "UNKNOWN";
        public final static String ConnectionLost = "CONNECTION_LOST";

        /* Internal, backend and voice server */
        public final static String LoggedInElsewhere = "LOGGEDIN_ELSEWHERE";
        public final static String Roaming = "ROAMING";
        public final static String MasterSync = "MASTERSYNC";
        public final static String TryAgain = "TRY_AGAIN";
        public final static String Join = "JOIN";
        public final static String Update = "UPDATE";
        public final static String Roam = "ROAM";
    }

    /*
    These events are generated by voice servers and may not be changed without also changing in the voice server code.
     */
    public static class VoiceServerEvents
    {
        public final static String ClientRegisteredToChannel = "EVENT_CLIENT_REGISTERED_TOCHANNEL";
        public final static String ClientUnregistered = "EVENT_CLIENT_UNREGISTERED";
        public final static String ChannelDestroyed = "EVENT_CHANNEL_DESTROYED";
        public final static String StateClient = "EVENT_STATE_CLIENT";
    }

    public static class Events
    {
        public final static String UserConnected = "EVENT_USER_CONNECTED";
        public final static String UserDisconnected = "EVENT_USER_DISCONNECTED";
        public final static String UserConnectedToChannel = "EVENT_USER_CONNECTED_TO_CHANNEL";
        public final static String UserDisconnectedFromChannel = "EVENT_USER_DISCONNECTED_FROM_CHANNEL";
        public final static String ChannelDestroyed = "EVENT_CHANNEL_DESTROYED";
    }

    public static class Limits
    {
        public final static int locationLength = 64;
        public final static int channelIdLength = 255;
        public final static int channelDescLength = 255;
        public final static int userIdLength = 255;
        public final static int userDescLength = 255;
        public final static int reasonTypeLength = 255;
        public final static int reasonDescLength = 255;
        public final static int operatorIdLength = 255;
        public final static int addressLength = 255;
    }
}
