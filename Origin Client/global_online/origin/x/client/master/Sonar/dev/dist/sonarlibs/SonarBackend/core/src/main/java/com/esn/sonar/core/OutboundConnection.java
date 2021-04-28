package com.esn.sonar.core;

import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.*;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.io.IOException;
import java.net.ConnectException;
import java.net.InetSocketAddress;
import java.security.spec.InvalidKeySpecException;
import java.util.Random;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public abstract class OutboundConnection extends SimpleChannelUpstreamHandler
{
    static protected final Random random = new Random();

    protected final AtomicLong messageId = new AtomicLong(0L);
    private final InetSocketAddress masterAddress;
    private volatile Channel channel;
    private final boolean shouldRegister;
    private final ClientBootstrap bootstrap;

    private final AtomicBoolean isRegistered = new AtomicBoolean(false);
    private volatile Timeout reconnectTimeout;
    private volatile int retryTimeout;
    private int keepaliveIntervalSEC;
    private Timeout keepaliveTimeout;

    public OutboundConnection(int retryTimeoutSec, int keepaliveIntervalSEC, boolean shouldRegister, ClientBootstrap bootstrap, InetSocketAddress masterAddress)
    {
        this.bootstrap = bootstrap;
        this.masterAddress = masterAddress;
        this.shouldRegister = shouldRegister;
        this.retryTimeout = retryTimeoutSec;
        this.keepaliveIntervalSEC = keepaliveIntervalSEC;
    }

    private class ReconnectTask implements TimerTask
    {
        public void run(Timeout timeout) throws Exception
        {
            if (timeout.isCancelled())
                return;

            OutboundConnection.this.connect();
        }
    }

    protected abstract boolean onCommand(Message msg) throws Exception;

    protected abstract boolean onResult(Message msg);

    protected abstract void onChannelConnected();

    protected abstract void onSendRegister();

    protected abstract void onChannelDisconnected();

    protected abstract void onRegisterError(Message msg);

    protected abstract void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException;

    public ChannelFuture connect()
    {
        if (isConnected())
        {
            throw new IllegalStateException("Channel is already connected");
        }

        bootstrap.setPipelineFactory(new ChannelPipelineFactory()
        {
            public ChannelPipeline getPipeline() throws Exception
            {
                return getTabProtocolPipeline(OutboundConnection.this);
            }
        });

        return bootstrap.connect(masterAddress);
    }

    @Override
    public void channelClosed(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelClosed(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.

        if (retryTimeout > 0)
        {
            reconnectTimeout = Maintenance.getInstance().scheduleTask(retryTimeout, new ReconnectTask());
        }
    }

    public void setRetryTimeout(int sec)
    {
        retryTimeout = sec;
    }


    @Override
    public void channelOpen(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelOpen(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.
        channel = ctx.getChannel();
    }


    private class KeepaliveTimeoutTask implements TimerTask
    {
        public void run(Timeout timeout) throws Exception
        {
            if (timeout.isCancelled())
            {
                return;
            }

            //TODO: For some reason it appears this gets called though the timeout has been cancelled. Could be a bug in HashedWheelTimer or in our code.
            OutboundConnection.this.write(new Message(getMessageId(), Protocol.Commands.Keepalive));
            OutboundConnection.this.keepaliveTimeout = Maintenance.getInstance().scheduleTask(keepaliveIntervalSEC, this);

        }
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelConnected(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.

        isRegistered.set(false);

        onChannelConnected();

        if (shouldRegister)
        {
            onSendRegister();
        }

        if (keepaliveIntervalSEC > 0)
        {
            keepaliveTimeout = Maintenance.getInstance().scheduleTask(keepaliveIntervalSEC + random.nextInt(keepaliveIntervalSEC), new KeepaliveTimeoutTask());
        }

    }

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        isRegistered.set(false);

        super.channelDisconnected(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.

        onChannelDisconnected();

        close();
    }


    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        isRegistered.set(false);

        if (ctx.getChannel().isOpen())
        {
            close();
        }

        if (e.getCause() instanceof ConnectException ||
                e.getCause() instanceof IOException ||
                e.getCause() instanceof RejectedExecutionException)
        {
            return;
        }

        super.exceptionCaught(ctx, e);
    }

    @Override
    synchronized public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        Message msg = (Message) e.getMessage();

        if (msg.getName().equals(Protocol.Commands.Reply))
        {
            if (shouldRegister && !isRegistered.get())
            {
                if (msg.getArgument(0).equals("OK"))
                {
                    onRegisterSuccess(msg);
                    isRegistered.set(true);
                } else
                {
                    onRegisterError(msg);
                    isRegistered.set(false);
                    close();
                }
            } else
            {
                onResult(msg);
            }
        } else
        {
            onCommand(msg);
        }

    }

    public long getMessageId()
    {
        long id = messageId.getAndIncrement();
        return id & 0x7fffffff;
    }


    public ChannelFuture close()
    {
        if (reconnectTimeout != null)
            reconnectTimeout.cancel();

        if (keepaliveTimeout != null)
        {
            keepaliveTimeout.cancel();
            keepaliveTimeout = null;
        }

        retryTimeout = 0;

        ChannelFuture channelFuture = channel.close();

        return channelFuture;
    }

    public void write(Message message)
    {
        if (!channel.isConnected())
            return;

        channel.write(message);
    }


    public boolean isConnected()
    {
        if (channel == null)
        {
            return false;
        }

        return channel.isConnected();
    }

    public boolean isRegistered()
    {
        return isRegistered.get();
    }

    public boolean shouldRegister()
    {
        return shouldRegister;
    }

}
