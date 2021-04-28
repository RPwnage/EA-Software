package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.Channel;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.Expectation;

public class GetUsersInChannel extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;


    public GetUsersInChannel(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        user = masterTestDirector.getUserInChannelPool().acquire();

        if (user == null)
        {
            return false;
        }


        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        Channel currChannel = user.getCurrentChannel();

        final Expectation expectation = new Expectation("GetUsersInChannel")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(0).equals("OK"))
                {
                    throw new RuntimeException("Unexpected result");
                }

                boolean found = false;

                for (int index = 1; index < message.getArgumentCount(); index++)
                {
                    if (message.getArgument(index).equals(user.getUserId()))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    throw new RuntimeException("User was not in channel");
                }

                masterTestDirector.getUserInChannelPool().release(user);
                signalDone();
            }
        };

        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();
        oc.getUsersInChannel(MasterTestDirector.operatorId, currChannel.getChannelId(), expectation);
        masterTestDirector.getOperatorClientPool().release(oc);
    }
}
