package com.esn.sonar.core.testutil;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.net.InetSocketAddress;
import java.security.KeyPair;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class UserConnection extends OutboundConnection
{
    protected Token token;
    private CountDownLatch registrationLatch = new CountDownLatch(1);

    private KeyPair keyPair;

    public UserConnection(ClientBootstrap bootstrap, InetSocketAddress masterAddress, String operatorId, String userId, String userDesc, KeyPair keyPair)
    {
        super(0, 60, true, bootstrap, masterAddress);
        token = new Token(operatorId, userId, userDesc, "", "", "", Utils.getTimestamp() / 1000, masterAddress.getHostName(), masterAddress.getPort(), "", 0);

        this.keyPair = keyPair;
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected boolean onResult(Message msg)
    {
        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelConnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onChannelDisconnected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected void onSendRegister()
    {
        String version = String.format("%d.%d", Protocol.Versions.voiceClientMajor, 1);

        write(new Message(getMessageId(),
                Protocol.Commands.Register,
                token.sign(keyPair.getPrivate()),
                version));
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        throw new RuntimeException("Registration failed: " + msg.getArgument(1));
    }

    @Override
    protected void onRegisterSuccess(Message msg)
    {
        registrationLatch.countDown();
    }

    public void waitForRegistration() throws InterruptedException
    {
        if (!registrationLatch.await(1000, TimeUnit.SECONDS))
        {
            throw new RuntimeException("Wait for registration timed out");
        }
    }

}
