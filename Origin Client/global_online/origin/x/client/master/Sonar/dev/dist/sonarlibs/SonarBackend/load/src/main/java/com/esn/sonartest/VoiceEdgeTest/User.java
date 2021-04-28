package com.esn.sonartest.VoiceEdgeTest;

public class User
{
    private String userDesc;
    private String userId;

    public User(String userId, String userDesc)
    {
        this.userId = userId;
        this.userDesc = userDesc;
    }


    public String getUserId()
    {
        return userId;
    }

    public String getUserDesc()
    {
        return userDesc;
    }
}
