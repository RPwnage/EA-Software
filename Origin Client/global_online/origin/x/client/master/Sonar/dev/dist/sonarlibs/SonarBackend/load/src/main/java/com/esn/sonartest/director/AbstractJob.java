package com.esn.sonartest.director;

import java.util.concurrent.atomic.AtomicInteger;

public abstract class AbstractJob implements Runnable
{
    private long tsDone;
    private long tsRun;
    private long tsAbort;
    private final JobTrigger trigger;

    private static final AtomicInteger jobsRunning = new AtomicInteger(0);

    protected AbstractJob(JobTrigger trigger)
    {
        this.trigger = trigger;
    }


    /*
    Called before onRun. Jobs which fails to start due to predictable causes should return false here.
     */
    abstract protected boolean onStart() throws Exception;

    /*
    Called after onStart() should contain actual test part of the job
     */
    abstract protected void onRun() throws Exception;

    public void run()
    {
        try
        {
            jobsRunning.incrementAndGet();

            if (!onStart())
            {
                signalAbort();
                jobsRunning.decrementAndGet();
                return;
            }

            signalRun();
            onRun();

        } catch (Exception e)
        {
            e.printStackTrace();
        }
        jobsRunning.decrementAndGet();
    }

    private void signalAbort()
    {
        tsAbort = System.currentTimeMillis();
        trigger.signalJobAborted();
    }

    private void signalRun()
    {
        tsRun = System.currentTimeMillis();
    }

    protected void signalDone()
    {
        tsDone = System.currentTimeMillis();
        trigger.signalJobDone(tsDone - tsRun);
    }

    public static int getJobsRunning()
    {
        return jobsRunning.get();
    }
}
