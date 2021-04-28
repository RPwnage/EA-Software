package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.Channel;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.Expectation;

public class PartUserFromChannelJob extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;

    public PartUserFromChannelJob(JobTrigger trigger, MasterTestDirector masterTestDirector)
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
        final Channel currChannel = user.getCurrentChannel();

        Expectation resultExpectation = new Expectation("PartUserFromChannel")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(0).equals("OK"))
                {
                    throw new RuntimeException("User was not in channel");
                }
            }
        };

        final Expectation expectedUserPartEvent = new Expectation("UserPartEvent")
        {
            @Override
            public void verify(Message message)
            {
                if (!user.getUserId().equals(message.getArgument(1)) || !currChannel.getChannelId().equals(message.getArgument(3)))
                {
                    throw new RuntimeException();
                }

                masterTestDirector.getConnectedUserPool().release(user);
            }
        };

        final Expectation expectedDestroyEvent = new Expectation("ChannalDestroyEvent")
        {
            @Override
            public void verify(Message message)
            {
                masterTestDirector.getDestroyedChannelPool().release(currChannel);
                signalDone();
            }
        };

        EventVerifier.getInstance().addUserShouldLeaveChannelExpectation(user, currChannel, expectedUserPartEvent);
        EventVerifier.getInstance().addChannelShouldBeDestroyed(currChannel, expectedDestroyEvent);

        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();
        oc.partUserFromChannel(MasterTestDirector.operatorId, user.getUserId(), currChannel.getChannelId(), "UNKNOWN", user.getUserId(), resultExpectation);

        masterTestDirector.getOperatorClientPool().release(oc);

    }
}
