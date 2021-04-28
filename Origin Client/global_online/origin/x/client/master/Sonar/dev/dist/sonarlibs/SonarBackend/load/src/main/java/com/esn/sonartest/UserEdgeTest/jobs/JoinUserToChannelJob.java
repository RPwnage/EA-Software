package com.esn.sonartest.UserEdgeTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.UserEdgeTest.User;
import com.esn.sonartest.UserEdgeTest.UserEdgeTestDirector;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.Expectation;

public class JoinUserToChannelJob extends AbstractJob
{
    private UserEdgeTestDirector director;
    private User user;

    public JoinUserToChannelJob(JobTrigger trigger, UserEdgeTestDirector director)
    {
        super(trigger);
        this.director = director;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.user = director.getConnectedUserPool().acquire();

        if (user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        OperatorClient oc = director.getOperatorPool().acquire();
        oc.joinUserToChannel("OPER", user.getUserId(), user.getUserId(), "", "", new Expectation("Dummy")
        {
            @Override
            public void verify(Message message)
            {
                if (!message.getArgument(0).equals("OK"))
                {
                    throw new RuntimeException("");
                }

                director.getConnectedUserPool().release(user);
                signalDone();
            }
        });
        director.getOperatorPool().release(oc);

    }
}
