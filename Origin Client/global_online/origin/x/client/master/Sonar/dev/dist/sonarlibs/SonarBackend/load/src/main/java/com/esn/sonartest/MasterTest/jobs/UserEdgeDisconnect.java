package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.UserEdgeClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.ResultVerifier;

import java.util.Collection;

public class UserEdgeDisconnect extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private JobTrigger[] jobTriggers;
    private double[] rateBackups;
    private UserEdgeClient edgeClient;

    public UserEdgeDisconnect(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }


    @Override
    protected boolean onStart() throws Exception
    {
        jobTriggers = masterTestDirector.getJobTriggers();


        for (JobTrigger jobTrigger : jobTriggers)
        {
            jobTrigger.pause(true);
        }

        while (AbstractJob.getJobsRunning() > 1)
        {
            System.err.printf("Jobs running %d\n", AbstractJob.getJobsRunning());
            Thread.sleep(100);
        }

        while (EventVerifier.getInstance().size() > 0 && ResultVerifier.getInstance().size() > 0)
        {
            System.err.printf("Events and Results verifications %d / %d\n", EventVerifier.getInstance().size(), ResultVerifier.getInstance().size());
            Thread.sleep(100);
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        System.err.printf("Job is running!\n");

        edgeClient = masterTestDirector.getRandomUserEdgeClient();

        edgeClient.close().awaitUninterruptibly();
        Collection<User> users = edgeClient.getUsers();

        for (User user : users)
        {
            masterTestDirector.getDisconnectedUserPool().add(user);
        }

        for (JobTrigger jobTrigger : jobTriggers)
        {
            jobTrigger.pause(false);
        }

        signalDone();
    }


}
