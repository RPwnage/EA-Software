package com.esn.sonartest;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.MasterTestConfig;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.io.IOException;
import java.security.spec.InvalidKeySpecException;


/*
Discards everything it receives
 */

public class DiscardingEventClient extends EventClient
{
    private Runnable registrationCallback;

    public DiscardingEventClient(MasterTestConfig config, ClientBootstrap bootstrap, String roundRobinGroup, Runnable registrationCallback)
    {
        super(config, bootstrap, roundRobinGroup);

        this.registrationCallback = registrationCallback;
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        return true;
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        super.onRegisterSuccess(msg);
        registrationCallback.run();
    }

}
