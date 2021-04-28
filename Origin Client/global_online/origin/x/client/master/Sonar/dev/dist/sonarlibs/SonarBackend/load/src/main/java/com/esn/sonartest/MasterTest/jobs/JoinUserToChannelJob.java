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

public class JoinUserToChannelJob extends AbstractJob
{
    private final MasterTestDirector masterTestDirector;
    private User user;
    private Channel channel;

    public JoinUserToChannelJob(JobTrigger trigger, MasterTestDirector masterTestDirector)
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

        channel = masterTestDirector.getDestroyedChannelPool().acquire();

        if (channel == null)
        {
            if (user != null)
            {
                masterTestDirector.getConnectedUserPool().release(user);
            }

            return false;
        }

        user.setPendingChannel(channel);

        return true;

    }

    public void onRun() throws Exception
    {
        String location = "IP:" + masterTestDirector.getRandomIP();

        final Expectation expectedEvent = new Expectation("UserConnectedToChannel")
        {
            @Override
            public void verify(Message message)
            {
                if (!user.getUserId().equals(message.getArgument(1)) || !channel.getChannelId().equals(message.getArgument(3)))
                {
                    throw new RuntimeException();
                }

                if (user.getCurrentChannel() != channel)
                {
                    throw new RuntimeException();
                }

                if (user.getCurrentChannel() == null)
                {
                    throw new RuntimeException();
                }

                masterTestDirector.getUserInChannelPool().add(user);
                signalDone();
            }
        };

        EventVerifier.getInstance().addUserShouldBeInChannelExpectation(user, channel, expectedEvent);

        final Expectation expectedResult = new Expectation("joinUserToChannel")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(0).equals("OK"))
                {
                    throw new RuntimeException();
                }
            }
        };

        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();
        oc.joinUserToChannel(MasterTestDirector.operatorId, user.getUserId(), channel.getChannelId(), channel.getChannelDesc(), location, expectedResult);
        masterTestDirector.getOperatorClientPool().release(oc);
    }

}
