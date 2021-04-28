package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeVoiceServerClient;
import com.esn.sonar.allinone.client.SonarClientFactory;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2010-dec-02
 * Time: 13:58:23
 * To change this template use File | Settings | File Templates.
 */
public class ChallengeTest extends BaseIntegrationTest
{
    FakeVoiceServerClient voiceServerClient;
    SonarClientFactory clientFactory;

    @Before
    public void start()
    {
        clientFactory = new SonarClientFactory();
    }

    @After
    public void after()
    {
        voiceServerClient.close();
        clientFactory.release();
    }


    @Test
    public void challengeExpire() throws InterruptedException
    {
        voiceServerClient = clientFactory.newVoiceServerClient(config, false, true, false, false, false, 10);
        voiceServerClient.connect().waitForRegisterError();
        voiceServerClient.close();
    }

    @Test
    public void challengeMalformed() throws InterruptedException
    {
        voiceServerClient = clientFactory.newVoiceServerClient(config, true, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegisterError();
        voiceServerClient.close();
    }

    @Test
    public void challengeReply() throws InterruptedException
    {
        voiceServerClient = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        voiceServerClient.connect().waitForRegister();
        voiceServerClient.close();
    }
}
