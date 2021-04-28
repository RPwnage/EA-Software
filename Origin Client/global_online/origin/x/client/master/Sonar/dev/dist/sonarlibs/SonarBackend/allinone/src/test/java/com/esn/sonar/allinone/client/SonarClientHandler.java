package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import org.apache.log4j.Logger;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.*;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertTrue;

public abstract class SonarClientHandler extends SimpleChannelUpstreamHandler
{
    private static final Logger log = Logger.getLogger(SonarClientHandler.class);
    private static final int TIMEOUT = 2000;
    protected int registerTimeout;

    protected Channel channel;
    protected final AllInOneConfig config;
    protected final String hostname;
    protected final int port;

    private final ClientBootstrap bootstrap;
    private long messageId = 0;
    private boolean connected = false;
    private boolean registered = false;
    //private ChannelFuture connectFuture = null;
    //private ChannelFuture disconnectFuture = null;

    private final CountDownLatch connectLatch = new CountDownLatch(1);
    private final CountDownLatch disconnectLatch = new CountDownLatch(1);
    private final CountDownLatch registerLatch = new CountDownLatch(1);
    private final CountDownLatch channelClosedLatch = new CountDownLatch(1);
    private final CountDownLatch registerFailedLatch = new CountDownLatch(1);
    private final CountDownLatch errorResultLatch = new CountDownLatch(1);
    protected final CountDownLatch keepaliveLatch = new CountDownLatch(1);

    public SonarClientHandler(ClientBootstrap bootstrap, String host, int port, AllInOneConfig config)
    {
        this.bootstrap = bootstrap;
        this.hostname = host;
        this.port = port;
        this.config = config;
        registerTimeout = config.voiceEdgeConfig.getChallengeTimeout() * 1000;
        registerTimeout += registerTimeout / 4;
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        super.exceptionCaught(ctx, e);    //To change body of overridden methods use File | Settings | File Templates.
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        connected = true;
        channel = e.getChannel();
        connectLatch.countDown();
    }

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        connected = false;
        registered = false;
        channelClosedLatch.countDown();
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        Message message = (Message) e.getMessage();
        if (message.getName().equals(Protocol.Commands.Reply))
        {
            if (message.getArgument(0).equals("OK"))
            {
                if (!registered)
                {
                    registered = true;
                    registerLatch.countDown();
                    log.info("Registered");
                    onRegister(message);
                    return;
                }
            } else
            {
                if (!registered)
                {
                    registerFailedLatch.countDown();
                } else
                {
                    errorResultLatch.countDown();
                }
            }
        }

        onMessage(message);
    }

    public SonarClientHandler connect()
    {
        bootstrap.connect(new InetSocketAddress(hostname, port)).awaitUninterruptibly();
        return this;
    }

    public void waitForChannelClosed(int timeout) throws InterruptedException
    {
        if (!channelClosedLatch.await(timeout, TimeUnit.SECONDS))
        {
            throw new InterruptedException("Waiting timed out");
        }
    }

    public void waitForKeepalive(int timeout) throws InterruptedException
    {
        if (!keepaliveLatch.await(timeout, TimeUnit.SECONDS))
        {
            throw new InterruptedException("Waiting timed out");
        }
    }


    public SonarClientHandler disconnect() throws InterruptedException
    {
        waitForConnect();
        channel.write(new Message(getMessageId(), Protocol.Commands.Unregister, "", "")).addListener(new ChannelFutureListener()
        {
            public void operationComplete(ChannelFuture future) throws Exception
            {
                channel.close();
                disconnectLatch.countDown();
            }
        });

        return this;
    }

    public boolean isConnected()
    {
        return connected;
    }

    public ChannelFuture send(Message message)
    {
        return channel.write(message);
    }

    public long getMessageId()
    {
        messageId++;
        return messageId % 0xffffffff;
    }

    public SonarClientHandler waitForRegister() throws InterruptedException
    {
        assertTrue("Register latch", registerLatch.await(registerTimeout, TimeUnit.MILLISECONDS));
        return this;
    }

    public SonarClientHandler waitForErrorResult() throws InterruptedException
    {
        assertTrue("Error Result latch", errorResultLatch.await(TIMEOUT, TimeUnit.MILLISECONDS));
        return this;
    }

    public SonarClientHandler waitForConnect() throws InterruptedException
    {
        assertTrue("Connect latch", connectLatch.await(TIMEOUT, TimeUnit.MILLISECONDS));
        return this;
    }

    public SonarClientHandler waitForDisconnect() throws InterruptedException
    {
        assertTrue("Disconnect latch", disconnectLatch.await(TIMEOUT, TimeUnit.MILLISECONDS));
        return this;
    }

    public SonarClientHandler waitForRegisterError() throws InterruptedException
    {
        assertTrue("Register Error latch", registerFailedLatch.await(registerTimeout, TimeUnit.MILLISECONDS));
        return this;
    }


    public boolean isRegistered()
    {
        return registered;
    }

    public void close()
    {
        if (channel != null)
        {
            channel.close().awaitUninterruptibly();
        }
    }

    abstract public void onMessage(Message message) throws Token.InvalidToken;

    abstract public void onRegister(Message message) throws IOException;

    abstract public SonarClientHandler sendRegister();

}
