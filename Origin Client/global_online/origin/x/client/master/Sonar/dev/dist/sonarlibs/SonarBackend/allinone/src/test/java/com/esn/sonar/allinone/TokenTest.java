package com.esn.sonar.allinone;

import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import org.junit.Before;
import org.junit.Test;

import java.net.UnknownHostException;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;

import static org.junit.Assert.assertEquals;

public class TokenTest extends BaseIntegrationTest
{
    private KeyPair keyPair;

    @Before
    public void init() throws NoSuchAlgorithmException
    {
        KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
        keyGen.initialize(512, new SecureRandom());
        keyPair = keyGen.generateKeyPair();
    }

    @Test(expected = Token.InvalidToken.class)
    public void forgedToken() throws Token.InvalidToken
    {

        long ts = Utils.getTimestamp() / 1000;
        String tokenString = new Token("operatorservice", "userId", "userDesc", "channelId", "channelDesc", "", ts, "127.0.0.1", 31337, "", 0).sign(keyPair.getPrivate());

        tokenString = tokenString.replaceFirst("127\\.0\\.0\\.1", "130.244.1.1");

        ts += 10;

        Token checkToken = new Token(tokenString, keyPair.getPublic(), -1);
    }


    @Test
    public void parseToken() throws UnknownHostException, Token.InvalidToken
    {
        long ts = Utils.getTimestamp() / 1000;
        String tokenString = new Token("operatorservice", "userId", "userDesc", "channelId", "channelDesc", "location", ts, "127.0.0.1", 31337, "130.244.1.1", 31338).sign(keyPair.getPrivate());

        Token token = new Token(tokenString, keyPair.getPublic(), -1);

        assertEquals(token.getControlAddress(), "127.0.0.1");
        assertEquals(token.getControlPort(), 31337);
        assertEquals(token.getCreationTime(), ts);
        assertEquals(token.getUserId(), "userId");
        assertEquals(token.getUserDescription(), "userDesc");
        assertEquals(token.getChannelId(), "channelId");
        assertEquals(token.getChannelDescription(), "channelDesc");
        assertEquals(token.getLocation(), "location");
        assertEquals(token.getVoipAddress(), "130.244.1.1");
        assertEquals(token.getVoipPort(), 31338);
        assertEquals(token.getOperatorId(), "operatorservice");
    }
}
