package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeUserClient;
import com.esn.sonar.allinone.client.FakeVoiceServerClient;
import com.esn.sonar.core.Token;
import org.junit.Test;

public class RegistrationTest extends BaseIntegrationTest
{
    @Test
    public void timeoutVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, true, false, false, 10);
        voiceServerClient.connect().waitForChannelClosed(30);
        voiceServerClient.close();
    }

    @Test
    public void keepaliveIgnoreVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, true, 10);
        voiceServerClient.connect().waitForRegister();
        voiceServerClient.waitForChannelClosed(30);
        voiceServerClient.close();
    }

    @Test
    public void keepaliveVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegister().waitForKeepalive(30);
        voiceServerClient.close();
    }


    @Test
    public void timeoutClient() throws InterruptedException
    {
        FakeUserClient client = clientFactory.newUserClient(new Token("operatorservice", "poppe", "poppe", "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, true, false);
        client.connect().waitForChannelClosed(4);
        client.close();
    }

    @Test
    public void successVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegister();
        voiceServerClient.close();
    }

    @Test
    public void successClient() throws InterruptedException
    {
        FakeUserClient client = clientFactory.newUserClient(new Token("operatorservice", "poppe", "poppe", "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        client.connect().waitForRegister();
        client.close();
    }

    @Test
    public void doubleRegistrationClient() throws InterruptedException
    {
        FakeUserClient client = clientFactory.newUserClient(new Token("operatorservice", "poppe", "poppe", "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, true);
        client.connect().waitForRegister();
        client.waitForErrorResult();
        client.close();
    }

    @Test
    public void doubleRegistrationVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, true, false, 10);
        voiceServerClient.connect().waitForRegisterError();
        voiceServerClient.close();
    }

    @Test
    public void syncDoubleRegistrationClient() throws InterruptedException
    {
        FakeUserClient client = clientFactory.newUserClient(new Token("operatorservice", "poppe", "poppe", "", "", "", 0, config.userEdgeConfig.getLocalAddress(), config.userEdgeConfig.getBindAddress().getPort(), "", 0), config, false, false);
        client.connect().waitForConnect().waitForRegister();
        client.sendRegister().waitForErrorResult();
        client.close();
    }

    @Test
    public void syncDoubleRegistrationVoiceServer() throws InterruptedException
    {
        FakeVoiceServerClient client = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        client.connect().waitForConnect().waitForRegister();
        client.sendRegister().waitForErrorResult();
        client.close();
    }
}
