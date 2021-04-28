package com.esn.sonartest.UserEdgeTest.jobs;

import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.UserEdgeTest.User;
import com.esn.sonartest.UserEdgeTest.UserEdgeTestDirector;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

public class ConnectUserJob extends AbstractJob
{
    private final UserEdgeTestDirector director;
    private User user;

    public ConnectUserJob(JobTrigger trigger, UserEdgeTestDirector director)
    {
        super(trigger);
        this.director = director;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.user = director.getDisconnectedUserPool().acquire();

        if (user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        final Timeout timeout = Maintenance.getInstance().scheduleTask(5000, new TimerTask()
        {
            public void run(Timeout timeout) throws Exception
            {
                throw new RuntimeException("Registration failed, timed out");
            }
        });

        user.setRegistrationSuccessCallback(new User.MessageCallback()
        {
            public void onMessage(Message msg)
            {
                timeout.cancel();
                signalDone();
                director.getConnectedUserPool().add(user);
            }
        });

        user.setRegistrationError(new User.MessageCallback()
        {
            public void onMessage(Message msg)
            {
                // We accept try again and simply retry again later
                if (!msg.getArgument(1).equals(Protocol.Reasons.TryAgain))
                {
                    throw new RuntimeException();
                }

                timeout.cancel();
                user.close().awaitUninterruptibly();
                director.getDisconnectedUserPool().add(user);
            }
        });


        user.connect();

    }
}