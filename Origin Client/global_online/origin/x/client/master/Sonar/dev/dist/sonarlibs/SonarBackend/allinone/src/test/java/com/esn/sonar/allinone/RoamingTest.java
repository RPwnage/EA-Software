package com.esn.sonar.allinone;

import com.esn.sonar.allinone.client.FakeUserClient;
import com.esn.sonar.allinone.client.FakeVoiceServerClient;
import com.esn.sonar.core.Token;
import com.esn.sonar.master.api.operator.OperatorService;
import com.jayway.awaitility.core.ConditionFactory;
import org.hamcrest.Matchers;
import org.junit.Ignore;
import org.junit.Test;

import java.util.UUID;
import java.util.concurrent.Callable;

import static com.jayway.awaitility.Awaitility.await;
import static java.util.concurrent.TimeUnit.SECONDS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class RoamingTest extends BaseIntegrationTest
{
    private final ConditionFactory await = await().atMost(5, SECONDS);

    /**
     * Voice server was shutdown correctly and sent an UNREGISTER to server
     * It is responsible for disconnecting the TCP socket
     * Backend server will then roam all users to another server.
     */
    @Test
    @Ignore("We do not intend to implement this functionality")
    public void voiceServerCleanShutdown()
    {
        System.out.println("RoamingTest.voiceServerCleanShutdown");
    }

    /**
     * Connection to voice server was lost due to unknown reason (process killed, bad internet etc)
     * Backend server will then take no action. Users will reconnect themselves to backend server
     * and ask for a voice server to join with their channel
     * This is by design to protect ourselves from roaming an entire the entire user base when the backend server loses its
     * internet connection
     */
    @Test
    public void voiceServerUncleanShutdown() throws Exception, OperatorService.ChannelAllocationFailed, OperatorService.InvalidArgumentException, OperatorService.ChannelNotFoundException, Token.InvalidToken
    {
        System.out.println("RoamingTest.voiceServerUncleanShutdown");
        config.voiceEdgeConfig.setChallengeTimeout(10);

        // Connect voice server 1
        // Connect voice server 2
        FakeVoiceServerClient vs1 = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        FakeVoiceServerClient vs2 = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        vs1.connect().waitForRegister();
        vs2.connect().waitForRegister();

        // Connect client 1 to voice channel 1
        // Connect client 2 to voice channel 1
        FakeUserClient us1 = clientFactory.newUserClient(new Token("", "user1", "user1", "channel1", "", "", 0, "", 0, "", 0), config, false, false);
        FakeUserClient us2 = clientFactory.newUserClient(new Token("", "user2", "user2", "channel1", "", "", 0, "", 0, "", 0), config, false, false);

        us1.connect().waitForRegister();
        us2.connect().waitForRegister();

        operatorService.joinUserToChannel("", "", "user1", "channel1", "");
        operatorService.joinUserToChannel("", "", "user2", "channel1", "");

        us1.waitForJoinChannel();
        us2.waitForJoinChannel();

        // Shutdown voice server with channel
        FakeVoiceServerClient errServer;
        FakeVoiceServerClient okServer;

        if (vs1.getClientCount() == 2)
        {
            errServer = vs1;
            okServer = vs2;
        } else if (vs2.getClientCount() == 2)
        {
            errServer = vs2;
            okServer = vs1;
        } else
        {
            assertTrue(false);
            return;
        }

        Thread.sleep(1000);

        errServer.close();

        // Check if both clients are still in channel through voice cache
        await.until(usersInChannel("", "channel1"), Matchers.hasItems("user1", "user2"));


        // Reconnect client 1 to voice channel 1
        operatorService.joinUserToChannel("", "", "user1", "channel1", "");
        us1.waitForJoinChannel();

        // Make sure voice channel is allocated on other voice server
        await.until(usersInChannel("", "channel1"), Matchers.hasItems("user1"));
        assertEquals(1, okServer.getClientCount());

        UUID uuid = errServer.getUuid();

        errServer = clientFactory.newVoiceServerClient(config, false, false, false, false, false, 10);
        errServer.setUuid(uuid);
        errServer.connectUser(us2, us2.getCurrentToken().toString());
        errServer.connect().waitForRegister();

        //us2.waitForVoiceServerDisconnect();


        okServer.close();
        us1.close();
        us2.close();
        errServer.close();

        System.out.println("RoamingTest.voiceServerUncleanShutdown done");
    }

    private Callable<Iterable<String>> usersInChannel(final String operatorId, final String channelId)
    {
        return new Callable<Iterable<String>>()
        {
            public Iterable<String> call() throws Exception
            {
                return operatorService.getUsersInChannel(operatorId, channelId);
            }
        };
    }


    /**
     * Backend server experienced a hardware/OS failure or process crashed/hung
     * All users should remain on their respective voice servers and then slowly reconnect their control connections.
     * When the voice server reconnects to backend server it will sync the state back to the backend.
     * Should any inconsistencies arise, backend server will assign the channel to whatever voice claimed it first
     */
    @Test
    @Ignore("Not implemented yet")
    public void backendServerProcessKilled()
    {
        System.out.println("RoamingTest.backendServerProcessKilled");
    }

    /**
     * Backend server lost its internet connection (temporarily).
     * Actions must (or must not) be taken to ensure everything doesn't go haywire with roaming around all users.
     */
    @Test
    @Ignore("Not implemented yet")
    public void backendServerLostInternetConnection()
    {
        System.out.println("RoamingTest.backendServerLostInternetConnection");
    }
}
