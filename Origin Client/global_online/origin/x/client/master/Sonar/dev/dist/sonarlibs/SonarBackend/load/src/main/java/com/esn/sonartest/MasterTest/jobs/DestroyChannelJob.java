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

public class DestroyChannelJob extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;

    public DestroyChannelJob(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.user = masterTestDirector.getUserInChannelPool().acquire();

        if (this.user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        final Channel channel = user.getCurrentChannel();

        EventVerifier.getInstance().addChannelShouldBeDestroyed(channel, new Expectation("ChannelDestroyEvent")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(1).equals(channel.getChannelId()))
                {
                    throw new RuntimeException("Channel Id doesn't match");
                }
            }
        });

        EventVerifier.getInstance().addUserShouldLeaveChannelExpectation(user, channel, new Expectation("UserPartChannelEvent")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(1).equals(user.getUserId()) ||
                        !message.getArgument(3).equals(channel.getChannelId()))
                {
                    throw new RuntimeException("User or Channel Id doesn't match");
                }

                masterTestDirector.getConnectedUserPool().add(user);
                masterTestDirector.getDestroyedChannelPool().add(channel);

                signalDone();
            }
        });

        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();

        oc.destroyChannel(MasterTestDirector.operatorId, channel.getChannelId(), "UNKNOWN", "", new Expectation("DestroyChannel")
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
