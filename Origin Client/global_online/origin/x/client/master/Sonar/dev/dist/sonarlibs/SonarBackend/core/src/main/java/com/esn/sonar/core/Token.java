package com.esn.sonar.core;

import com.esn.sonar.core.util.Base64;
import com.esn.sonar.core.util.Utils;
import org.jboss.netty.util.CharsetUtil;

import java.io.IOException;
import java.security.*;

import static com.esn.sonar.core.util.Utils.tokenize;

public class Token
{
    public static class InvalidToken extends Exception
    {
        public InvalidToken(String message)
        {
            super(message);
        }
    }

    private final String userId;
    private final String userDescription;
    private final String channelId;
    private final String channelDescription;
    private final String controlAddress;
    private final int controlPort;
    private long creationTime;
    private final int voipPort;
    private final String voipAddress;
    private final String location;
    private final String operatorId;

    /*
    SONAR2 | [controlAddress]:[controlPort] | [voipAddress]:[voipPort] | [operatorId] | [userId] | [userDesc] | [channelId] | [channelDesc] | [location] | [creationTime] | [signed]
    */

    public Token(String operatorId, String userId, String userDescription, String channelId, String channelDescription, String location, long creationTimeSEC, String controlAddress, int controlPort, String voipAddress, int voipPort)
    {
        this.userId = userId;
        this.userDescription = userDescription;
        this.channelId = channelId;
        this.channelDescription = channelDescription;
        this.controlAddress = controlAddress;
        this.controlPort = controlPort;
        this.voipAddress = voipAddress;
        this.voipPort = voipPort;
        this.location = location;
        this.operatorId = operatorId;

        if (creationTimeSEC == 0)
        {
            creationTimeSEC = Utils.getTimestamp() / 1000;
        }

        this.creationTime = creationTimeSEC;
    }

    public Token(String rawToken, PublicKey key, long maxAgeSEC) throws InvalidToken
    {
        String[] args = parse(rawToken);
        if (args == null)
            throw new InvalidToken("Couldn't parse token");


        try
        {
            creationTime = Long.valueOf(args[9]);
        } catch (NumberFormatException e)
        {
            throw new InvalidToken("Unable to parse creation time");
        }

        long tsNow = (Utils.getTimestamp() / 1000);

        if (maxAgeSEC != -1 && tsNow - creationTime > maxAgeSEC)
        {
            throw new InvalidToken("Token expired");
        }

        if (key != null)
        {
            try
            {
                String inputDigest = args[args.length - 1];
                String message = rawToken.substring(0, rawToken.lastIndexOf('|') + 1);

                byte[] messageBytes = message.getBytes(CharsetUtil.UTF_8);

                byte[] signatureBytes = Base64.decode(inputDigest, Base64.DONT_GUNZIP);

                Signature signature = Signature.getInstance("SHA1withRSA");
                signature.initVerify(key);
                signature.update(messageBytes);

                if (!signature.verify(signatureBytes))
                {
                    throw new InvalidToken("Signature mismatch");
                }
            } catch (IOException e)
            {
                throw new InvalidToken(e.getMessage());
            } catch (NoSuchAlgorithmException e)
            {
                throw new InvalidToken(e.getMessage());
            } catch (InvalidKeyException e)
            {
                throw new InvalidToken(e.getMessage());
            } catch (SignatureException e)
            {
                throw new InvalidToken(e.getMessage());
            }
        }

        //SONAR2 | [controlAddress]:[controlPort] | [voipAddress]:[voipPort] | [operatorId] | [userId] | [userDesc] | [channelId] | [channelDesc] | [location] | [creationTime] | [signed]

        String[] controlAddressSplit = Utils.tokenize(args[1], ':');


        if (controlAddressSplit.length == 2)
        {
            controlAddress = controlAddressSplit[0];
            try
            {
                controlPort = Integer.valueOf(controlAddressSplit[1]);
            } catch (NumberFormatException e)
            {
                throw new InvalidToken("Unable to parse control address port to integer");
            }
        } else
        {
            controlAddress = "";
            controlPort = 0;
        }


        String[] voipAddressSplit = Utils.tokenize(args[2], ':');

        if (voipAddressSplit.length == 2)
        {
            voipAddress = voipAddressSplit[0];

            try
            {
                voipPort = Integer.valueOf(voipAddressSplit[1]);
            } catch (NumberFormatException e)
            {
                throw new InvalidToken("Unable to parse voip address port to integer");
            }
        } else
        {
            voipAddress = "";
            voipPort = 0;
        }


        operatorId = args[3];
        userId = args[4];
        userDescription = args[5];
        channelId = args[6];
        channelDescription = args[7];
        location = args[8];


    }

    public String sign(PrivateKey key)
    {
        StringBuilder clientToken = new StringBuilder();

        clientToken.append("SONAR2");
        clientToken.append('|');

        if (controlPort != 0)
        {
            clientToken.append(controlAddress);
            clientToken.append(':');
            clientToken.append(controlPort);
        }
        clientToken.append('|');

        if (voipPort != 0)
        {
            clientToken.append(voipAddress);
            clientToken.append(":");
            clientToken.append(voipPort);
        }
        clientToken.append('|');
        clientToken.append(operatorId);
        clientToken.append('|');
        clientToken.append(userId);
        clientToken.append("|");
        clientToken.append(userDescription);
        clientToken.append('|');
        clientToken.append(channelId);
        clientToken.append('|');
        clientToken.append(channelDescription);
        clientToken.append('|');
        clientToken.append(location);
        clientToken.append('|');
        clientToken.append(creationTime);
        clientToken.append('|');

        String message = clientToken.toString();

        if (key != null)
        {
            try
            {
                clientToken.append(sign(key, message));
            } catch (InvalidKeyException e)
            {
            } catch (NoSuchProviderException e)
            {
            } catch (NoSuchAlgorithmException e)
            {
            } catch (SignatureException e)
            {
            } catch (IOException e)
            {
            }
        }

        return clientToken.toString();
    }


    public String toString()
    {
        return sign(null);
    }

    public String getUserId()
    {
        return userId;
    }

    public String getLocation()
    {
        return location;
    }

    public String getUserDescription()
    {
        return userDescription;
    }

    public String getChannelId()
    {
        return channelId;
    }

    public String getChannelDescription()
    {
        return channelDescription;
    }

    public String getControlAddress()
    {
        return controlAddress;
    }

    public int getControlPort()
    {
        return controlPort;
    }

    public long getCreationTime()
    {
        return creationTime;
    }

    public String getVoipAddress()
    {
        return voipAddress;
    }

    public int getVoipPort()
    {
        return voipPort;
    }

    public String getOperatorId()
    {
        return operatorId;
    }

    public void setCreationTime(long creationTime)
    {
        this.creationTime = creationTime;
    }

    private static SecureRandom secureRandom = new SecureRandom();

    public static String sign(PrivateKey key, String message) throws InvalidKeyException, NoSuchProviderException, NoSuchAlgorithmException, SignatureException, IOException
    {
        byte[] messageBytes = message.getBytes(CharsetUtil.UTF_8);
        Signature signature = Signature.getInstance("SHA1withRSA");

        signature.initSign(key, secureRandom);
        signature.update(messageBytes);

        byte[] signatureBytes = signature.sign();

        return Base64.encodeBytes(signatureBytes, Base64.NO_OPTIONS);
    }

    private static String[] parse(String token)
    {
        if (!token.startsWith("SONAR2|"))
        {
            return null;
        }

        String[] argsPlain = tokenize(token, '|');

        if (argsPlain.length < 11)
        {
            return null;
        }
        return argsPlain;
    }
}
