package com.esn.sonar.core;

import java.security.*;
import java.security.spec.EncodedKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;

public abstract class ConnectionManager
{
    private KeyPair keyPair;

    abstract public void registerClient(InboundConnection client);

    abstract public void unregisterClient(InboundConnection client, String reasonType, String reasonDesc);

    private KeyFactory keyFactory;

    protected ConnectionManager() throws NoSuchAlgorithmException
    {
        keyFactory = KeyFactory.getInstance("RSA");
    }

    public void updateKeyPair(byte[] pubKey, byte[] prvKey) throws InvalidKeySpecException
    {
        EncodedKeySpec prvKeySpec = new PKCS8EncodedKeySpec(prvKey);
        EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(pubKey);


        PrivateKey privateKey = keyFactory.generatePrivate(prvKeySpec);
        PublicKey publicKey = keyFactory.generatePublic(pubKeySpec);

        this.keyPair = new KeyPair(publicKey, privateKey);

    }


    public KeyPair getKeyPair()
    {
        return keyPair;
    }
}
