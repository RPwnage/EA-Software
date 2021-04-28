package com.esn.sonar.master.user;

import com.esn.sonar.core.Token;

public class User
{
    private final String userId;
    private final String userDesc;
    private final UserEdgeConnection connection;
    private final String operatorId;
    private final String remoteAddress;

    public User(String operatorId, String userId, String userDesc, String remoteAddress, UserEdgeConnection connection)
    {
        this.userId = userId;
        this.userDesc = userDesc;
        this.remoteAddress = remoteAddress;
        this.connection = connection;
        this.operatorId = operatorId;
    }

    public String getRemoteAddress()
    {
        return remoteAddress;
    }

    public String getUserId()
    {
        return userId;
    }

    public String getUserDesc()
    {
        return userDesc;
    }

    public UserEdgeConnection getConnection()
    {
        return connection;
    }

    public void sendUpdateToken(Token token, String type)
    {
        connection.sendEdgeUpdateToken(operatorId, userId, token, type);
    }

    public void sendUnregisterClient(String reasonType, String reasonDesc)
    {
        connection.sendEdgeUnregisterClient(operatorId, userId, reasonType, reasonDesc);
    }

    public String getOperatorId()
    {
        return operatorId;
    }

    @Override
    public String toString()
    {
        return operatorId + "|" + userId + "|" + remoteAddress;
    }
}
