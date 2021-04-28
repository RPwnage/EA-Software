package com.esn.sonartest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.MasterTest.MasterTestConfig;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.verifier.EventVerifier;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.io.IOException;
import java.security.spec.InvalidKeySpecException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class EventClient extends OutboundConnection
{
    private CountDownLatch registrationLatch = new CountDownLatch(1);
    private String roundRobinGroup;

    public EventClient(MasterTestConfig config, ClientBootstrap bootstrap, String roundRobinGroup)
    {
        super(-1, 60, true, bootstrap, config.getEventAddress());
        this.roundRobinGroup = roundRobinGroup;
    }

    @Override
    protected void onChannelConnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        EventVerifier.getInstance().onEvent(msg);
        return true;
    }

    @Override
    protected boolean onResult(Message msg)
    {
        return true;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onSendRegister()
    {
        write(new Message(getMessageId(), Protocol.Commands.Register, MasterTestDirector.operatorId, roundRobinGroup));
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelDisconnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        throw new RuntimeException("Registration failed");
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        registrationLatch.countDown();
    }

    public void waitForRegistration() throws InterruptedException
    {
        if (!registrationLatch.await(10, TimeUnit.SECONDS))
        {
            throw new RuntimeException("Registration of event client timed out");
        }

    }
}
