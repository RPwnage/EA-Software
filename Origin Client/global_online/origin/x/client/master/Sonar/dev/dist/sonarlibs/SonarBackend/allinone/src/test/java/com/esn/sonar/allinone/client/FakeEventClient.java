package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class FakeEventClient extends SonarClientHandler
{
    private final ConcurrentLinkedQueue<Message> eventList = new ConcurrentLinkedQueue<Message>();
    private String operatorId;

    public FakeEventClient(ClientBootstrap bootstrap, AllInOneConfig config, String operatorId)
    {
        super(bootstrap, config.userEdgeConfig.getLocalAddress(), config.masterConfig.getEventServiceBindAddress().getPort(), config);
        this.operatorId = operatorId;
    }

    @Override
    public void onMessage(Message message) throws Token.InvalidToken
    {
        //To change body of implemented methods use File | Settings | File Templates.

        if (message.getName().startsWith("EVENT_"))
        {
            eventList.add(message);
        }
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelConnected(ctx, e);

        sendRegister();
    }

    @Override
    public void onRegister(Message message)
    {
    }

    @Override
    public SonarClientHandler sendRegister()
    {
        send(new Message(getMessageId(), Protocol.Commands.Register, operatorId, ""));
        return this;
    }

    public Message waitForEventMessage()
    {
        return waitForEventMessage(5);
    }

    public Message waitForEventMessage(int seconds)
    {
        long expire = System.currentTimeMillis() + (seconds * 1000);

        while (System.currentTimeMillis() < expire)
        {
            if (eventList.isEmpty())
            {
                try
                {
                    Thread.sleep(20);
                } catch (InterruptedException e)
                {
                    return null;
                }
                continue;
            }

            return eventList.poll();

        }

        return null;
    }
}
