package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.Expectation;

public class DisconnectUser extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;

    public DisconnectUser(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        user = masterTestDirector.getConnectedUserPool().acquire();

        if (user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();

        EventVerifier.getInstance().addUserShouldDisconnect(user, new Expectation("UserDisconnectEvent(DisconnectUser)")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(1).equals(user.getUserId()))
                {
                    throw new RuntimeException("UserId mismatch");
                }

                if (user.getCurrentChannel() != null)
                {
                    throw new RuntimeException("User was in channel");
                }

                masterTestDirector.getDisconnectedUserPool().add(user);
                signalDone();
            }
        });

        oc.disconnectUser(MasterTestDirector.operatorId, user.getUserId(), "UNKNOWN", "", new Expectation("DisconnectUserResult")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(0).equals("OK"))
                {
                    throw new RuntimeException();
                }
            }
        });

        masterTestDirector.getOperatorClientPool().release(oc);
    }

}
