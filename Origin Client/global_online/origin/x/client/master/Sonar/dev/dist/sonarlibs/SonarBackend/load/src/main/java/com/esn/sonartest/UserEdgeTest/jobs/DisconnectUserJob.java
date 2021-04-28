package com.esn.sonartest.UserEdgeTest.jobs;

import com.esn.sonartest.UserEdgeTest.User;
import com.esn.sonartest.UserEdgeTest.UserEdgeTestDirector;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-07-04
 * Time: 18:35
 * To change this template use File | Settings | File Templates.
 */
public class DisconnectUserJob extends AbstractJob
{
    private UserEdgeTestDirector director;
    private boolean nice;
    private User user;

    public DisconnectUserJob(JobTrigger jobTrigger, UserEdgeTestDirector userEdgeTestDirector, boolean nice)
    {
        super(jobTrigger);

        this.director = userEdgeTestDirector;
        this.nice = nice;
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
        this.user.close().addListener(new ChannelFutureListener()
        {
            public void operationComplete(ChannelFuture future) throws Exception
            {
                signalDone();
                director.getDisconnectedUserPool().add(user);
            }
        });
    }

}
