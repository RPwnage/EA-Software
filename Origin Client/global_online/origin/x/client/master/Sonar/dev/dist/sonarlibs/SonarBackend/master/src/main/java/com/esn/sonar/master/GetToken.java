package com.esn.sonar.master;

import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;

public class GetToken
{
    public static void main(String[] args)
    {
        System.out.printf("TOKEN CONNECT: %s\n", new Token("default", "jonas", "Jonas", "", "", "", Utils.getTimestamp() / 1000, "192.168.1.174", 22990, "192.168.1.174", 22990).toString());
        System.out.printf("TOKEN CONNECT: %s\n", new Token("default", "nisse", "Nisse", "", "", "", Utils.getTimestamp() / 1000, "192.168.1.174", 22990, "192.168.1.174", 22990).toString());
        System.out.printf("TOKEN CHANNEL: %s\n", new Token("default", "jonas", "Jonas", "jonaschannel", "Jonas Pretty Channel", "", Utils.getTimestamp() / 1000, "192.168.1.174", 22990, "192.168.1.174", 22990).toString());
    }
}
