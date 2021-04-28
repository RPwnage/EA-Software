package com.esn.sonar.master.api.event;

import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.MasterServer;

public class EventConnection extends InboundConnection implements EventConsumer
{
    private String roundRobinGroup;
    private String operatorId;
    private EventPublisher eventPublisher;

    public EventConnection()
    {
        super(true, 10, -1);
    }

    @Override
    public String getId()
    {
        return null;
    }

    @Override
    protected void onRegistrationTimeout()
    {
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        if (msg.getArgumentCount() < 2)
        {
            sendErrorReply(msg.getId(), Protocol.Errors.NotEnoughArguments);
            return false;
        }

        this.roundRobinGroup = msg.getArgument(1);
        this.eventPublisher = MasterServer.getInstance().getOperatorManager().getOperator(msg.getArgument(0)).getEventPublisher();
        this.eventPublisher.addConsumer(this);

        sendSuccessReply(msg.getId());

        setRegistered(true);

        return true;
    }

    @Override
    protected boolean onMessage(Message msg)
    {
        return false;
    }

    @Override
    protected void onResult(Message msg)
    {
    }

    @Override
    protected void onChannelDisconnected()
    {
        if (eventPublisher != null)
        {
            eventPublisher.removeConsumer(this);
        }
    }

    @Override
    protected void onChannelConnected()
    {
    }

    public String getRoundRobinGroup()
    {
        return roundRobinGroup;
    }

    public void handleEvent(Message event)
    {
        sendMessage(event);
    }
}
