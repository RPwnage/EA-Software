package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonar.core.Token;
import com.esn.sonar.master.MasterServer;
import org.apache.log4j.Logger;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class FakeUserClient extends SonarClientHandler
{
    private final Logger log = Logger.getLogger(FakeUserClient.class);

    private CountDownLatch joinChannelLatch;
    private CountDownLatch partChannelLatch;
    private CountDownLatch voiceServerConnectLatch = new CountDownLatch(1);
    private CountDownLatch voiceServerDisconnectLatch = new CountDownLatch(1);
    private boolean voiceServerHasBeenConnected = false; // Has been connected to voice server at least once
    private String currentChannel;
    private Token currentToken;
    private final Token controlToken;
    private final boolean skipRegistration;
    private final boolean doubleRegistration;
    private CountDownLatch updateTokenLatch = new CountDownLatch(1);

    public FakeUserClient(ClientBootstrap bootstrap, Token controlToken, AllInOneConfig config, boolean skipRegistration, boolean doubleRegistration)
    {
        super(bootstrap, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), config);
        this.controlToken = controlToken;
        this.skipRegistration = skipRegistration;
        this.doubleRegistration = doubleRegistration;
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        super.channelConnected(ctx, e);
        joinChannelLatch = new CountDownLatch(1);
        voiceServerConnectLatch = new CountDownLatch(1);
        voiceServerDisconnectLatch = new CountDownLatch(1);

        log.info("Connected");
        log.info("Registering with server..");

        sendRegister();
    }

    @Override
    public SonarClientHandler sendRegister()
    {
        String version = String.format("%d.%d", Protocol.Versions.voiceClientMajor, 1);

        String rawToken = controlToken.sign(MasterServer.getInstance().getKeyPair().getPrivate());

        if (!skipRegistration)
        {
            channel.write(new Message(getMessageId(), "REGISTER", rawToken, version));

            if (doubleRegistration)
            {
                channel.write(new Message(getMessageId(), "REGISTER", rawToken, version));
            }
        }

        return this;
    }

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception
    {
        if (currentToken != null)
        {
            if (currentChannel != null)
            {
                FakeVoiceServerClient.getServer(currentToken.getVoipPort()).disconnectUser(this);
                if (partChannelLatch != null)
                    partChannelLatch.countDown();
            }
        }

        super.channelDisconnected(ctx, e);
        log.info("Disconnected");
    }

    @Override
    public void onMessage(Message message) throws Token.InvalidToken
    {
        log.info("Received: " + message);
        if (message.getName().equals(Protocol.Commands.UpdateToken))
        {
            currentToken = new Token(message.getArgument(0), null, -1);

            if (message.getArgument(1).equals("UPDATE"))
            {
                updateTokenLatch.countDown();
            } else
            {
                int tempPort = currentToken.getVoipPort();
                FakeVoiceServerClient vs = FakeVoiceServerClient.getServer(tempPort);
                vs.connectUser(this, message.getArgument(0));
            }
        }
    }

    @Override
    public void onRegister(Message message)
    {
    }

    public void onVoiceServerConnect()
    {
        // TODO: Check that we actually got connected to channel? Since all this is faked..
        currentChannel = currentToken.getChannelId();

        assertFalse("Empty channel Id", currentChannel.length() == 0);

        partChannelLatch = new CountDownLatch(1);
        voiceServerDisconnectLatch = new CountDownLatch(1);
        voiceServerConnectLatch.countDown();
        joinChannelLatch.countDown();
        voiceServerHasBeenConnected = true;
    }

    public void onVoiceServerDisconnect()
    {
        currentToken = null;
        currentChannel = null;
        voiceServerDisconnectLatch.countDown();
    }

    public FakeUserClient waitForJoinChannel() throws InterruptedException
    {
        waitForVoiceServerConnect();
        assertTrue("Join channel latch", joinChannelLatch.await(registerTimeout, TimeUnit.MILLISECONDS));
        assertNotNull("User is in a channel", getCurrentChannel());
        return this;
    }

    public FakeUserClient waitForVoiceServerConnect() throws InterruptedException
    {
        assertTrue("Voice server connect latch", voiceServerConnectLatch.await(registerTimeout, TimeUnit.MILLISECONDS));
        assertTrue("User is connected", isConnected());
        return this;
    }

    public FakeUserClient waitForVoiceServerDisconnect() throws InterruptedException
    {
        assertTrue("Has been connected to voice server at least once", voiceServerHasBeenConnected);
        assertTrue("Voice server disconnect latch", voiceServerDisconnectLatch.await(registerTimeout, TimeUnit.MILLISECONDS));
        assertFalse("User is disconnected", isConnected());
        return this;
    }


    public FakeUserClient waitForUpdateToken(int sec) throws InterruptedException
    {
        assertTrue("Update Token latch failed", updateTokenLatch.await(sec, TimeUnit.SECONDS));
        return this;

    }


    public String getCurrentChannel()
    {
        return currentChannel;
    }

    public Token getCurrentToken()
    {
        return currentToken;
    }

    public String getUserId()
    {
        return controlToken.getUserId();
    }

    public boolean isConnected()
    {
        return currentToken != null;
    }

    public String getUserDescription()
    {
        return controlToken.getUserDescription();
    }

    public String getOperatorId()
    {
        return controlToken.getOperatorId();
    }
}
