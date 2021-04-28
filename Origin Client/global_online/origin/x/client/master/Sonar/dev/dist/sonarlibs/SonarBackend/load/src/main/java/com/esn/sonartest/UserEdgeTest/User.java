package com.esn.sonartest.UserEdgeTest;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.OutboundConnection;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import org.jboss.netty.bootstrap.ClientBootstrap;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.PrivateKey;
import java.security.spec.InvalidKeySpecException;
import java.util.concurrent.CountDownLatch;

public class User extends OutboundConnection
{
    private final CountDownLatch registrationLatch = new CountDownLatch(1);
    private Token token;
    private PrivateKey privateKey;


    public static interface MessageCallback
    {
        public void onMessage(Message msg);
    }

    private MessageCallback cbRegistrationSuccess;
    private MessageCallback cbRegistrationError;


    public User(Token token, PrivateKey privateKey, ClientBootstrap bootstrap, InetSocketAddress masterAddress)
    {
        super(-1, 60, true, bootstrap, masterAddress);
        this.token = token;
        this.privateKey = privateKey;
    }

    @Override
    protected void onChannelConnected()
    {
    }

    @Override
    protected boolean onCommand(Message msg) throws Exception
    {
        if (msg.getName().equals(Protocol.Commands.Keepalive))
        {
            return true;
        } else if (msg.getName().equals(Protocol.Commands.UpdateToken))
        {
            Token t = new Token(msg.getArgument(0), null, -1);

            String type = msg.getArgument(1);

            if (type.equals("JOIN"))
            {
                if (!t.getChannelId().equals(getUserId()))
                {
                    throw new RuntimeException();
                }
            }

            this.token = t;
            return true;
        }


        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    protected boolean onResult(Message msg)
    {
        return true;
    }

    @Override
    protected void onChannelDisconnected()
    {
    }

    @Override
    protected void onRegisterError(Message msg)
    {
        this.cbRegistrationError.onMessage(msg);
    }

    @Override
    protected void onSendRegister()
    {
        token.setCreationTime(Utils.getTimestamp() / 1000);

        write(new Message(getMessageId(), Protocol.Commands.Register, token.sign(privateKey), "3.0"));
    }

    @Override
    protected void onRegisterSuccess(Message msg) throws IOException, InvalidKeySpecException
    {
        registrationLatch.countDown();
        cbRegistrationSuccess.onMessage(msg);
    }

    public void setRegistrationSuccessCallback(MessageCallback cbRegistrationSuccess)
    {
        this.cbRegistrationSuccess = cbRegistrationSuccess;
    }

    public void setRegistrationError(MessageCallback cbRegistrationError)
    {
        this.cbRegistrationError = cbRegistrationError;
    }

    public void sendUnregister()
    {
        write(new Message(getMessageId(), Protocol.Commands.Register, token.sign(privateKey), "0.0"));
    }

    public String getUserId()
    {
        return token.getUserId();
    }
}
