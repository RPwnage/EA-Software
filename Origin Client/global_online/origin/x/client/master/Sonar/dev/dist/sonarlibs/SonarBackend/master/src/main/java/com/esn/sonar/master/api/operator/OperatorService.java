package com.esn.sonar.master.api.operator;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.MasterMetrics;
import com.esn.sonar.master.MasterStats;

import java.util.Collection;
import java.util.concurrent.atomic.AtomicLong;

public interface OperatorService
{
    public void joinUserToChannel(String operatorId, String location, String userId, String channelId, String channelDesc) throws ChannelAllocationFailed, InvalidArgumentException;

    public void disconnectUser(String operatorId, String userId, String reasonType, String reasonDesc) throws InvalidArgumentException;

    public void partUserFromChannel(String operatorId, String userId, String channelId, String reasonType, String reasonDesc) throws UserNotFoundException, NotInThatChannelException, ChannelNotFoundException, OutOfSyncException, InvalidArgumentException;

    public void destroyChannel(String operatorId, String channelId, String reasonType, String reasonDesc) throws ChannelNotFoundException, InvalidArgumentException;

    public Collection<UserOnlineStatus> getUsersOnlineStatus(String operatorId, Collection<String> userIds) throws InvalidArgumentException;

    public Collection<String> getUsersInChannel(String operatorId, String channelId) throws ChannelNotFoundException, InvalidArgumentException;

    public String getUserControlToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location) throws InvalidArgumentException, UnavailableException;

    public String getChannelToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location, String clientAddress) throws InvalidArgumentException, ChannelAllocationFailed;

    /*
    enum EventArgumentTypes
    {
        userId,
        userDescription,
        channelId,
        channelDescription,
        channelUserCount,
        operatorId,
    }
    */

    static public class EventMessage extends Message
    {
        static AtomicLong idCounter = new AtomicLong(0);

        public EventMessage(String name, String... args)
        {
            super(idCounter.incrementAndGet(), name, args);
        }
    }

    static public class UserConnectedEvent extends EventMessage
    {
        public UserConnectedEvent(String operatorId, String userId, String userDesc)
        {
            super(Protocol.Events.UserConnected, operatorId, userId, userDesc);
            MasterStats.getStatsManager().metric(MasterMetrics.ApiEventUserConnectedEvent).incrementAndGet();

        }
    }

    static public class UserDisconnectedEvent extends EventMessage
    {
        public UserDisconnectedEvent(String operatorId, String userId, String userDesc)
        {
            super(Protocol.Events.UserDisconnected, operatorId, userId, userDesc);
            MasterStats.getStatsManager().metric(MasterMetrics.ApiEventUserDisconnectedEvent).incrementAndGet();
        }
    }

    static public class UserConnectedToChannelEvent extends EventMessage
    {
        public UserConnectedToChannelEvent(String operatorId, String userId, String userDesc, String channelId, String channelDesc, int serverUserCount)
        {
            super(Protocol.Events.UserConnectedToChannel, operatorId, userId, userDesc, channelId, channelDesc, Integer.toString(serverUserCount));
            MasterStats.getStatsManager().metric(MasterMetrics.ApiEventUserConnectedToChannelEvent).incrementAndGet();
        }
    }

    static public class UserDisconnectedFromChannelEvent extends EventMessage
    {
        public UserDisconnectedFromChannelEvent(String operatorId, String userId, String userDesc, String channelId, String channelDesc, int channelUserCount)
        {
            super(Protocol.Events.UserDisconnectedFromChannel, operatorId, userId, userDesc, channelId, channelDesc, Integer.toString(channelUserCount));
            MasterStats.getStatsManager().metric(MasterMetrics.ApiEventUserDisconnectedFromChannelEvent).incrementAndGet();

        }
    }


    static public class ChannelDestroyedEvent extends EventMessage
    {
        public ChannelDestroyedEvent(String operatorId, String channelId)
        {
            super(Protocol.Events.ChannelDestroyed, operatorId, channelId);
            MasterStats.getStatsManager().metric(MasterMetrics.ApiEventChannelDestroyedEvent).incrementAndGet();
        }
    }

    static public class OperatorException extends Exception
    {
        public OperatorException(String message)
        {
            super(message);
        }
    }

    static public class OutOfSyncException extends OperatorException
    {
        public OutOfSyncException(String message)
        {
            super(message);
        }
    }

    static public class UserNotFoundException extends OperatorException
    {
        public UserNotFoundException(String userId)
        {
            super("User " + userId + " not found");
        }
    }

    static public class ChannelAllocationFailed extends OperatorException
    {
        public ChannelAllocationFailed(String msg)
        {
            super(msg);
        }
    }

    static public class InvalidArgumentException extends OperatorException
    {
        public InvalidArgumentException(String msg)
        {
            super(msg);
        }
    }

    static public class NotInThatChannelException extends OperatorException
    {
        public NotInThatChannelException(String message)
        {
            super(message);
        }
    }

    static public class ChannelNotFoundException extends OperatorException
    {
        public ChannelNotFoundException(String channelId)
        {
            super("Channel " + channelId + " not found");
        }
    }

    static public class UnavailableException extends OperatorException
    {
        public UnavailableException(String message)
        {
            super(message);
        }
    }

    static public class UserOnlineStatus
    {
        private final String userId;
        private final boolean isOnline;
        private final String channelId;

        @Override
        public String toString()
        {
            StringBuilder sb = new StringBuilder();

            sb.append(userId);
            sb.append('|');
            sb.append(isOnline ? '1' : '0');
            sb.append('|');
            sb.append(channelId);

            return sb.toString();
        }

        public UserOnlineStatus(String userId, boolean isOnline, String channelId)
        {
            this.userId = userId;
            this.isOnline = isOnline;
            this.channelId = channelId;
        }

        @Override
        public boolean equals(Object o)
        {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            UserOnlineStatus that = (UserOnlineStatus) o;

            if (isOnline != that.isOnline) return false;
            if (channelId != null ? !channelId.equals(that.channelId) : that.channelId != null) return false;
            if (userId != null ? !userId.equals(that.userId) : that.userId != null) return false;

            return true;
        }

        @Override
        public int hashCode()
        {
            return userId != null ? userId.hashCode() : 0;
        }
    }

}
