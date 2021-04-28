package com.esn.sonar.core;

import org.jboss.netty.channel.*;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

public abstract class InboundConnection extends SimpleChannelUpstreamHandler
{
    private final AtomicLong idCounter = new AtomicLong(0);
    private Channel channel;
    private final boolean expectRegistration;
    private final int registrationTimeoutSEC;
    private Timeout registrationTimeout;
    private Timeout keepaliveTimeout;
    private volatile boolean isRegistered = false;

    private final static CommandHandler commandHandler = new CommandHandler();

    static
    {
        setupCommandHandler();
    }

    private final int keepaliveIntervalSEC;
    protected final static Random random = new Random();
    private volatile long keepaliveProtoId = -1;

    protected abstract void onRegistrationTimeout();

    protected abstract boolean onRegisterCommand(Message msg) throws IOException;

    protected abstract boolean onMessage(Message msg) throws Exception;

    protected abstract void onResult(Message msg);

    protected abstract void onChannelDisconnected();

    public abstract String getId();

    private void handleUnregister(Message msg)
    {
    }


    private class RegistrationTimeoutTask implements TimerTask
    {
        public void run(Timeout timeout) throws Exception
        {
            if (timeout.isCancelled())
            {
                return;
            }

            //TODO: For some reason it appears this gets called though the timeout has been cancelled. Could be a bug in HashedWheelTimer or in our code.
            onRegistrationTimeout();
        }
    }

    private class KeepaliveSendTask implements TimerTask
    {

        public void run(Timeout timeout) throws Exception
        {
            if (timeout.isCancelled())
            {
                return;
            }

            if (keepaliveProtoId != -1)
            {
                closeChannel();
                return;
            }

            keepaliveProtoId = getNextId();
            sendCommand(keepaliveProtoId, Protocol.Commands.Keepalive, new ArrayList<Object>());
            InboundConnection.this.keepaliveTimeout = Maintenance.getInstance().scheduleTask(keepaliveIntervalSEC, this);
        }
    }

    public InboundConnection(boolean expectRegistration, int registrationTimeoutSEC, int keepaliveIntervalSEC)
    {
        this.expectRegistration = expectRegistration;
        this.registrationTimeoutSEC = registrationTimeoutSEC;
        this.keepaliveIntervalSEC = keepaliveIntervalSEC;
    }

    private void handleResult(Message msg)
    {
        if (msg.getId() == keepaliveProtoId)
        {
            keepaliveProtoId = -1;
        }

        onResult(msg);
    }

    private void handleKeepalive(Message msg)
    {
        sendSuccessReply(msg.getId());
    }

    private void handleRegister(Message msg) throws IOException
    {
        if (isRegistered)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.Unexpected);
            return;
        }

        if (registrationTimeout != null)
        {
            registrationTimeout.cancel();
            registrationTimeout = null;
        }

        if (!onRegisterCommand(msg))
        {
            closeChannel();
        }
    }

    public InetSocketAddress getRemoteAddress()
    {
        return ((InetSocketAddress) channel.getRemoteAddress());
    }

    public int getRemotePort()
    {
        return ((InetSocketAddress) channel.getRemoteAddress()).getPort();
    }

    public ChannelFuture sendCommand(String cmd, Collection<Object> argList)
    {
        return sendCommand(getNextId(), cmd, argList);
    }

    public ChannelFuture sendCommand(String cmd, Object... args)
    {
        return sendCommand(getNextId(), cmd, Arrays.asList(args));
    }

    private ChannelFuture sendCommand(long id, String cmd, Collection<Object> argList)
    {
        String[] msgArgs = new String[argList.size()];

        int index = 0;
        for (Object arg : argList)
        {
            msgArgs[index] = arg.toString();
            index++;
        }

        return channel.write(new Message(id, cmd, msgArgs));
    }

    protected ChannelFuture sendMessage(Message msg)
    {
        return channel.write(msg);

    }

    protected ChannelFuture sendSuccessReply(long id)
    {
        ArrayList<Object> args = new ArrayList<Object>(1);
        args.add("OK");

        return sendCommand(id, Protocol.Commands.Reply, args);
    }

    protected ChannelFuture sendSuccessReply(long id, Collection<Object> argList)
    {
        ArrayList<Object> args = new ArrayList<Object>(argList.size() + 1);
        args.add("OK");
        args.addAll(argList);

        return sendCommand(id, Protocol.Commands.Reply, args);
    }

    protected ChannelFuture sendSuccessReply(long id, Object... args)
    {
        return sendSuccessReply(id, Arrays.asList(args));
    }

    protected ChannelFuture sendErrorReply(long id, Object text)
    {
        ArrayList<Object> args = new ArrayList<Object>(2);
        args.add("ERROR");
        args.add(text);

        return sendCommand(id, Protocol.Commands.Reply, args);
    }

    private long getNextId()
    {
        return idCounter.incrementAndGet();
    }

    @Override
    public void channelClosed(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
    }

    @Override
    public void channelOpen(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelOpen(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.
        this.channel = ctx.getChannel();
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        if (expectRegistration && registrationTimeoutSEC > 0)
        {
            registrationTimeout = Maintenance.getInstance().scheduleTask(registrationTimeoutSEC, new RegistrationTimeoutTask());
        }

        if (keepaliveIntervalSEC > 0)
        {
            keepaliveTimeout = Maintenance.getInstance().scheduleTask(keepaliveIntervalSEC + random.nextInt(keepaliveIntervalSEC), new KeepaliveSendTask());
        }

        onChannelConnected();
    }

    protected abstract void onChannelConnected() throws IOException;

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        onChannelDisconnected();
        closeChannel();
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        if (e.getCause() instanceof IOException)
        {
            closeChannel();
            return;
        }

        if (e.getCause() instanceof NullPointerException)
        {
            e.getCause().printStackTrace();
            return;
        }

        super.exceptionCaught(ctx, e);
    }

    public boolean isRegistered()
    {
        return isRegistered;
    }

    public void setRegistered(boolean registered)
    {
        isRegistered = registered;
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        Message msg = (Message) e.getMessage();

        if (!commandHandler.call(this, msg))
        {
            if (expectRegistration && !isRegistered)
            {
                sendErrorReply(msg.getId(), Protocol.Errors.Unexpected);
                return;
            }

            if (!onMessage(msg))
            {
                sendErrorReply(msg.getId(), Protocol.Errors.UnknownCommand);
            }
        }
    }

    protected void closeChannel()
    {
        if (registrationTimeout != null)
        {
            registrationTimeout.cancel();
            registrationTimeout = null;
        }

        if (keepaliveTimeout != null)
        {
            keepaliveTimeout.cancel();
            keepaliveTimeout = null;
        }

        if (channel.isOpen())
            channel.close();
    }

    public void sendUnregisterAndClose(String reasonType, String reasonDesc)
    {
        setRegistered(false);

        sendCommand(Protocol.Commands.Unregister, reasonType, reasonDesc).addListener(new ChannelFutureListener()
        {
            public void operationComplete(ChannelFuture future) throws Exception
            {
                closeChannel();
            }
        });
    }

    public void write(Message msg)
    {
        if (!channel.isConnected())
            return;

        channel.write(msg);
    }

    /**
     * Initialization of command handler, performed only once when this class is first loaded.
     */
    private static void setupCommandHandler()
    {
        commandHandler.registerCallback(Protocol.Commands.Register, new CommandCallback()
        {
            public void call(Object instance, Message msg) throws IOException
            {
                ((InboundConnection) instance).handleRegister(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.Keepalive, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((InboundConnection) instance).handleKeepalive(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.Reply, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((InboundConnection) instance).handleResult(msg);
            }
        });

        commandHandler.registerCallback(Protocol.Commands.Unregister, new CommandCallback()
        {
            public void call(Object instance, Message msg)
            {
                ((InboundConnection) instance).handleUnregister(msg);
            }
        });
    }
}

