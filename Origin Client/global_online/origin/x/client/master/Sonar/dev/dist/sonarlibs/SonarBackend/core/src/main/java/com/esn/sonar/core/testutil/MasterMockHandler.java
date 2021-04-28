package com.esn.sonar.core.testutil;

import com.esn.sonar.core.InboundConnection;
import com.esn.sonar.core.Message;

public class MasterMockHandler extends InboundConnection
{
    public MasterMockHandler()
    {
        super(true, 1, 1);
    }

    @Override
    public String getId()
    {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onRegistrationTimeout()
    {
    }

    @Override
    protected boolean onRegisterCommand(Message msg)
    {
        sendSuccessReply(msg.getId());
        setRegistered(true);
        return true;
    }

    @Override
    protected boolean onMessage(Message msg)
    {
        return true;
    }

    @Override
    protected void onResult(Message msg)
    {
    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    @Override
    protected void onChannelConnected()
    {
    }
}
