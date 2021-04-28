package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.Expectation;

public class UserDisconnect extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;

    public UserDisconnect(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.user = masterTestDirector.getConnectedUserPool().acquire();

        if (user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        Expectation eventExpectation = new Expectation("UserDisconnect")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(1).equals(user.getUserId()))
                {
                    throw new RuntimeException("User doesn't match");
                }

                masterTestDirector.getDisconnectedUserPool().add(user);
                signalDone();
            }
        };

        EventVerifier.getInstance().addUserShouldDisconnect(user, eventExpectation);
        user.getEdgeClient().disconnectUser(user);
    }

}
