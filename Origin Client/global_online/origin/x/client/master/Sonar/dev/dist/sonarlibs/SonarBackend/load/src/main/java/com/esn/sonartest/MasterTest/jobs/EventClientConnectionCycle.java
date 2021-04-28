package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonartest.DiscardingEventClient;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;

import java.util.concurrent.atomic.AtomicInteger;

public class EventClientConnectionCycle extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private DiscardingEventClient eventClient;

    static public AtomicInteger liveCounter = new AtomicInteger();

    public EventClientConnectionCycle(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);

        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        liveCounter.incrementAndGet();

        eventClient = new DiscardingEventClient(masterTestDirector.getConfig(), masterTestDirector.getBootstrap(), "KAKA", new Runnable()
        {
            public void run()
            {
                liveCounter.decrementAndGet();
                signalDone();

                eventClient.close();
            }
        });

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        eventClient.connect();
    }
}
