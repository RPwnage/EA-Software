package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.master.api.operator.OperatorService;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.MessageEvent;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import static com.esn.sonar.core.util.Utils.tokenize;

public class FakeOperatorClient extends SonarClientHandler implements OperatorService
{
    private final AtomicLong protoId = new AtomicLong();

    public FakeOperatorClient(ClientBootstrap bootstrap, AllInOneConfig config)
    {
        super(bootstrap, config.userEdgeConfig.getLocalAddress(), config.masterConfig.getOperatorServiceBindAddress().getPort(), config);
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        Message message = (Message) e.getMessage();
        onMessage(message);
    }

    @Override
    public SonarClientHandler sendRegister()
    {
        return this;
    }

    private class Request
    {
        private final Message command;
        private Message result;
        private final CountDownLatch latch;

        public Request(Message command)
        {
            this.command = command;
            this.latch = new CountDownLatch(1);
        }

        public Message getCommand()
        {
            return command;
        }

        public Message getResult()
        {
            return result;
        }

        public void setResult(Message result)
        {
            this.result = result;
            this.latch.countDown();
        }

        public Message waitForResult()
        {
            try
            {
                if (!this.latch.await(5, TimeUnit.SECONDS))
                {
                    return null;
                }

                return result;
            } catch (InterruptedException e)
            {
                return waitForResult();
            }
        }
    }

    final ConcurrentHashMap<Long, Request> idToRequestMap = new ConcurrentHashMap<Long, Request>();

    @Override
    public void onMessage(Message message) throws Token.InvalidToken
    {
        Request request = idToRequestMap.get(message.getId());

        if (request != null && message.getName().equals(Protocol.Commands.Reply))
        {
            request.setResult(message);
        }
    }

    @Override
    public void onRegister(Message message)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    private Request sendCommandMessage(String... args)
    {
        String[] strings = Arrays.copyOfRange(args, 1, args.length);

        Message msg = new Message(protoId.incrementAndGet(), args[0], strings);

        Request request = new Request(msg);

        idToRequestMap.put(msg.getId(), request);
        channel.write(msg);

        return request;
    }

    public void joinUserToChannel(String operatorId, String location, String userId, String channelId, String channelDesc) throws ChannelAllocationFailed, InvalidArgumentException
    {
        Message result = sendCommandMessage(Protocol.Commands.JoinUserToChannel, operatorId, userId, channelId, channelDesc, location).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.ChannelAllocationFailed))
            {
                throw new ChannelAllocationFailed("Channel allocation failed");
            } else if (result.getArgument(1).equals(Protocol.Errors.InvalidArgument))
            {
                throw new InvalidArgumentException("Channel allocation failed");
            }
        }
    }


    public void disconnectUser(String operatorId, String userId, String reasonType, String reasonDesc)
    {
        sendCommandMessage(Protocol.Commands.DisconnectUser, operatorId, userId, reasonType, reasonDesc).waitForResult();
    }

    public void partUserFromChannel(String operatorId, String userId, String channelId, String reasonType, String reasonDesc) throws UserNotFoundException, NotInThatChannelException
    {
        Message result = sendCommandMessage(Protocol.Commands.PartUserFromChannel, operatorId, userId, channelId, reasonType, reasonDesc).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.UserNotFound))
            {
                throw new UserNotFoundException(userId);
            } else if (result.getArgument(1).equals(Protocol.Errors.NotInThatChannel))
            {
                throw new NotInThatChannelException("");
            }
        }

    }

    public void destroyChannel(String operatorId, String channelId, String reasonType, String reasonDesc) throws ChannelNotFoundException
    {
        Message result = sendCommandMessage(Protocol.Commands.DestroyChannel, operatorId, channelId, reasonType, reasonDesc).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.ChannelNotFound))
            {
                throw new ChannelNotFoundException(channelId);
            }
        }
    }

    public Collection<UserOnlineStatus> getUsersOnlineStatus(String operatorId, Collection<String> userIds)
    {
        ArrayList<String> args = new ArrayList<String>(userIds.size() + 2);
        args.add(Protocol.Commands.GetUsersOnlineStatus);
        args.add(operatorId);
        args.addAll(userIds);

        String[] strings = args.toArray(new String[args.size()]);

        Request request = sendCommandMessage(strings);

        Message result = request.waitForResult();

        Collection<UserOnlineStatus> retList = new ArrayList<UserOnlineStatus>(result.getArgumentCount() - 1);


        for (int index = 1; index < result.getArgumentCount(); index++)
        {
            String arg = result.getArgument(index);

            String[] uosParams = tokenize(arg, '|');

            retList.add(new UserOnlineStatus(uosParams[0], uosParams[1].equals("1") ? true : false, uosParams[2]));
        }

        return retList;
    }

    public Collection<String> getUsersInChannel(String operatorId, String channelId) throws ChannelNotFoundException
    {
        Message result = sendCommandMessage(Protocol.Commands.GetUsersInChannel, operatorId, channelId).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.ChannelNotFound))
            {
                throw new ChannelNotFoundException(channelId);
            }
        }

        ArrayList<String> retList = new ArrayList<String>(result.getArgumentCount() - 1);

        for (int index = 1; index < result.getArgumentCount(); index++)
        {
            retList.add(result.getArgument(index));
        }

        return retList;
    }

    public String getUserControlToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location) throws InvalidArgumentException, UnavailableException
    {
        Message result = sendCommandMessage(Protocol.Commands.GetUserControlToken, operatorId, userId, userDesc, channelId, channelDesc, location).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.InvalidArgument))
            {
                throw new InvalidArgumentException("Invalid argument");
            } else if (result.getArgument(1).equals(Protocol.Errors.Unavailable))
            {
                throw new UnavailableException("Unavailable");
            }
        }

        return result.getArgument(1);
    }

    public String getChannelToken(String operatorId, String userId, String userDesc, String channelId, String channelDesc, String location, String clientAddress) throws InvalidArgumentException, ChannelAllocationFailed
    {
        Message result = sendCommandMessage(Protocol.Commands.GetChannelToken, operatorId, userId, userDesc, channelId, channelDesc, location, clientAddress).waitForResult();

        if (result.getArgument(0).equals("ERROR"))
        {
            if (result.getArgument(1).equals(Protocol.Errors.InvalidArgument))
            {
                throw new InvalidArgumentException("Invalid argument");
            } else if (result.getArgument(1).equals(Protocol.Errors.ChannelAllocationFailed))
            {
                throw new ChannelAllocationFailed("Unavailable");
            }
        }

        return result.getArgument(1);
    }
}
