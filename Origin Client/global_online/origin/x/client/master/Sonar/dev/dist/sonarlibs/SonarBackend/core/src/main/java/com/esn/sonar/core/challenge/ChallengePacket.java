package com.esn.sonar.core.challenge;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2010-dec-02
 * Time: 11:50:37
 * To change this template use File | Settings | File Templates.
 */
public class ChallengePacket
{
    private final int challengeId;

    public ChallengePacket(int challengeId)
    {
        this.challengeId = challengeId;
    }

    public int getChallengeId()
    {
        return challengeId;
    }
}
